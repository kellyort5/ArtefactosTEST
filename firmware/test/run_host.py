#!/usr/bin/env python3
"""Build the actual portable firmware logic with the pinned IDF cJSON implementation."""
import os, pathlib, subprocess, tempfile, urllib.request, ctypes.util
root = pathlib.Path(__file__).resolve().parents[1]
cache = pathlib.Path(tempfile.gettempdir()) / 'sentinel-core-host'
cache.mkdir(exist_ok=True)
idf = pathlib.Path.home() / '.platformio/packages/framework-espidf/components/json/cJSON'
if not (idf / 'cJSON.c').exists():
    for name in ('cJSON.c', 'cJSON.h'):
        target = cache / name
        if not target.exists():
            urllib.request.urlretrieve('https://raw.githubusercontent.com/DaveGamble/cJSON/v1.7.19/' + name, target)
    idf = cache
subprocess.run(['python3',str(root/'tools/generate_validator.py'),str(cache/'v2_validator.inc')],check=True)
flags = ['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if os.environ.get('SENTINEL_SANITIZE') == '1' else []
subprocess.run(['cc', *flags, '-std=c99', '-g', '-c', str(idf / 'cJSON.c'), '-o', str(cache / 'cJSON.o')], check=True)
crypto = ctypes.util.find_library('crypto')
if not crypto:
    raise SystemExit('Install the system OpenSSL runtime to run the host digest checks.')
command = ['c++', *flags, '-std=c++17', '-g', '-Wall', '-Wextra', '-Wno-misleading-indentation',
           '-I' + str(idf), '-I'+str(cache), '-I'+str(root/'components/sentinel_api/include'), '-I' + str(root / 'components/sentinel/include'),
           str(root / 'components/sentinel/core.cpp'), str(root/'components/sentinel/protocol_v2.cpp'), str(root/'components/sentinel/module_runtime.cpp'), str(root / 'components/sentinel/wire.cpp'), str(root / 'components/sentinel/digest.cpp'),
           str(root / 'test/core_test.cpp'), str(root/'test/module_consumer.cpp'),str(root/'test/module_test.cpp'), str(cache / 'cJSON.o'), '-l:' + crypto, '-o', str(cache / 'core_test')]
subprocess.run(command, check=True)
subprocess.run([str(cache / 'core_test')], check=True, cwd=root.parent)
print('Firmware portable core: PASS (host; no physical radio/NVS claims)')
