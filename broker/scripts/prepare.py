#!/usr/bin/env python3
"""Create a disposable, private lab. No passwords in argv, stdout or Docker env."""
import argparse
import base64
import json
import os
import re
import secrets
import subprocess
import sys
from common import BROKER, ROOT, IMAGE, docker, host_path, private_write


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--node-id', required=True, help='ID real del nodo (sin secretos)')
    parser.add_argument('--peer-id', default='acl-check-peer', help='Segundo nodo local para verificar aislamiento ACL')
    args = parser.parse_args()
    if args.peer_id == args.node_id or not all(re.fullmatch(r'[A-Za-z0-9_-]{1,55}', x) for x in (args.node_id, args.peer_id)):
        raise RuntimeError('ID inválido: usar 1–55 caracteres alfanuméricos, _ o -.')
    lab = BROKER / '.secrets'
    if lab.exists() and any(lab.iterdir()):
        raise RuntimeError('broker/.secrets ya existe; no se sobrescribe. Conservarlo y usar otro checkout para otro laboratorio.')
    docker('info', '--format', '{{.ServerVersion}}', stdout=subprocess.DEVNULL)
    docker('pull', IMAGE)
    # Confirm Git exclusion BEFORE generating any private content.
    subprocess.run(['git', 'check-ignore', '-q', 'broker/.secrets/clients.json'], cwd=ROOT, check=True)
    os.umask(0o077)
    lab.mkdir(mode=0o700, exist_ok=True)
    if re.match(r'^/mnt/[a-z]/', str(lab)) is not None and restrict_windows_acl(lab) is False:
        raise RuntimeError('No se pudo restringir ACL Windows de broker/.secrets; corregir permisos antes de generar secretos.')
    runtime = lab / 'runtime'
    runtime.mkdir(mode=0o700)

    accounts = {role: {'username': f'{role}-{args.node_id}', 'password': secrets.token_urlsafe(32)}
                for role in ('node', 'operator', 'viewer')}
    peer = {role: {'username': f'{role}-{args.peer_id}', 'password': secrets.token_urlsafe(32)}
            for role in ('node', 'operator', 'viewer')}
    # -U hashes a private plaintext file in-place. No -b password argument.
    private_write(runtime / 'mosquitto.passwd', ''.join(f'{a["username"]}:{a["password"]}\n' for a in [*accounts.values(), *peer.values()]))
    try:
        docker('run', '--rm', '--network', 'none', '--entrypoint', 'mosquitto_passwd',
               '-v', host_path(runtime) + ':/private', IMAGE, '-U', '/private/mosquitto.passwd',
               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except BaseException:
        # Do not leave a plaintext password_file after a failed hash operation.
        (runtime / 'mosquitto.passwd').write_text('')
        raise
    if not all(line.split(':', 1)[1].startswith('$7$') for line in (runtime / 'mosquitto.passwd').read_text().splitlines()):
        raise RuntimeError('mosquitto_passwd no produjo hashes PBKDF2 esperados; no iniciar broker.')
    template = (BROKER / 'config/acl.template').read_text()
    private_write(runtime / 'acl', '\n'.join(template.replace('@NODE_ID@', node) for node in (args.node_id, args.peer_id)))
    private_write(lab / 'clients.json', json.dumps({'node_id': args.node_id, 'host': 'localhost',
                                                 'accounts': accounts, 'peer_id': args.peer_id, 'peer_accounts': peer}, indent=2) + '\n')
    private_write(runtime / 'ready', 'prepared\n')
    print('Laboratorio preparado en broker/.secrets; credenciales solo en clients.json privado. Sin certificados.')


def restrict_windows_acl(path):
    """DrvFs chmod may be ineffective. Restrict the directory to the current Windows SID."""
    command = "$ErrorActionPreference='Stop'; $p='" + subprocess.check_output(['wslpath', '-w', str(path)], text=True).strip().replace("'", "''") + "'; $id=[System.Security.Principal.WindowsIdentity]::GetCurrent().User; $acl=Get-Acl -LiteralPath $p; $acl.SetAccessRuleProtection($true,$false); foreach($old in @($acl.Access)){ $acl.RemoveAccessRuleSpecific($old) }; $rule=New-Object System.Security.AccessControl.FileSystemAccessRule($id,'FullControl','ContainerInherit,ObjectInherit','None','Allow'); $acl.AddAccessRule($rule); [System.IO.Directory]::SetAccessControl($p,$acl); $check=Get-Acl -LiteralPath $p; if(-not $check.AreAccessRulesProtected){exit 2}; $rules=$check.GetAccessRules($true,$true,[System.Security.Principal.SecurityIdentifier]); if($rules.Count -eq 0){exit 3}; foreach($r in $rules){if($r.IdentityReference.Value -ne $id.Value){exit 4}}"
    windows = subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive', '-EncodedCommand', base64.b64encode(command.encode('utf-16le')).decode()], cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return windows.returncode == 0


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError, OSError) as error:
        print(str(error), file=sys.stderr)
        print('Preparación fallida; comprobar Docker/permisos y broker/README.md. No se habilitó acceso anónimo.', file=sys.stderr)
        sys.exit(1)
