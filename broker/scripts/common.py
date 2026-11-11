"""Local Docker CLI helpers; never put credential content in subprocess arguments."""
import os
from functools import lru_cache
from pathlib import Path
import shutil
import subprocess

BROKER = Path(__file__).resolve().parents[1]
ROOT = BROKER.parent
IMAGE = 'eclipse-mosquitto:2.0.22@sha256:81a6193ae693942a3231cd3ac52875b02b7e3a19da671b2fb4b55cc874c5b279'


def docker_bin():
    explicit = os.environ.get('DOCKER_BIN')
    if explicit:
        return explicit
    return _automatic_docker_bin()


@lru_cache(maxsize=1)
def _automatic_docker_bin():
    native = shutil.which('docker')
    windows = Path('/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe')
    candidates = [native] if native else []
    # An existing WSL socket may be stale. Probe the daemon, not the socket file.
    # Preserve an explicitly selected Docker endpoint/context instead of routing
    # commands to a different engine behind the user's back.
    explicit_endpoint = os.environ.get('DOCKER_HOST') or os.environ.get('DOCKER_CONTEXT')
    if windows.exists() and not explicit_endpoint:
        candidates.append(str(windows))
    for candidate in candidates:
        try:
            result = subprocess.run(
                [candidate, 'info', '--format', '{{.ServerVersion}}'], cwd=ROOT,
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5,
            )
            if result.returncode == 0:
                return candidate
        except (OSError, subprocess.TimeoutExpired):
            continue
    if candidates:
        # Keep offline `compose config` usable. Runtime operations then report
        # Docker's actual daemon error; never start another engine implicitly.
        return candidates[0]
    raise RuntimeError('Falta Docker CLI; iniciar Docker Desktop e integrar WSL o definir DOCKER_BIN.')


def host_path(path):
    path = str(Path(path).resolve())
    if docker_bin().endswith('.exe'):
        return subprocess.check_output(['wslpath', '-w', path], text=True).strip()
    return path


def docker(*args, **kwargs):
    return subprocess.run([docker_bin(), *args], cwd=ROOT, check=True, **kwargs)


def compose(*args, **kwargs):
    return docker('compose', '-f', host_path(BROKER / 'compose.yaml'), *args, **kwargs)


def private_write(path, content):
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w') as stream:
        stream.write(content)
