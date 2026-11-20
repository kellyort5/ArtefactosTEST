import 'fake-indexeddb/auto';
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {decodeAnyMessage} from '@sentinel/contracts';
import {NodeController} from '../src/controller';
import {EventJournal} from '../src/journal';
import {createNodeStore} from '../src/store';
import type {NodeCommand,NodeMetadata,NodeState,NodeAck} from '../src/protocol';
function setup(){
 const state=decodeAnyMessage(readFileSync(new URL('../../contracts/fixtures/v2/states/escudo.valid.json',import.meta.url),'utf8')) as NodeState;
 const store=createNodeStore();store.getState().select(state.device_id);store.getState().setLink('ble','connected');
 store.getState().receive(state,'ble',false,100);
 let meta:NodeMetadata={version:2,device_id:state.device_id,boot_id:state.boot_id,revision:state.payload.revision+2,uptime_ms:2000,max_message_bytes:2048};
 const sent:NodeCommand[]=[];let reads=0;
 const controller=new NodeController(store,new EventJournal('public-control-'+crypto.randomUUID()),()=>({
  ble:{channel:'ble',async connect(){return meta;},async readMetadata(){reads++;return meta;},startPolling(){},async send(m){sent.push(structuredClone(m));},async disconnect(){}},
  mqtt:{channel:'mqtt',async connect(){},async send(){assert.fail('Escudo must never use MQTT');},async disconnect(){}},
 }),()=>100);
 return {controller,store,sent,state,get reads(){return reads;},setMeta:(m:NodeMetadata)=>{meta=m;},meta};
}

test('normal control rejects malformed arguments and absent capabilities without a diagnostic bypass',async()=>{
 const x=setup();
 await assert.rejects(x.controller.send({operation:'escudo.scan',args:{password:'PRIVATE'}} as never,'ble'));
 await assert.rejects(x.controller.send({operation:'escudo.select_target',args:{ssid:'red',bssid:'invalid',channel:1}},'ble'));
 await assert.rejects(x.controller.send({operation:'set_mode',args:{mode:'cebo'}},'ble'));
 x.store.getState().setLink('ble','disconnected');x.store.getState().setLink('mqtt','connected');
 await assert.rejects(x.controller.send({operation:'escudo.scan',args:{}},'ble'));
 assert.equal(x.sent.length,0);assert.ok(!JSON.stringify(x.store.getState()).includes('PRIVATE'));
});
test('normal control refreshes metadata and isolates view observers without retrying rejected mutations',async()=>{
 const x=setup(),seen:NodeAck[]=[];
 const off=x.controller.observeMessages(()=>{throw new Error('view failure');});
 x.controller.observeMessages(m=>{if(m.type==='ack')seen.push(m);});
 const id=await x.controller.send({operation:'escudo.scan',args:{}},'ble');
 assert.equal(x.reads,1);assert.equal(x.sent[0].payload.expected_revision,x.meta.revision);
 assert.equal(x.store.getState().commands[0].status,'pending');
 const command=x.sent[0];
 const ack:NodeAck={version:2,type:'ack',device_id:command.device_id,boot_id:command.boot_id,message_id:'public-ack',seq:100,uptime_ms:3000,timestamp:null,time_quality:'unknown',payload:{command_id:id,status:'rejected',revision:x.meta.revision,error:'busy',result:null}};
 await x.controller.receive(ack,'ble');
 assert.equal(seen.length,1);assert.equal(x.store.getState().commands[0].status,'rejected');assert.equal(x.sent.length,1);off();
});
