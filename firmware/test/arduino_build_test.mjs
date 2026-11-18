#!/usr/bin/env node
// Host regression checks exercise actual module factories and SDK rejection guards.
// A preprocessor test is not a firmware build or physical hardware evidence.
import assert from 'node:assert/strict';
import {writeFileSync} from 'node:fs';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';
import {sketch, run, capture, cjsonSources, temporary} from './host_support.mjs';
const cjson = await cjsonSources();
temporary('sentinel-module-flags-', build => {
  writeFileSync(join(build,'esp_err.h'),'#pragma once\ntypedef int esp_err_t;\n');
  writeFileSync(join(build,'main.cpp'),'namespace sentinel { void register_optional_modules(); }\n'
    + 'int main() { sentinel::register_optional_modules(); }\n');
  const base = ['-std=c++17','-I'+build,'-I'+cjson,
    '-I'+join(sketch,'src/sentinel/include'),'-I'+join(sketch,'src/sentinel_api/include')];
  const modes = ['ESCUDO','SANITIZADOR','CEBO'];
  for (const enabled of [[],['ESCUDO'],['SANITIZADOR'],['CEBO'],modes]) {
    const flags = modes.map(mode => '-DSENTINEL_MODULE_'+mode+'='+Number(enabled.includes(mode)));
    const object = join(build,'optional_modules.o');
    run('c++',[...base,...flags,'-c',join(sketch,'src/sentinel/optional_modules.cpp'),'-o',object]);
    const symbols = capture('nm',['-u',object]);
    for (const mode of modes) assert.equal(symbols.includes('sentinel_'+mode.toLowerCase()+'_module'),
      enabled.includes(mode),`factory ${mode}; enabled=${enabled}`);
    const linked = spawnSync('c++',[object,join(build,'main.cpp'),'-o',join(build,'module_flags')],{encoding:'utf8'});
    assert.ifError(linked.error);
    assert.equal(linked.status === 0,enabled.length === 0,'Enabled factories must exist at link time');
  }
  const invalid = spawnSync('c++',[...base,'-DSENTINEL_MODULE_CEBO=2','-c',
    join(sketch,'src/sentinel/optional_modules.cpp'),'-o',join(build,'invalid.o')],{encoding:'utf8'});
  assert.ifError(invalid.error);
  assert.notEqual(invalid.status,0);
  assert.match(invalid.stderr,/flags must be 0 or 1/);
});
temporary('sentinel-display-flag-', directory => {
  const source = join(directory,'display.cpp');
  writeFileSync(source,'#include "'+join(sketch,'SentinelConfig.h')+'"\n');
  const counter = join(directory,'counter.cpp');
  writeFileSync(counter,'#include "'+join(sketch,'src/sentinel/include/test_diagnostics.h')+'"\n#include <cassert>\nint main(){ assert(sentinel::detector_inputs()==0); sentinel::note_detector_input(); sentinel::note_detector_input(); assert(sentinel::detector_inputs()==2*SENTINEL_SERIAL_TEST_DISPLAY); }\n');
  for (const value of [0,1]) {
    const binary=join(directory,'counter-'+value);
    run('c++',['-std=c++17','-DSENTINEL_SERIAL_TEST_DISPLAY='+value,counter,'-o',binary]);run(binary);
  }
  for (const value of [0,1,2]) {
    const result = spawnSync('c++',['-E','-DSENTINEL_SERIAL_TEST_DISPLAY='+value,source],{encoding:'utf8'});
    assert.equal(result.status===0,value!==2,'Explicit boolean test display flag');
  }
});
console.log('Module value flags, disabled factory symbols and missing enabled factories: PASS');

