// Real Chromium/IndexedDB smoke check. Contract fixtures are test data, never hardware evidence.
import {chromium} from 'playwright';
import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const event=JSON.parse(readFileSync(new URL('../../contracts/fixtures/event.valid.json',import.meta.url)));
const moduleEvent=JSON.parse(readFileSync(new URL('../../contracts/fixtures/v2/events/escudo.deauth_burst.valid.json',import.meta.url)));
const v2=JSON.parse(readFileSync(new URL('../../contracts/fixtures/v2/states/escudo.valid.json',import.meta.url)));
const browser=await chromium.launch({headless:true});
try{
 const page=await browser.newPage();const failures=[];page.on('pageerror',e=>failures.push(e.message));
 await page.goto(process.env.SENTINEL_APP_URL??'http://127.0.0.1:5173');
 await page.getByRole('heading',{name:'Estado del nodo',exact:true}).waitFor();
 assert.equal(await page.getByText('Esperando un dispositivo').count(),1);
 await page.getByRole('button',{name:'Buscar y conectar BLE'}).click();
 await page.getByRole('alert').filter({hasText:'BLE requiere la app Android'}).waitFor();
 await page.getByRole('button',{name:'Cerrar error'}).click();
 await page.evaluate(async fixture=>{
  const {EventJournal}=await import('/src/journal.ts');const a=new EventJournal('sentinel-browser-check-v1');
  for(let n=1;n<=258;n++)await a.append({...fixture,message_id:`browser-${n}`,payload:{...fixture.payload,journal_seq:n}});
  await a.append({...fixture,message_id:'browser-258',payload:{...fixture.payload,journal_seq:258}});
  await a.confirmPage(fixture.device_id,{next_cursor:10,retained_from:3,retained_to:258,more:true,gap:true},0);
 },event);
 await page.reload();
 const result=await page.evaluate(async id=>{const {EventJournal}=await import('/src/journal.ts');const h=await new EventJournal('sentinel-browser-check-v1').read(id);return {count:h.events.length,cursor:h.cursor,gap:h.gap?.gap,last:h.events.at(-1)?.payload.journal_seq};},event.device_id);
 assert.deepEqual(result,{count:256,cursor:10,gap:true,last:3});
 // Test-only store injection exercises rendered v2 state; no product mocks or radio claims.
 await page.evaluate(async ({fixture,event})=>{
  const {nodeStore}=await import('/src/store.ts');
  fixture.payload.capabilities.capture=true;fixture.payload.capabilities.detectors=true;
  nodeStore.getState().select(fixture.device_id);nodeStore.getState().setLink('ble','connected');
  nodeStore.getState().receive(fixture,'ble',false,performance.now());
  const {EventJournal}=await import('/src/journal.ts');const journal=new EventJournal('sentinel-browser-ui-v2');
  event.payload.origin='simulated';await journal.append(event);await journal.append(event);
  const h=await journal.read(fixture.device_id);nodeStore.getState().setHistory(fixture.device_id,h.events,h.cursor,h.gap);
 },{fixture:v2,event:moduleEvent});
 await page.getByText('Simulado',{exact:true}).waitFor();
 const nav=page.getByRole('navigation',{name:'Secciones de módulos'});
 await nav.getByRole('button',{name:'Escudo Pasivo',exact:true}).click();
 await page.getByRole('region',{name:'Estado de Escudo Pasivo'}).waitFor();
 assert.equal(await nav.getByRole('button',{name:/Auditor y Sanitizador/}).isDisabled(),true);
 assert.equal(await nav.getByRole('button',{name:/Cebo de Red Local/}).isDisabled(),true);
 // Rejected commands stay visible without changing the confirmed profile.
 await page.evaluate(async()=>{
  const {nodeStore}=await import('/src/store.ts');const {makeNodeCommand}=await import('/src/protocol.ts');
  const s=nodeStore.getState(),c=makeNodeCommand(s.metadata,{operation:'set_mode',args:{mode:'idle'}},'browser-rejected');
  s.track(c,performance.now());s.sent(c.payload.command_id);
  s.receive({...c,type:'ack',message_id:'ack-browser',payload:{command_id:c.payload.command_id,status:'rejected',revision:c.payload.expected_revision,error:'not_implemented',result:null}},'ble',false,performance.now());
 });
 await page.getByText('rejected',{exact:true}).waitFor();
 assert.equal(await page.locator('.profile-option.selected b').innerText(),'Escudo');
 const form=page.locator('form').filter({has:page.getByRole('button',{name:'Conectar WS',exact:true})});
 await form.locator('[name="deviceId"]').fill(v2.device_id);
 await form.locator('[name="username"]').fill('invalid-role-for-test');
 await form.locator('[name="password"]').fill('TEST-ONLY-PRIVATE-MARKER');
 await form.getByRole('button',{name:'Conectar WS',exact:true}).click();
 await page.getByRole('alert').filter({hasText:'La cuenta debe corresponder'}).waitFor();
 assert.equal(await form.locator('[name="password"]').inputValue(),'');
 const leaked=await page.evaluate(async()=>{const {nodeStore}=await import('/src/store.ts');return JSON.stringify(nodeStore.getState()).includes('TEST-ONLY-PRIVATE-MARKER')||JSON.stringify(localStorage).includes('TEST-ONLY-PRIVATE-MARKER');});
 assert.equal(leaked,false);
 await page.setViewportSize({width:390,height:844});
 await page.screenshot({path:process.env.SENTINEL_SCREENSHOT??'/tmp/sentinel-app-mobile.png',fullPage:true});
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=window.innerWidth),true);
 assert.deepEqual(failures,[]);
 console.log('PASS Chromium: UI inicial, diagnóstico BLE web, navegación v2/capacidades y rechazo sin éxito falso (fixtures), responsive e IndexedDB256/cursor/gap tras recarga.');
}finally{await browser.close();}
