#!/usr/bin/env python3
"""Cross-check the real C++ wire serializer/parser and shared AJV codec without hardware."""
import json, pathlib, subprocess, tempfile, os
repo = pathlib.Path(__file__).resolve().parents[2]
binary = pathlib.Path(tempfile.gettempdir()) / 'sentinel-core-host/core_test'
subprocess.run(['python3', str(repo / 'firmware/test/run_host.py')], cwd=repo, check=True)
env = os.environ.copy()
version = subprocess.check_output(['node', '-p', 'process.versions.node'], env=env, text=True).strip()
if int(version.split('.')[0]) < 24:
    raise SystemExit('Use Node 24 in PATH before checking shared contracts.')
subprocess.run(['npm', 'run', 'build', '-w', 'contracts'], cwd=repo, env=env, check=True)
wire = subprocess.check_output([str(binary), 'wire'], cwd=repo)
node = '''
import {decodeMessage} from '@sentinel/contracts';
let text='';for await(const part of process.stdin)text+=part;
for(const line of text.trim().split('\\n'))decodeMessage(line);
console.log('C++ output -> shared AJV codec: PASS');
'''
subprocess.run(['node', '--input-type=module', '-e', node], input=wire, cwd=repo, env=env, check=True)
for path in sorted((repo / 'contracts/fixtures').glob('*.json')):
    data = json.loads(path.read_text())
    if data.get('type') != 'command':
        continue
    result = subprocess.run([str(binary), 'parse', str(path)], capture_output=True, text=True, cwd=repo)
    assert (result.returncode == 0) == ('.valid.' in path.name), path.name
print('Shared command fixtures -> C++ cJSON parser: PASS')

# Same native predicates compiled into the ESP32; exercise all v2 core fixtures.
checked=0
for path in sorted((repo/'contracts/fixtures/v2').rglob('*.json')):
    if 'notifications' in path.parts:
        continue
    data=json.loads(path.read_text())
    if not data.get('type') and 'max_message_bytes' not in data:
        continue
    result=subprocess.run([str(binary),'v2',str(path)],capture_output=True,cwd=repo)
    assert (result.returncode==0)==('.valid.' in path.name),path
    checked+=1
print(f'Shared v2 fixtures -> native schema/semantic validation: PASS ({checked})')
wire=subprocess.check_output([str(binary),'wire2'],cwd=repo)
subprocess.run(['node','--input-type=module','-e',node.replace('decodeMessage','decodeModuleMessage')],input=wire,cwd=repo,env=env,check=True)
