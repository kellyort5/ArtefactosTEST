import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile,readdir} from 'node:fs/promises';
import {decodeMessage,encodeMessage,decodeMetadata,makeCommand,fragmentMessage,BleAssembler} from '../dist/src/index.js';
const fixtures=new URL('../fixtures/',import.meta.url);
// v2 owns its own fixture tree; preserve the v1 corpus as top-level JSON files.
for(const name of (await readdir(fixtures)).filter(name=>name.endsWith('.json'))){
 test(name,async()=>{const json=await readFile(new URL(name,fixtures),'utf8');if(name.includes('.valid.'))assert.deepEqual(decodeMessage(encodeMessage(decodeMessage(json))),JSON.parse(json));else assert.throws(()=>decodeMessage(json));});
}
test('malformed UTF8, JSON and private fields rejected without echo',()=>{
 for(const input of ['{',new Uint8Array([0xff]),' '.repeat(2049)])assert.throws(()=>decodeMessage(input));
});
test('metadata and monotonic command TTL',()=>{
 const m=decodeMetadata(JSON.stringify({version:1,device_id:'node-example',boot_id:'boot-example',revision:3,uptime_ms:1000,max_message_bytes:2048}));
 const command=makeCommand(m,{operation:'get_state',args:{}},'cmd-1',100);
 assert.equal(command.payload.expires_at_uptime_ms,31100);assert.equal(command.payload.expected_revision,3);assert.equal(command.payload.target_boot_id,'boot-example');
 assert.throws(()=>makeCommand(m,{operation:'get_state',args:{}},'cmd-1',15001));
 assert.throws(()=>decodeMetadata(' '.repeat(257)));
});
test('public errors cannot carry private fields',async()=>{
 const ack=JSON.parse(await readFile(new URL('ack.valid.json',fixtures)));
 ack.payload.status='rejected';ack.payload.error='expired';
 assert.equal(decodeMessage(JSON.stringify(ack)).payload.error,'expired');
 ack.payload.args={password:'DO-NOT-ECHO'};
 assert.throws(()=>decodeMessage(JSON.stringify(ack)),e=>!e.message.includes('DO-NOT-ECHO'));
});
test('MTU23 and 2048-byte boundary roundtrip',()=>{
 const bytes=new Uint8Array(2048).map((_,i)=>i%256),packets=fragmentMessage(bytes,0xffffffff),a=new BleAssembler();
 assert.equal(packets.length,171);assert.equal(Math.max(...packets.map(p=>p.length)),20);
 let result;for(const p of packets)result=a.push(p,1);assert.deepEqual(result,bytes);
 assert.throws(()=>fragmentMessage(new Uint8Array(2049),1));
});
test('timeout, disconnect, interleaving, order and bounds discard partial assembly',()=>{
 const packets=fragmentMessage(new Uint8Array(25),5);const a=new BleAssembler();
 a.push(packets[0],0);assert.throws(()=>a.push(packets[1],10000));
 a.push(packets[0],0);a.reset();assert.throws(()=>a.push(packets[1],1));
 a.push(packets[0],0);assert.throws(()=>a.push(packets[2],1));
 a.push(packets[0],0);assert.throws(()=>a.push(fragmentMessage(new Uint8Array(25),6)[1],1));
 const excess=packets[0].slice();new DataView(excess.buffer).setUint16(6,2049,true);assert.throws(()=>a.push(excess,1));
 assert.throws(()=>a.push(new Uint8Array(8),1));
});
test('UTF8 byte bound applies independently from character count',async()=>{
 const cmd=JSON.parse(await readFile(new URL('configure_connectivity.valid.json',fixtures)));
 cmd.payload.args.ssid='é'.repeat(32);assert.throws(()=>decodeMessage(JSON.stringify(cmd)));
 cmd.payload.args.ssid='LAB';cmd.payload.args.ca_certificate='é'.repeat(1500);assert.throws(()=>decodeMessage(JSON.stringify(cmd)));
});

test('inherited and non-string message types never bypass schema lookup',()=>{
 for(const type of ['constructor','__proto__','toString',null,1,{},[]])assert.throws(()=>decodeMessage(JSON.stringify({version:1,type})),e=>e.name==='ContractError'&&e.code==='invalid_message');
});

test('invalid MQTT URI/port and NUL rejected with sanitized codec error',async()=>{
 const cmd=JSON.parse(await readFile(new URL('configure_connectivity.valid.json',fixtures)));
 for(const uri of ['mqtts://broker.example','mqtt://[::::]','mqtt://broker.example:65536','mqtt://broker.example:0','mqtt://user:secret@broker.example']){
  cmd.payload.args.mqtt_uri=uri;assert.throws(()=>decodeMessage(JSON.stringify(cmd)),e=>e.name==='ContractError'&&e.code==='invalid_message'&&!e.message.includes(uri));
 }
 cmd.payload.args.mqtt_uri='mqtt://[::1]:1883';assert.equal(decodeMessage(JSON.stringify(cmd)).type,'command');
 cmd.payload.args.wifi_password='a\0b';assert.throws(()=>decodeMessage(JSON.stringify(cmd)));
});

test('canonical UTC calendar and fractional timestamps match firmware',async()=>{
 const state=JSON.parse(await readFile(new URL('state.valid.json',fixtures)));
 const command=JSON.parse(await readFile(new URL('set_time.valid.json',fixtures)));
 const valid=[
  '0001-01-01T00:00:00Z',
  '9999-12-31T23:59:59Z',
  '2000-02-29T12:34:56Z',
  '2024-02-29T12:34:56.1Z',
  '2026-01-01T00:00:00.12345678901Z',
 ];
 const invalid=[
  '2026-01-01t00:00:00Z', '2026-01-01 00:00:00Z', '2026-01-01T00:00:00z',
  '0000-01-01T00:00:00Z', '10000-01-01T00:00:00Z',
  '2016-12-31T23:59:60Z', '2026-01-01T24:00:00Z',
  '2026-01-01T00:60:00Z', '2026-01-01T00:00:00+00:00',
  '2026-01-01T00:00:00.Z', '2026-01-01T00:00:00.123456789012Z',
  '2026-01-01T00:00:00Z\n', '2026-00-01T00:00:00Z', '2026-13-01T00:00:00Z',
  '2026-01-00T00:00:00Z', '2026-04-31T00:00:00Z',
  '2025-02-29T00:00:00Z', '1900-02-29T00:00:00Z', '2100-02-29T00:00:00Z',
 ];
 for(const timestamp of valid){
  for(const time_quality of ['app_sync','network_sync']){
   state.timestamp=timestamp;state.time_quality=time_quality;
   assert.equal(decodeMessage(encodeMessage(state)).timestamp,timestamp);
  }
  command.payload.args.timestamp=timestamp;
  assert.equal(decodeMessage(encodeMessage(command)).payload.args.timestamp,timestamp);
 }
 for(const timestamp of invalid){
  for(const time_quality of ['app_sync','network_sync']){
   state.timestamp=timestamp;state.time_quality=time_quality;
   assert.throws(()=>decodeMessage(JSON.stringify(state)),e=>e.name==='ContractError'&&e.code==='invalid_message');
  }
  command.payload.args.timestamp=timestamp;
  assert.throws(()=>decodeMessage(JSON.stringify(command)),e=>e.name==='ContractError'&&e.code==='invalid_message');
 }
 state.timestamp=null;state.time_quality='unknown';assert.equal(decodeMessage(encodeMessage(state)).timestamp,null);
});
