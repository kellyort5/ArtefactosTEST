// Real Chromium WS and native TCP against our authorized local lab; no physical-radio claim.
import {chromium} from 'playwright';
import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
import mqtt from 'mqtt';
const lab={node_id:'browser-anonymous-check'};
const fixture=JSON.parse(readFileSync(new URL('../../contracts/fixtures/v2/states/cebo.valid.json',import.meta.url),'utf8'));
fixture.device_id=lab.node_id;
const node=await mqtt.connectAsync('mqtt://127.0.0.1:1883',{protocolVersion:5,clean:true,reconnectPeriod:0,connectTimeout:5000});
const browser=await chromium.launch({headless:true});
try {
 const page=await browser.newPage({viewport:{width:390,height:844}});
 const errors=[];page.on('pageerror',e=>errors.push(e.name));
 await page.goto(process.env.SENTINEL_APP_URL??'http://127.0.0.1:5173');
 const form=page.locator('form').filter({has:page.getByRole('button',{name:'Conectar WS',exact:true})});
 await form.locator('[name="endpoint"]').fill('ws://127.0.0.1:9001/mqtt');
 await form.locator('[name="deviceId"]').fill(lab.node_id);
 await form.getByRole('button',{name:'Conectar WS',exact:true}).click();
 await page.getByText('Canal conectado',{exact:true}).waitFor();
 await node.publishAsync(`sentinel/v2/nodes/${lab.node_id}/state`,JSON.stringify(fixture),{qos:1,retain:false});
 await page.getByRole('heading',{name:lab.node_id,exact:true}).waitFor();
 for(const name of ['ca_certificate','username','password','mqtt_username','mqtt_password'])assert.equal(await page.locator(`[name="${name}"]`).count(),0);
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
 assert.deepEqual(errors,[]);
 await page.getByRole('button',{name:'Desconectar WS',exact:true}).click();
 console.log('PASS real Chromium mobile viewport: anonymous WS without CA or accounts, TCP→WS v2 state, no CA form, no login, disconnect. Fixtures are test data only.');
} catch {
 console.error('FAIL browser MQTT: revisar broker local, UI y conexión WS; detalles privados omitidos.');
 process.exitCode=1;
} finally {await browser.close();await node.endAsync(true);}
