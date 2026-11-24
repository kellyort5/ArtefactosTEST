import 'fake-indexeddb/auto';
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {decodeAnyMessage,decodeAnyMetadata,encodeAnyMessage} from '@sentinel/contracts';
import {makeNodeCommand,modeAvailable,type NodeState,type NodeEvent,type NodeCommand,type NodeAck,type NodeMetadata} from '../src/protocol';
import {createNodeStore} from '../src/store';
import {NodeController} from '../src/controller';
import {EventJournal} from '../src/journal';
function fixture(path:string){return decodeAnyMessage(readFileSync(new URL('../../contracts/fixtures/'+path,import.meta.url),'utf8'));}
function state(profile='escudo'):NodeState {
 const s=fixture(`v2/states/${profile}.valid.json`) as NodeState;
 if(s.version===2){s.payload.capabilities.capture=true;s.payload.capabilities.detectors=true;}
 return s;
}
function metadata(s:NodeState):NodeMetadata{return {version:s.version,device_id:s.device_id,boot_id:s.boot_id,revision:s.payload.revision,uptime_ms:s.uptime_ms,max_message_bytes:2048};}
function acknowledgement(c:NodeCommand,result:any=null,status:'accepted'|'applied'|'rejected'='applied'):NodeAck {
 return {version:c.version,type:'ack',device_id:c.device_id,boot_id:c.boot_id,message_id:`ack-${c.payload.command_id}`,seq:10,uptime_ms:1100,time_quality:'unknown',timestamp:null,
  payload:{command_id:c.payload.command_id,status,revision:c.payload.expected_revision+1,error:status==='rejected'?'not_implemented':null,result}} as NodeAck;
}
function setup(s=state()) {
 const store=createNodeStore();store.getState().select(s.device_id);store.getState().setLink('ble','connected');store.getState().receive(s,'ble',false,100);
 return store;
}
function controllerSetup(s=state()) {
 const store=setup(s),journal=new EventJournal(`controller-${crypto.randomUUID()}`),sent:NodeCommand[]=[];
 let meta=metadata(s),reject=false,clock=100;
 const controller=new NodeController(store,journal,sink=>({
  ble:{channel:'ble',async connect(){return meta;},async readMetadata(){sink.metadata(meta);return meta;},startPolling(){},async send(m){sent.push(m);if(reject)throw new Error('interrupted');},async disconnect(){sink.status('ble','disconnected');}},
  mqtt:{channel:'mqtt',async connect(){sink.status('mqtt','connected');},async send(m){sent.push(m);},async disconnect(){sink.status('mqtt','disconnected');}},
 }),()=>clock);
 return {store,journal,controller,sent,setNow:(value:number)=>{clock=value;},setMetadata:(m:NodeMetadata)=>{meta=m;},fail:()=>{reject=true;}};
}
test('negotiate v1 then v2 from metadata without changing shared wire APIs',()=>{
 const legacy=fixture('state.valid.json') as NodeState;
 assert.equal(makeNodeCommand(metadata(legacy),{operation:'get_state',args:{}},'legacy').version,1);
 assert.throws(()=>makeNodeCommand(metadata(legacy),{operation:'escudo.scan',args:{}},'wrong-version'));
 const current=state();current.boot_id='current-boot';
 const store=setup(legacy);store.getState().noteMetadata(decodeAnyMetadata(JSON.stringify(metadata(current))),101);
 store.getState().receive(current,'ble',false,102);
 assert.equal(store.getState().snapshot?.version,2);
 assert.equal(makeNodeCommand(metadata(current),{operation:'escudo.scan',args:{}},'functional').version,2);
 assert.throws(()=>decodeAnyMetadata(JSON.stringify({...metadata(current),version:3})));
});
test('retained state never negotiates, confirms or overwrites a live version',()=>{
 const current=state();const store=createNodeStore();store.getState().select(current.device_id);store.getState().setLink('mqtt','connected');
 store.getState().receive(current,'mqtt',true,100);
 assert.equal(store.getState().metadata,null);assert.equal(store.getState().stale,true);
 store.getState().receive(current,'mqtt',false,101);
 const old=fixture('state.valid.json') as NodeState;old.boot_id='old-boot';
 store.getState().receive(old,'mqtt',true,102);
 assert.equal(store.getState().snapshot?.version,2);assert.equal(store.getState().stale,false);
});
test('reject false success from wrong ACK kind, retained ACK and rejected mode',()=>{
 const s=state(),store=setup(s);
 const c=makeNodeCommand(metadata(s),{operation:'set_mode',args:{mode:'cebo'}},'mode-command');store.getState().track(c,100);store.getState().sent(c.payload.command_id);
 const wrong=fixture('v2/acks/events.valid.json') as NodeAck;wrong.payload.command_id=c.payload.command_id;
 store.getState().receive(wrong,'ble',false,101);assert.equal(store.getState().commands[0].status,'pending');
 store.getState().receive(acknowledgement(c),'mqtt',true,102);assert.equal(store.getState().commands[0].status,'pending');
 store.getState().receive(acknowledgement(c,null,'rejected'),'ble',false,103);
 assert.equal(store.getState().commands[0].status,'rejected');assert.equal(store.getState().snapshot?.payload.profile,'escudo');
 store.getState().receive(acknowledgement(c),'ble',false,104);assert.equal(store.getState().commands[0].status,'rejected');
});
test('new boot invalidates old confirmations and wrong state cannot reconcile a mode',()=>{
 const s=state(),store=setup(s),c=makeNodeCommand(metadata(s),{operation:'set_mode',args:{mode:'cebo'}},'pending-mode');store.getState().track(c,100);
 const wrong=structuredClone(s);wrong.payload.revision++;wrong.payload.last_command_id=c.payload.command_id;wrong.uptime_ms++;
 store.getState().receive(wrong,'ble',false,101);assert.equal(store.getState().commands[0].status,'sending');
 const reboot=structuredClone(s);reboot.boot_id='boot-new';reboot.uptime_ms=10;
 store.getState().noteMetadata(metadata(reboot),102);store.getState().receive(reboot,'ble',false,103);
 store.getState().receive(acknowledgement(c),'ble',false,104);
 assert.equal(store.getState().commands[0].status,'sending');assert.equal(store.getState().snapshot?.boot_id,'boot-new');
});
test('correlated inline get_state is consumed; typed page data is not assumed for every result',()=>{
 const s=state(),store=setup(s),c=makeNodeCommand(metadata(s),{operation:'get_state',args:{}},'state-query');store.getState().track(c,100);
 const next=structuredClone(s);next.payload.revision++;next.uptime_ms++;
 store.getState().receive(acknowledgement(c,{kind:'state',state:next.payload}),'ble',false,101);
 assert.equal(store.getState().commands[0].status,'applied');assert.equal(store.getState().snapshot?.payload.revision,next.payload.revision);
});
test('private v2 configurations never enter store correlation or command history',()=>{
 const s=state(),store=setup(s);
 const c=makeNodeCommand(metadata(s),{operation:'sanitizador.configure_upstream',args:{ssid:'LAB',wifi_password:'PRIVATE-MARKER'}},'secret');
 store.getState().track(c,100);assert.equal(store.getState().commands[0].correlation,null);
 assert.ok(!JSON.stringify(store.getState()).includes('PRIVATE-MARKER'));
});
test('controller sends negotiated codec and keeps interrupted command pending',async()=>{
 const {controller,sent,store,fail}=controllerSetup();
 await controller.send({operation:'get_state',args:{}});
 assert.equal(sent[0].version,2);assert.equal(store.getState().commands[0].status,'pending');
 fail();await assert.rejects(()=>controller.send({operation:'set_mode',args:{mode:'idle'}}));
 assert.equal(store.getState().commands.at(-1)?.status,'pending');
});
test('controller requires live Cebo for anonymous MQTT and BLE-only module commands',async()=>{
 const {controller,store,setNow}=controllerSetup(state('cebo'));
 store.getState().setLink('ble','disconnected');store.getState().setLink('mqtt','connected');
 store.getState().receive(state('cebo'),'mqtt',true,102);
 store.getState().tick(20000);await assert.rejects(()=>controller.send({operation:'get_state',args:{}},'mqtt'));
 setNow(20001);store.getState().receive({...state('cebo'),uptime_ms:2000},'mqtt',false,20001);
 await controller.send({operation:'get_state',args:{}},'mqtt');
 await assert.rejects(()=>controller.send({operation:'escudo.scan',args:{}},'mqtt'));
});
test('incomplete or incorrectly correlated page does not advance the durable cursor',async()=>{
 const {controller,sent,store,journal}=controllerSetup();
 await controller.send({operation:'read_events',args:{after_cursor:0,limit:2}});
 const c=sent[0];const two=fixture('v2/events/escudo.deauth_burst.valid.json') as NodeEvent;
 const wrong=acknowledgement(c,{kind:'events',next_cursor:8,retained_from:1,retained_to:8,more:false,gap:true});
 wrong.boot_id='old-boot';
 await controller.receive(wrong,'ble');assert.equal(store.getState().commands[0].status,'pending');
 assert.equal((await journal.read(c.device_id)).cursor,0);
 two.message_id='event-2';two.payload.journal_seq=2;
 await controller.receive(two,'ble');
 const ack=acknowledgement(c,{kind:'events',next_cursor:2,retained_from:1,retained_to:2,more:false,gap:false});
 await assert.rejects(()=>controller.receive(ack,'ble'));assert.equal((await journal.read(two.device_id)).cursor,0);
 assert.equal(store.getState().commands[0].status,'pending');
 const one=structuredClone(two);one.message_id='event-1';one.payload.journal_seq=1;
 await controller.receive(one,'ble');await controller.receive(ack,'ble');
 assert.equal((await journal.read(two.device_id)).cursor,2);assert.equal(store.getState().commands[0].status,'applied');
 await controller.receive(two,'ble');assert.equal((await journal.read(two.device_id)).events.length,2);
});
test('controller cancels an intent when device selection changes during metadata read',async()=>{
 const {controller,store,sent}=controllerSetup();
 let release!:(value:NodeMetadata)=>void;
 controller.ble.readMetadata=()=>new Promise(resolve=>{release=resolve;});
 const pending=controller.send({operation:'get_state',args:{}},'ble');
 await new Promise(resolve=>setImmediate(resolve));
 const other={...metadata(state()),device_id:'another-node'};store.getState().select(other.device_id);release(other);
 await assert.rejects(()=>pending,/cambió/);assert.equal(sent.length,0);
});
test('rejected mode and link loss query state without retrying the mutation',async()=>{
 const {controller,store,sent}=controllerSetup(state('cebo'));
 await controller.send({operation:'set_mode',args:{mode:'idle'}},'ble');
 await controller.receive(acknowledgement(sent[0],null,'rejected'),'ble');
 await new Promise(resolve=>setImmediate(resolve));
 assert.deepEqual(sent.map(c=>c.payload.operation),['set_mode','get_state']);
 assert.equal(store.getState().commands[0].status,'rejected');
 await controller.send({operation:'set_mode',args:{mode:'idle'}},'ble');
 store.getState().setLink('mqtt','connected');await controller.ble.disconnect();
 await new Promise(resolve=>setImmediate(resolve));
 assert.deepEqual(sent.map(c=>c.payload.operation),['set_mode','get_state','set_mode','get_state']);
 assert.equal(store.getState().snapshot?.payload.profile,'cebo');
});
test('metrics for old boot cannot replace current module metrics or make retained state fresh',()=>{
 const s=state(),store=setup(s);const metric=fixture('v2/metrics/escudo.valid.json');
 store.getState().receive(metric,'ble',false,101);assert.ok(store.getState().metrics.escudo);
 store.getState().receive({...metric,boot_id:'old-boot'},'mqtt',false,102);
 assert.equal(store.getState().metrics.escudo.boot_id,s.boot_id);
});
test('navigation gates absent capabilities and v1 Sanitizador without inventing success',()=>{
 const idle=state('idle');if(idle.version===2){idle.payload.capabilities.capture=false;idle.payload.capabilities.detectors=false;}
 assert.equal(modeAvailable(idle,'escudo'),false);assert.equal(modeAvailable(idle,'cebo'),false);assert.equal(modeAvailable(idle,'sanitizador'),false);
 assert.equal(modeAvailable(idle,'idle'),true);assert.equal(modeAvailable(fixture('state.valid.json') as NodeState,'sanitizador'),false);
});
