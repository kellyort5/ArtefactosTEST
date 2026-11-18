// UI integration with a fake BLE boundary only in the test runner. No radio claim.
import {chromium} from 'playwright';
import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const fixture=JSON.parse(readFileSync(new URL('../../contracts/fixtures/v2/states/escudo.valid.json',import.meta.url)));
const catalog=JSON.parse(readFileSync(new URL('../../contracts/fixtures/v2/acks/escudo.networks.valid.json',import.meta.url)));
const browser=await chromium.launch({headless:true});
try{
 const page=await browser.newPage({viewport:{width:390,height:844}}),failures=[];
 page.on('pageerror',e=>failures.push(e.message));
 await page.goto(process.env.SENTINEL_APP_URL??'http://127.0.0.1:5184');
 const panel=page.getByRole('region',{name:'Diagnóstico P1 BLE'});
 await panel.waitFor();assert.equal(await panel.getByRole('button',{name:'Enviar por BLE',exact:true}).isDisabled(),true);
 await page.evaluate(async({fixture,catalog})=>{
  const {nodeStore}=await import('/src/store.ts'),{controller}=await import('/src/controller.ts');
  nodeStore.getState().select(fixture.device_id);nodeStore.getState().setLink('ble','connected');
  nodeStore.getState().receive(fixture,'ble',false,performance.now());
  const meta={version:2,device_id:fixture.device_id,boot_id:fixture.boot_id,revision:fixture.payload.revision,uptime_ms:1000,max_message_bytes:2048};
  controller.ble.readMetadata=async()=>meta;
  window.p1Sent=[];
  controller.ble.send=async command=>{
   window.p1Sent.push(structuredClone(command));
   const ack={...catalog,device_id:command.device_id,boot_id:command.boot_id,message_id:'browser-ack-'+window.p1Sent.length,payload:{...catalog.payload,command_id:command.payload.command_id,revision:meta.revision}};
   await controller.receive(ack,'ble');
  };
 },{fixture,catalog});
 await panel.locator('select').first().selectOption('Catálogo');
 await panel.getByRole('button',{name:'Enviar por BLE',exact:true}).click();
 await panel.getByText(/Envío .*Espera ACK/).waitFor();
 assert.ok((await panel.innerText()).includes('LAB-EXAMPLE'));
 const command=await page.evaluate(()=>window.p1Sent[0]);assert.equal(command.payload.operation,'escudo.networks');
 await panel.getByRole('button',{name:'Reenviar última orden exacta'}).click();
 await page.waitForFunction(()=>window.p1Sent.length===2);
 assert.deepEqual(await page.evaluate(()=>window.p1Sent[1]),command);
 await panel.locator('textarea').fill('{"password":"PRIVATE-TEST-MARKER"}');
 await panel.getByRole('button',{name:'Enviar por BLE',exact:true}).click();
 await panel.getByText('No se pudo enviar.',{exact:false}).waitFor();
 assert.equal(await page.evaluate(()=>window.p1Sent.length),2);
 const leak=await page.evaluate(async()=>{const {nodeStore}=await import('/src/store.ts');return JSON.stringify(nodeStore.getState()).includes('PRIVATE-TEST-MARKER')||localStorage.length>0;});
 assert.equal(leak,false);
 await panel.getByRole('button',{name:'Limpiar diagnóstico'}).click();
 assert.equal(await panel.getByRole('button',{name:'Reenviar última orden exacta'}).isDisabled(),true);
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=window.innerWidth),true);
 assert.deepEqual(failures,[]);
 console.log('PASS P1 Chromium: opt-in, desconexión, catálogo/ACK real del controller (BLE de test), reenvío explícito exacto, sanitización y ancho móvil. Sin evidencia BLE física.');
}finally{await browser.close();}
