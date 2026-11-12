#!/usr/bin/env python3
"""Focused tests of the real Escudo component + portable supervisor/wire (no radio claims)."""
import ctypes.util, os, pathlib, subprocess, tempfile
repo=pathlib.Path(__file__).resolve().parents[3]
fw=repo/'firmware'
cache=pathlib.Path(tempfile.gettempdir())/'sentinel-escudo-host'
cache.mkdir(exist_ok=True)
idf=pathlib.Path.home()/'.platformio/packages/framework-espidf/components/json/cJSON'
if not (idf/'cJSON.c').exists():
    raise SystemExit('Prepare the pinned PlatformIO/ESP-IDF environment before running these tests.')
flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if os.environ.get('SENTINEL_SANITIZE')=='1' else []
subprocess.run(['python3',str(fw/'tools/generate_validator.py'),str(cache/'v2_validator.inc')],check=True)
subprocess.run(['cc',*flags,'-std=c99','-g','-c',str(idf/'cJSON.c'),'-o',str(cache/'cJSON.o')],check=True)
includes=[idf,cache,fw/'components/sentinel/include',fw/'components/sentinel_api/include',fw/'components/escudo/include']
sources=[fw/'components/sentinel'/s for s in ('core.cpp','wire.cpp','digest.cpp','module_runtime.cpp','protocol_v2.cpp')]
sources += [fw/'components/escudo'/s for s in ('discovery.cpp','module.cpp')]
crypto=ctypes.util.find_library('crypto')
if not crypto:raise SystemExit('Install the system OpenSSL runtime for native command digests.')
binary=cache/'discovery_test'
subprocess.run(['c++',*flags,'-std=c++17','-g','-Wall','-Wextra','-Wno-misleading-indentation',*['-I'+str(p) for p in includes],*[str(s) for s in sources],str(fw/'test/escudo/discovery_test.cpp'),str(cache/'cJSON.o'),'-l:'+crypto,'-o',str(binary)],check=True)
subprocess.run([str(binary)],cwd=repo,check=True)
