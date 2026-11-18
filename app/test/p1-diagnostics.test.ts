import 'fake-indexeddb/auto';
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {decodeAnyMessage,encodeAnyMessage} from '@sentinel/contracts';
import {NodeController,type DiagnosticFault} from '../src/controller';
import {EventJournal} from '../src/journal';
import {createNodeStore} from '../src/store';
import {p1ToolsEnabled,publicP1Request} from '../src/P1Diagnostics';
import type {NodeCommand,NodeMetadata,NodeState,NodeAck} from '../src/protocol';
function setup(){
 const state=decodeAnyMessage(readFileSync(new URL('../../contracts/fixtures/v2/states/escudo.valid.json',import.meta.url),'utf8')) as NodeState;
 const store=createNodeStore();store.getState().select(state.device_id);store.getState().setLink('ble','connected');
 store.getState().receive(state,'ble',false,100);
 let meta:NodeMetadata={version:2,device_id:state.device_id,boot_id:state.boot_id,revision:state.payload.revision+2,uptime_ms:2000,max_message_bytes:2048};
 const sent:NodeCommand[]=[];let reads=0;
 const controller=new NodeController(store,new EventJournal('p1-'+crypto.randomUUID()),()=>({
  ble:{channel:'ble',async connect(){return meta;},async readMetadata(){reads++;return meta;},startPolling(){},async send(m){sent.push(structuredClone(m));},async disconnect(){}},
  mqtt:{channel:'mqtt',async connect(){},async send(){assert.fail('diagnostics must never use MQTT');},async disconnect(){}},
 }),()=>100);
 return {controller,store,sent,state,get reads(){return reads;},setMeta:(m:NodeMetadata)=>{meta=m;},meta};
}
test('P1 default off, disconnected, private and invalid args never reach wire',async()=>{
 assert.equal(p1ToolsEnabled,false);
 const x=setup();
 await assert.rejects(x.controller.sendDiagnostic({operation:'get_state',args:{}},'normal',false));
 await assert.rejects(x.controller.sendDiagnostic({operation:'configure_connectivity',args:{}} as never,'normal',true));
 assert.throws(()=>publicP1Request('configure_connectivity','{}'));
 assert.throws(()=>publicP1Request('escudo.scan','password secret'));
 await assert.rejects(x.controller.sendDiagnostic({operation:'escudo.scan',args:{password:'PRIVATE'}} as never,'normal',true));
 x.store.getState().setLink('ble','disconnected');x.store.getState().setLink('mqtt','connected');
 await assert.rejects(x.controller.sendDiagnostic({operation:'get_state',args:{}},'normal',true));
 assert.equal(x.sent.length,0);assert.ok(!JSON.stringify(x.store.getState()).includes('PRIVATE'));
});
test('P1 shares fresh metadata, real ACK/store path, observer failure isolation and exact explicit replay',async()=>{
 const x=setup();const seen:NodeAck[]=[];
 const off=x.controller.observeMessages(()=>{throw new Error('diagnostic observer error');});
 x.controller.observeMessages(m=>{if(m.type==='ack')seen.push(m);});
 const id=await x.controller.sendDiagnostic({operation:'escudo.scan',args:{}},'normal',true);
 assert.equal(x.reads,1);assert.equal(x.sent[0].payload.expected_revision,x.meta.revision);
 assert.equal(x.store.getState().commands[0].status,'pending');
 const command=x.sent[0];
 const ack:NodeAck={version:2,type:'ack',device_id:command.device_id,boot_id:command.boot_id,message_id:'p1-ack',seq:100,uptime_ms:3000,timestamp:null,time_quality:'unknown',payload:{command_id:id,status:'applied',revision:x.meta.revision,error:null,result:null}};
 // scan has an asynchronous scan result; use a real schema-valid rejection for gate observation.
 ack.payload.status='rejected';ack.payload.error='busy';
 await x.controller.receive(ack,'ble');assert.equal(seen.length,1);assert.equal(x.store.getState().commands[0].status,'rejected');
 assert.equal(x.sent.length,1); // no automatic modification retry
 await x.controller.replayDiagnostic(command,true);
 assert.deepEqual(encodeAnyMessage(x.sent[1]),encodeAnyMessage(command));assert.equal(x.store.getState().commands.length,1);
 x.setMeta({...x.meta,boot_id:'new-boot'});await assert.rejects(x.controller.replayDiagnostic(command,true));
 x.setMeta({...x.meta,uptime_ms:40000});await assert.rejects(x.controller.replayDiagnostic(command,true));
 x.store.getState().setLink('ble','disconnected');await assert.rejects(x.controller.replayDiagnostic(command,true));
 assert.equal(x.sent.length,2);off();
});
test('negative presets remain valid v2 envelopes and do not affect normal product commands',async()=>{
 for(const fault of ['boot','revision','expired'] as DiagnosticFault[]){
  const x=setup();await x.controller.sendDiagnostic({operation:'set_mode',args:{mode:'idle'}},fault,true);
  const m=x.sent[0];decodeAnyMessage(encodeAnyMessage(m));
  if(fault==='boot')assert.notEqual(m.boot_id,x.meta.boot_id);
  if(fault==='revision')assert.notEqual(m.payload.expected_revision,x.meta.revision);
  if(fault==='expired'&&m.version===2)assert.equal(m.payload.expires_at_uptime_ms,1);
 }
 const x=setup();await x.controller.send({operation:'get_state',args:{}},'ble');
 assert.equal(x.sent[0].boot_id,x.meta.boot_id);assert.equal(x.sent[0].payload.expected_revision,x.meta.revision);
 await assert.rejects(x.controller.sendDiagnostic({operation:'escudo.select_target',args:{ssid:'red',bssid:'invalid',channel:1}},'normal',true));
 assert.equal(x.sent.length,1);
});