const required = {
  IDF_TARGET_ESP32S3:1, ESPTOOLPY_FLASHSIZE_16MB:1,
  SPIRAM:1, SPIRAM_MODE_OCT:1, SPIRAM_SPEED_80M:1,
  BT_ENABLED:1, BT_NIMBLE_ENABLED:1, BT_NIMBLE_SM_SC:1,
  BT_NIMBLE_MAX_BONDS:3, BT_NIMBLE_NVS_PERSIST:1,
  MBEDTLS_SSL_PROTO_TLS1_2:1,
  LWIP_IP_FORWARD:1, LWIP_IPV4_NAPT:1,
};
temporary('sentinel-sdk-guards-', directory => {
  function check(values, {version = [3,3,12], partition = true} = {}) {
    writeFileSync(join(directory,'sdkconfig.h'),Object.entries(values)
      .map(([k,v]) => '#define CONFIG_'+k+' '+v+'\n').join(''));
    writeFileSync(join(directory,'esp_arduino_version.h'),['MAJOR','MINOR','PATCH']
      .map((name,i) => '#define ESP_ARDUINO_VERSION_'+name+' '+version[i]+'\n').join(''));
    const result = spawnSync('c++',['-E','-I'+directory, ...(partition ? ['-DARDUINO_PARTITION_sentinel'] : []),
      join(sketch,'src/sentinel/arduino_requirements.cpp')],{encoding:'utf8'});
    assert.ifError(result.error);
    return result.status;
  }
  assert.equal(check(required),0,'Secure complete SDK configuration');
  for (const key of Object.keys(required)) {
    const without = {...required}; delete without[key];
    assert.notEqual(check(without),0,'Missing required SDK property: '+key);
  }
  assert.equal(check({...required, BT_NIMBLE_SM_SC_ONLY:0, BT_NIMBLE_SM_LVL:0,
    BT_NIMBLE_SM_LEGACY:1, BT_NIMBLE_MAX_CONNECTIONS:3, MBEDTLS_HAVE_TIME_DATE:0}),0,
    'Official defaults are permitted; adapter SC/MITM/session checks remain mandatory at runtime');
  for (const key of ['ESP_PHY_INIT_DATA_IN_PARTITION', 'BT_NIMBLE_SM_SC_DEBUG_KEYS',
    'ESP_TLS_INSECURE','ESP_TLS_SKIP_SERVER_CERT_VERIFY'])
    assert.notEqual(check({...required,[key]:1}),0,'Insecure SDK property: '+key);
  for (const bonds of [0,1,2,4]) assert.notEqual(check({...required,BT_NIMBLE_MAX_BONDS:bonds}),0,
    'Only the verified 3-bond official storage layout is accepted');
  assert.notEqual(check(required,{partition:false}),0,'Standard USB Sentinel partition option is mandatory');
  for (const version of [[2,3,12],[3,2,12],[3,3,11]]) assert.notEqual(check(required,{version}),0,
    'Unverified core version: '+version.join('.'));
});
console.log('Official core/version/partition/SDK rejection guards and accepted defaults: PASS (preprocessor only)');

temporary('sentinel-transport-guards-', directory => {
  writeFileSync(join(directory,'wolfMQTT.h'),'#pragma once\n');
  const required = {WOLFMQTT_V5:1};
  function check(values) {
    const result = spawnSync('c++',['-E','-x','c++','-I'+directory,
      ...Object.entries(values).map(([k,v]) => '-D'+k+'='+v),
      '-include',join(sketch,'src/sentinel/include/transport_requirements.h'),'-'],{input:'',encoding:'utf8'});
    assert.ifError(result.error);
    return result.status;
  }
  assert.equal(check(required),0,'MQTT5 with the external Arduino TCP client');
  for (const key of Object.keys(required)) {
    const without = {...required}; delete without[key];
    assert.notEqual(check(without),0,'Missing transport requirement: '+key);
  }
  for (const key of ['ENABLE_MQTT_TLS'])
    for (const value of [0,1]) assert.notEqual(check({...required,[key]:value}),0,'Forbidden transport option: '+key);

});
console.log('MQTT5 and external TCP transport guards: PASS (preprocessor only)');
