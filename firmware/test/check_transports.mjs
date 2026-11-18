#!/usr/bin/env node
// Optional host probe: real worker and wolfMQTT; TCP socket/DNS/RTOS are doubles.
import assert from 'node:assert/strict';
import {copyFileSync, cpSync, mkdirSync, readFileSync, readdirSync, writeFileSync} from 'node:fs';
import {join, resolve} from 'node:path';
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
  const archive = resolve(archives,'wolfMQTT.tar.gz');
  assert.equal(sha256(readFileSync(archive)),library.sha256,'Upstream archive digest: '+name);
  return {archive,library};
};
const wolf = dependency('wolfMQTT');
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
    assert.equal(connect.username,undefined); assert.equal(connect.password,undefined);
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
  assert.equal(transcripts.size,5,'Three TCP paths and both reconnect transcripts must run');
  assert.deepEqual(transcripts.get('before-reconnect.hex'),transcripts.get('after-reconnect.hex'));
  console.log('Independent mqtt-packet: MQTT5/expiry0/LWT/QoS1/RAP/retention/stop/replayed payload PASS (5 transcripts)');
}

temporary('sentinel-transports-', build => {
  const vendor = join(build,'vendor'), worker = join(build,'worker'), fixtures = join(build,'fixtures'), output = join(build,'output');
  for (const directory of [vendor,worker,fixtures,output]) mkdirSync(directory);
  run('tar',['-xzf',wolf.archive,'-C',vendor]);
  const wolfRoot = join(vendor,'wolfMQTT-'+wolf.library.version);
  copyFileSync(join(wolfRoot,'wolfmqtt/options.h.in'),join(wolfRoot,'wolfmqtt/options.h'));
  cpSync(join(harness,'include'),join(worker,'include'),{recursive:true});
  // Only temporary copies of active production are used, never the handoff's old copies.
  for (const name of ['mqtt_transport.cpp']) copyFileSync(join(sketch,'src/sentinel',name),join(worker,name));
  for (const name of ['mqtt_transport.h','transport_requirements.h'])
    copyFileSync(join(sketch,'src/sentinel/include',name),join(worker,'include',name));
  writeFileSync(join(fixtures,'retained-command.bin'),mqttPacket.generate({cmd:'publish',qos:1,retain:true,
    topic:'sentinel/v2/nodes/probe/commands',payload:'{"fixture":true}',messageId:73,properties:{}},{protocolVersion:5}));
  const objects = [];
  for (const name of ['mqtt_client','mqtt_packet','mqtt_socket']) {
    const object = join(build,name+'.o'); objects.push(object);
    run('cc',['-g','-O1',...sanitizeFlags,'-DWOLFMQTT_V5','-I'+wolfRoot,'-c',join(wolfRoot,'src',name+'.c'),'-o',object]);
  }
  const binary = join(build,'worker_test');
  run('c++',['-std=c++17','-g','-O1',...sanitizeFlags,'-DWOLFMQTT_V5','-I'+wolfRoot,
    '-I'+join(worker,'include'),'-I'+worker,'-I'+harness,
    join(worker,'mqtt_transport.cpp'),join(harness,'fake_runtime.cpp'),join(harness,'worker_test.cpp'),
    ...objects,'-pthread','-o',binary]);
  run(binary,[fixtures,output]);
  checkPackets(output);
});
console.log('Optional transport host probe: PASS; no physical TCP/radio/RTOS claim.');
