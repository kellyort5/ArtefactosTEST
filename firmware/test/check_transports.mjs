#!/usr/bin/env node
// Optional host probe: real worker, wolfMQTT and BearSSL; TLS socket/DNS/RTOS are doubles.
import assert from 'node:assert/strict';
import {copyFileSync, cpSync, mkdirSync, readFileSync, readdirSync, writeFileSync} from 'node:fs';
import {join, dirname, resolve} from 'node:path';
import {createRequire} from 'node:module';
import {repo, sketch, run, sha256, temporary, sanitizeFlags} from './host_support.mjs';

const args = process.argv.slice(2);
assert(args.length === 0 || args.length === 2 && args[0] === '--archives', 'Use --archives <cached upstream archive directory>');
const archives = args[1] ?? process.env.SENTINEL_TRANSPORT_ARCHIVES;
assert(archives, 'Set SENTINEL_TRANSPORT_ARCHIVES or use --archives; no dependencies are downloaded by this optional test.');
const manifest = JSON.parse(readFileSync(join(repo,'firmware/arduino/dependencies.json'),'utf8'));
const dependency = name => {
  const library = manifest.libraries.find(item => item.name === name);
  assert(library,'Missing maintained dependency: '+name);
  const archive = resolve(archives,name === 'wolfMQTT' ? 'wolfMQTT.tar.gz' : 'ESP_SSLClient.zip');
  assert.equal(sha256(readFileSync(archive)),library.sha256,'Upstream archive digest: '+name);
  return {archive,library};
};
const wolf = dependency('wolfMQTT'), ssl = dependency('ESP_SSLClient');
const require = createRequire(import.meta.url);
assert.equal(require('mqtt-packet/package.json').version,'9.0.2','Use the existing pinned independent MQTT packet decoder');
const mqttPacket = require('mqtt-packet');
const harness = join(repo,'firmware/test/transports');

function checkPackets(output) {
  const transcripts = new Map();
  for (const name of readdirSync(output).filter(s => s.endsWith('.hex'))) {
    const packets = [], parser = mqttPacket.parser({protocolVersion:5});
    parser.on('packet',packet => packets.push(packet));
    parser.on('error',error => { throw error; });
    for (const line of readFileSync(join(output,name),'utf8').trim().split('\n')) parser.parse(Buffer.from(line,'hex'));
    const connect = packets.find(p => p.cmd === 'connect');
    assert(connect,name+': CONNECT');
    assert.equal(connect.protocolVersion,5); assert.equal(connect.clean,true);
    assert.equal(connect.properties.sessionExpiryInterval,0);
    assert.equal(connect.will.qos,1); assert.equal(connect.will.retain,true);
    const subscribe = packets.find(p => p.cmd === 'subscribe');
    assert(subscribe,name+': SUBSCRIBE');
    assert.equal(subscribe.subscriptions[0].qos,1);
    assert.equal(subscribe.subscriptions[0].rap,true); assert.equal(subscribe.subscriptions[0].rh,0);
    const publish = packets.find(p => p.cmd === 'publish');
    assert(publish,name+': queued PUBLISH'); assert.equal(publish.qos,1);
    assert.equal(publish.topic,'sentinel/v2/nodes/probe/events');
    assert.equal(JSON.parse(publish.payload).stable_event,name.includes('reconnect') ? 'replay-1' : 'fixture-1');
    if (name !== 'before-reconnect.hex') {
      const disconnect = packets.find(p => p.cmd === 'disconnect');
      assert(disconnect,name+': DISCONNECT');
      assert.equal(disconnect.reasonCode,4); assert.equal(disconnect.properties.sessionExpiryInterval,0);
    }
    transcripts.set(name,publish.payload);
  }
  assert.equal(transcripts.size,7,'Five valid paths and both reconnect transcripts must run');
  assert.deepEqual(transcripts.get('before-reconnect.hex'),transcripts.get('after-reconnect.hex'));
  console.log('Independent mqtt-packet: MQTT5/expiry0/LWT/QoS1/RAP/retention/stop/replayed payload PASS (7 transcripts)');
}

