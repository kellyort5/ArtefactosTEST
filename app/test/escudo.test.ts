import 'fake-indexeddb/auto';
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {decodeAnyMessage,type V2AckMessage} from '@sentinel/contracts';
import {NodeController} from '../src/controller';
import {EventJournal} from '../src/journal';
import {createNodeStore} from '../src/store';
import {createEscudoSession,ssidLabel,detections} from '../src/features/escudo/model';
import type {NodeCommand,NodeMessage} from '../src/protocol';
const fixture=(path:string)=>decodeAnyMessage(readFileSync(new URL(`../../contracts/fixtures/v2/${path}.valid.json`,import.meta.url),'utf8'));
function setup(){
 const state=fixture('states/escudo');if(state.type!=='state'||state.version!==2)throw Error();
 state.payload.capabilities.capture=true;
 const store=createNodeStore();store.getState().select(state.device_id);store.getState().setLink('ble','connected');store.getState().receive(state,'ble',false,100);
 const meta={version:2 as const,device_id:state.device_id,boot_id:state.boot_id,revision:state.payload.revision,uptime_ms:2000,max_message_bytes:2048},sent:NodeCommand[]=[];
 const controller=new NodeController(store,new EventJournal('escudo-'+crypto.randomUUID()),()=>({ble:{channel:'ble',async connect(){return meta;},async readMetadata(){return meta;},startPolling(){},async send(m){sent.push(m);},async disconnect(){}},mqtt:{channel:'mqtt',async connect(){},async send(){assert.fail('Escudo must use BLE');},async disconnect(){}}}),()=>100);
 const session=createEscudoSession(controller);const stop=session.start();
 function ack(id:string){const a=fixture('acks/escudo.networks') as V2AckMessage;a.payload.command_id=id;return a;}
 return {state,store,controller,meta,sent,session,stop,ack};
}
test('catalog pages use matching ACK/scan/cursor and never infer malicious duplicate SSIDs',async()=>{
 const x=setup();try{
 const id=await x.controller.send({operation:'escudo.networks',args:{after:null,limit:4}},'ble');
 const bad=x.ack('unknown');await x.controller.receive(bad,'ble');assert.equal(x.session.view.getState().loaded,false);
 const a=x.ack(id);if(a.payload.result?.kind!=='escudo.networks')throw Error();a.payload.result.more=true;a.payload.result.next='page-2';
 await x.controller.receive(a,'ble');assert.equal(x.session.view.getState().networks.length,1);
 const id2=await x.controller.send({operation:'escudo.networks',args:{after:'page-2',limit:4}},'ble');
 const b=x.ack(id2);if(b.payload.result?.kind!=='escudo.networks')throw Error();b.payload.result.networks[0].bssid='02:00:00:00:00:02';
 await x.controller.receive(b,'ble');await x.controller.receive(b,'ble');assert.equal(x.session.view.getState().networks.length,2);assert.equal(x.session.view.getState().next,null);
 const benign=fixture('events/escudo.same-ssid-benign');await x.controller.receive(benign,'ble');assert.equal(detections(x.store.getState().events).length,0);
 }finally{x.stop();}
});
test('late catalog across disconnect, other scan, wrong channel and rejected ACK are not shown',async()=>{
 const x=setup();try{
 const id=await x.controller.send({operation:'escudo.networks',args:{after:null,limit:4}},'ble');
 x.store.getState().setLink('ble','disconnected');x.store.getState().setLink('ble','connected');
 await x.controller.receive(x.ack(id),'ble');assert.equal(x.session.view.getState().loaded,false);
 const id2=await x.controller.send({operation:'escudo.networks',args:{after:null,limit:4}},'ble');
 const other=x.ack(id2);if(other.payload.result?.kind!=='escudo.networks')throw Error();other.payload.result.scan_id='other-scan';
 await x.controller.receive(other,'ble');assert.equal(x.session.view.getState().loaded,false);
 const id3=await x.controller.send({operation:'escudo.networks',args:{after:null,limit:4}},'ble');
 await x.controller.receive(x.ack(id3),'mqtt');assert.equal(x.session.view.getState().loaded,false);
 const id4=await x.controller.send({operation:'escudo.networks',args:{after:null,limit:4}},'ble');
 const rejected=x.ack(id4);rejected.payload.status='rejected';rejected.payload.error='busy';rejected.payload.result=null;
 await x.controller.receive(rejected,'ble');assert.equal(x.session.view.getState().loaded,false);assert.equal(x.store.getState().commands.at(-1)?.status,'rejected');
 }finally{x.stop();}
});
test('metric history is bounded, BLE only, deduplicated and scoped to current boot',async()=>{
 const x=setup();try{
 for(let i=0;i<65;i++){
 const m=fixture('metrics/escudo');m.message_id='metric-'+i;m.seq=i+1;m.uptime_ms=3000+i;
 await x.controller.receive(m,'ble');await x.controller.receive(m,'ble');
 }
 assert.equal(x.session.view.getState().samples.length,60);
 const mqtt=fixture('metrics/escudo');mqtt.message_id='mqtt-metric';mqtt.uptime_ms=4000;await x.controller.receive(mqtt,'mqtt');assert.notEqual(x.session.view.getState().samples.at(-1)?.id,'mqtt-metric');
 x.store.getState().noteMetadata({...x.meta,boot_id:'boot-new'},101);assert.equal(x.session.view.getState().samples.length,0);
 }finally{x.stop();}
});
test('all four detectors survive replay once; benign catalogs and SSID encoding stay distinct',async()=>{
 const x=setup();try{
 const names=['deauth_burst','suspicious_disconnect','evil_twin','karma_indication'];
 for(const [i,n] of names.entries()){const e=fixture('events/escudo.'+n);if(e.type!=='event')throw Error();e.payload.journal_seq=i+1;await x.controller.receive(e,'ble');await x.controller.receive(e,'ble');}
 assert.equal(detections(x.store.getState().events).length,4);
 assert.equal(ssidLabel(''),'SSID oculto');assert.equal(ssidLabel({encoding:'hex',value:'ff'}),'SSID hexadecimal: ff');
 const id=await x.controller.send({operation:'read_events',args:{after_cursor:0,limit:8}},'ble');
 const base=x.ack(id);base.payload.result={kind:'events',next_cursor:4,retained_from:1,retained_to:4,more:false,gap:true};
 await x.controller.receive(base,'ble');assert.equal(x.store.getState().cursor,4);assert.equal(x.store.getState().gap?.gap,true);
 }finally{x.stop();}
});
test('USB reset archives history atomically and restarts cursor; ordinary reopen keeps it',async()=>{
 const x=setup();try{
 const event=fixture('events/escudo.deauth_burst');if(event.type!=='event')throw Error();await x.controller.receive(event,'ble');
 await x.controller.journal.confirmPage(event.device_id,{next_cursor:1,retained_from:1,retained_to:1,more:false,gap:false},0);await x.controller.loadHistory();
 await assert.rejects(()=>x.controller.restartHistoryAfterUsb());assert.equal(x.store.getState().cursor,1);
 x.store.getState().setLink('ble','disconnected');await x.controller.restartHistoryAfterUsb();
 assert.equal(x.store.getState().cursor,0);assert.equal(x.store.getState().events.length,0);
 const archives=await x.controller.journal.archives(event.device_id);assert.equal(archives.length,1);assert.equal(archives[0].events[0].message_id,event.message_id);
 const fresh={...event,boot_id:'new-boot',message_id:'new-event'} as NodeMessage;
 await x.controller.receive(fresh,'ble');assert.equal(x.store.getState().events.length,1);assert.equal(x.store.getState().events[0].boot_id,'new-boot');
 assert.equal((await x.controller.journal.archives(event.device_id)).length,1);
 }finally{x.stop();}
});
test('empty catalog is a confirmed empty result, not a pending request',async()=>{
 const x=setup();try{
 const id=await x.controller.send({operation:'escudo.networks',args:{after:null,limit:4}},'ble');
 assert.equal(x.store.getState().commands.at(-1)?.status,'pending');assert.equal(x.session.view.getState().loaded,false);
 const a=x.ack(id);if(a.payload.result?.kind!=='escudo.networks')throw Error();a.payload.result.networks=[];
 await x.controller.receive(a,'ble');assert.equal(x.session.view.getState().loaded,true);assert.deepEqual(x.session.view.getState().networks,[]);
 }finally{x.stop();}
});
test('upgrading an existing v1 phone database preserves events and cursor',async()=>{
 const name='upgrade-'+crypto.randomUUID(),event=fixture('events/escudo.deauth_burst');if(event.type!=='event')throw Error();
 const db=await new Promise<IDBDatabase>((resolve,reject)=>{const r=indexedDB.open(name,1);r.onupgradeneeded=()=>{r.result.createObjectStore('events',{keyPath:['device_id','message_id']}).createIndex('node','device_id');r.result.createObjectStore('cursors',{keyPath:'device_id'});};r.onsuccess=()=>resolve(r.result);r.onerror=()=>reject(r.error);});
 await new Promise<void>((resolve,reject)=>{const tx=db.transaction(['events','cursors'],'readwrite');tx.objectStore('events').put(event);tx.objectStore('cursors').put({device_id:event.device_id,cursor:1,gap:null});tx.oncomplete=()=>resolve();tx.onerror=()=>reject(tx.error);});db.close();
 const upgraded=new EventJournal(name),history=await upgraded.read(event.device_id);
 assert.equal(history.cursor,1);assert.equal(history.events[0].message_id,event.message_id);assert.deepEqual(await upgraded.archives(event.device_id),[]);
});
