import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createNodeStore} from '../src/store';
import {makeCommand,type StateMessage,type AckMessage} from '@sentinel/contracts';
const fixture=JSON.parse(readFileSync(new URL('../../contracts/fixtures/state.valid.json',import.meta.url),'utf8')) as StateMessage;
function setup(){const store=createNodeStore();store.getState().select(fixture.device_id);store.getState().setLink('ble','connected');return store;}
const command=()=>makeCommand({version:1,device_id:fixture.device_id,boot_id:fixture.boot_id,revision:0,uptime_ms:1000,max_message_bytes:2048},{operation:'set_mode',args:{mode:'cebo'}},'cmd-1');
const ack=(status:'accepted'|'applied'):AckMessage=>({...fixture,type:'ack',payload:{command_id:'cmd-1',status,revision:status==='applied'?1:0,error:null,result:null}});
test('publication/accepted do not imply applied; cross-channel state reconciles',()=>{
 const s=setup();s.getState().receive(fixture,'ble',false,1000);s.getState().track(command(),1000);s.getState().sent('cmd-1');assert.equal(s.getState().commands[0].status,'pending');
 s.getState().receive(ack('accepted'),'mqtt',false,1001);assert.equal(s.getState().commands[0].status,'accepted');
 const applied=structuredClone(fixture);applied.uptime_ms=2000;applied.payload.revision=1;applied.payload.profile='cebo';applied.payload.last_command_id='cmd-1';
 s.getState().receive(applied,'mqtt',false,2000);assert.equal(s.getState().commands[0].status,'applied');
 s.getState().receive(ack('accepted'),'ble',false,2001);assert.equal(s.getState().commands[0].status,'applied');
});
test('retained alone stays stale; silence and disconnect mark old',()=>{
 const s=setup();s.getState().receive(fixture,'mqtt',true,1000);assert.equal(s.getState().stale,true);
 s.getState().receive(fixture,'ble',false,2000);assert.equal(s.getState().stale,false);s.getState().tick(17000);assert.equal(s.getState().stale,true);
 s.getState().receive({...fixture,seq:2,uptime_ms:2100},'ble',false,18000);s.getState().setLink('ble','disconnected');assert.equal(s.getState().stale,true);
});
test('live bootB -> retained old bootA -> live bootB preserves current boot',()=>{
 const s=setup(),b={...fixture,boot_id:'bootB'},a={...fixture,boot_id:'bootA'};
 s.getState().receive(b,'ble',false,1000);s.getState().receive(a,'mqtt',true,1001);assert.equal(s.getState().snapshot?.boot_id,'bootB');assert.equal(s.getState().stale,false);
 s.getState().receive({...b,uptime_ms:2000,seq:2},'ble',false,2000);assert.equal(s.getState().snapshot?.uptime_ms,2000);assert.equal(s.getState().stale,false);
});
test('reboot leaves old command unresolved and old boot cannot return',()=>{
 const s=setup();s.getState().receive(fixture,'ble',false,1000);s.getState().track(command(),1000);
 s.getState().receive({...fixture,boot_id:'new-boot',uptime_ms:10},'ble',false,1100);assert.equal(s.getState().commands[0].status,'sending');
 s.getState().receive({...fixture,uptime_ms:2000},'mqtt',false,1200);assert.equal(s.getState().snapshot?.boot_id,'new-boot');
});
test('store never keeps private command arguments',()=>{
 const s=setup(),c=command();c.payload={command_id:'secret-cmd',target_boot_id:fixture.boot_id,expected_revision:0,expires_at_uptime_ms:31000,operation:'configure_connectivity',args:{ssid:'LAB',wifi_password:'DO-NOT-PERSIST',mqtt_uri:'mqtt://broker.example',mqtt_username:'example',mqtt_password:'DO-NOT-PERSIST'}};
 s.getState().track(c,1);assert.ok(!JSON.stringify(s.getState()).includes('DO-NOT-PERSIST'));assert.ok(!JSON.stringify(s.getState()).includes('EXAMPLE'));
});
test('older revision/uptime and other node rejected',()=>{
 const s=setup(),newer={...fixture,uptime_ms:2000,payload:{...fixture.payload,revision:2}};
 s.getState().receive(newer,'ble',false,1);s.getState().receive(fixture,'mqtt',false,2);s.getState().receive({...newer,device_id:'other'},'mqtt',false,3);assert.equal(s.getState().snapshot?.payload.revision,2);
});
