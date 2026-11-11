// Real Chromium/IndexedDB smoke check. Contract fixtures are test data, never hardware evidence.
import {chromium} from 'playwright';
import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const event=JSON.parse(readFileSync(new URL('../../contracts/fixtures/event.valid.json',import.meta.url)));
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
 await page.setViewportSize({width:390,height:844});
 await page.screenshot({path:process.env.SENTINEL_SCREENSHOT??'/tmp/sentinel-app-mobile.png',fullPage:true});
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=window.innerWidth),true);
 assert.deepEqual(failures,[]);
 console.log('PASS Chromium: UI sin datos ficticios, BLE web accionable, responsive e IndexedDB256/cursor/gap tras recarga.');
}finally{await browser.close();}