temporary('sentinel-transports-', build => {
  const vendor = join(build,'vendor'), worker = join(build,'worker'), fixtures = join(build,'fixtures'), output = join(build,'output');
  for (const directory of [vendor,worker,fixtures,output]) mkdirSync(directory);
  run('tar',['-xzf',wolf.archive,'-C',vendor]);
  run('unzip',['-q',ssl.archive,'-d',vendor]);
  const wolfRoot = join(vendor,'wolfMQTT-'+wolf.library.version), bssl = join(vendor,'ESP_SSLClient/src/bssl');
  copyFileSync(join(wolfRoot,'wolfmqtt/options.h.in'),join(wolfRoot,'wolfmqtt/options.h'));
  cpSync(join(harness,'include'),join(worker,'include'),{recursive:true});
  // Only temporary copies of active production are used, never the handoff's old copies.
  for (const name of ['mqtt_transport.cpp','tls_verification.cpp']) copyFileSync(join(sketch,'src/sentinel',name),join(worker,name));
  for (const name of ['mqtt_transport.h','tls_verification.h','transport_requirements.h'])
    copyFileSync(join(sketch,'src/sentinel/include',name),join(worker,'include',name));
  for (const name of readdirSync(join(harness,'fixtures'),{recursive:true}).filter(s => s.endsWith('.pem'))) {
    const source = join(harness,'fixtures',name), pem = readFileSync(source,'utf8');
    assert(/^(?:-----BEGIN CERTIFICATE-----\r?\n[A-Za-z0-9+/=\r\n]+-----END CERTIFICATE-----\r?\n?)+$/.test(pem),name+': public certificates only');
    const target = join(fixtures,name); mkdirSync(dirname(target),{recursive:true}); copyFileSync(source,target);
    // The peer-chain double consumes DER, while Git retains text public certificates only.
    if (!name.endsWith('mixed-ca.pem')) run('openssl',['x509','-in',source,'-outform','DER','-out',target.replace(/\.pem$/,'.der')]);
  }
  writeFileSync(join(fixtures,'retained-command.bin'),mqttPacket.generate({cmd:'publish',qos:1,retain:true,
    topic:'sentinel/v2/nodes/probe/commands',payload:'{"fixture":true}',messageId:73,properties:{}},{protocolVersion:5}));
  // Original maintained engine, as in the handoff; no edits or instrumented crypto substitute.
  const engine = join(build,'libbearssl.so');
  run('cc',['-shared','-fPIC','-O1','-Werror=implicit-function-declaration',
    '-I'+join(harness,'bearssl-stubs'),'-I'+bssl,
    ...readdirSync(bssl).filter(s => s.endsWith('.c')).map(s => join(bssl,s)),'-o',engine]);
  const objects = [];
  for (const name of ['mqtt_client','mqtt_packet','mqtt_socket']) {
    const object = join(build,name+'.o'); objects.push(object);
    run('cc',['-g','-O1',...sanitizeFlags,'-DWOLFMQTT_V5','-I'+wolfRoot,'-c',join(wolfRoot,'src',name+'.c'),'-o',object]);
  }
  const binary = join(build,'worker_test');
  run('c++',['-std=c++17','-g','-O1',...sanitizeFlags,'-DWOLFMQTT_V5','-I'+wolfRoot,
    '-I'+join(worker,'include'),'-I'+worker,'-I'+harness,'-I'+bssl,
    join(worker,'mqtt_transport.cpp'),join(worker,'tls_verification.cpp'),join(harness,'fake_runtime.cpp'),join(harness,'worker_test.cpp'),
    ...objects,'-L'+build,'-Wl,-rpath,'+build,'-lbearssl','-lcrypto','-pthread','-o',binary]);
  run(binary,[fixtures,output]);
  checkPackets(output);
});
console.log('Optional transport host probe: PASS; no physical TLS/radio/RTOS claim.');
