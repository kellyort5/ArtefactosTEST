// Contract-driven browser check. The BLE boundary is simulated only in this runner.
import {chromium} from 'playwright';
import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const fixture=path=>JSON.parse(readFileSync(new URL(`../../contracts/fixtures/v2/${path}.valid.json`,import.meta.url)));
const browser=await chromium.launch({headless:true});
try{
 const page=await browser.newPage({viewport:{width:390,height:844}}),failures=[];
 page.on('pageerror',e=>failures.push(e.message));
 await page.goto(process.env.SENTINEL_APP_URL??'http://127.0.0.1:5184');
 await page.getByRole('heading',{name:'Estado del nodo'}).waitFor();
 assert.equal(await page.getByRole('region',{name:'Diagnóstico P1 BLE'}).count(),0);
 await page.evaluate(async({state,catalog})=>{
  const {nodeStore}=await import('/src/store.ts'),{controller}=await import('/src/controller.ts');
  state.payload.capabilities.capture=true;state.payload.capabilities.detectors=true;state.payload.module.target=null;state.payload.module.scan_status='idle';state.payload.module.scan_id=null;
  nodeStore.getState().select(state.device_id);nodeStore.getState().setLink('ble','connected');
  nodeStore.getState().receive(state,'ble',false,performance.now());
  const meta={version:2,device_id:state.device_id,boot_id:state.boot_id,revision:state.payload.revision,uptime_ms:1000,max_message_bytes:2048};
  controller.ble.readMetadata=async()=>meta;window.escudoSent=[];window.rejectSelection=false;
  controller.ble.send=async command=>{
   window.escudoSent.push(structuredClone(command));meta.uptime_ms+=100;let result=null,error=null;
   const op=command.payload.operation,args=command.payload.args;
   if(op==='escudo.scan'){state.payload.module.scan_status='complete';state.payload.module.scan_id='scan-example';result={kind:'job',job_id:command.payload.command_id};meta.revision++;}
   if(op==='escudo.networks'){
    result=structuredClone(catalog.payload.result);
    if(args.after===null){result.networks[0].ssid='';result.more=true;result.next='next-page';}
    else{result.networks[0].ssid={encoding:'hex',value:'ff'};result.networks[0].bssid='02:00:00:00:00:02';}
   }
   if(op==='escudo.select_target'){
    if(window.rejectSelection)error='busy';
    else {state.payload.module.target=args;result={kind:'escudo.target',target:{...args,capture_status:'stopped'}};meta.revision++;}
   }
   if(op==='escudo.capture_start'){state.payload.module.capture_status='running';meta.revision++;}
   if(op==='escudo.capture_stop'){state.payload.module.capture_status='stopped';meta.revision++;}
   state.payload.revision=meta.revision;state.uptime_ms=meta.uptime_ms;
   if(op==='get_state')result={kind:'state',state:structuredClone(state.payload)};
   const ack={...catalog,message_id:'ack-'+window.escudoSent.length,uptime_ms:meta.uptime_ms,seq:window.escudoSent.length,payload:{command_id:command.payload.command_id,status:error?'rejected':'applied',revision:meta.revision,error,result}};
   await controller.receive(ack,'ble');
  };
 },{state:fixture('states/escudo'),catalog:fixture('acks/escudo.networks')});
 await page.getByRole('navigation',{name:'Secciones de módulos'}).getByRole('button',{name:'Escudo Pasivo',exact:true}).click();
 const panel=page.getByRole('region',{name:'Escudo Pasivo',exact:true});
 assert.equal(await panel.getByRole('button',{name:'Iniciar monitoreo'}).isDisabled(),true);
 await panel.getByRole('button',{name:'Escanear redes',exact:true}).click();
 await panel.getByText('Completado',{exact:true}).waitFor();
 await panel.getByRole('button',{name:'Ver redes detectadas'}).click();
 await panel.getByRole('heading',{name:'SSID oculto'}).waitFor();
 await panel.getByRole('button',{name:'Cargar más redes'}).click();
 await panel.getByRole('heading',{name:'SSID hexadecimal: ff'}).waitFor();
 await page.evaluate(()=>{window.rejectSelection=true;});
 await panel.getByRole('button',{name:'Seleccionar 02:00:00:00:00:02'}).click();
 await panel.getByText(/Rechazada por el nodo/).waitFor();
 assert.equal(await panel.getByRole('button',{name:'Iniciar monitoreo'}).isDisabled(),true);
 await page.evaluate(()=>{window.rejectSelection=false;});
 await panel.getByRole('button',{name:'Seleccionar 02:00:00:00:00:02'}).click();
 await panel.getByText(/SSID hexadecimal: ff · 02:00:00:00:00:02 · canal 6/).waitFor();
 await panel.getByRole('button',{name:'Iniciar monitoreo'}).click();
 await panel.getByText('En ejecución',{exact:true}).waitFor();
 await page.evaluate(async({metric,events})=>{const {controller}=await import('/src/controller.ts');metric.uptime_ms=5000;await controller.receive(metric,'ble');for(const [i,e]of events.entries()){e.payload.journal_seq=i+1;await controller.receive(e,'ble');await controller.receive(e,'ble');}},{metric:fixture('metrics/escudo'),events:['deauth_burst','suspicious_disconnect','evil_twin','karma_indication','same-ssid-benign'].map(n=>fixture('events/escudo.'+n))});
 await panel.getByRole('img',{name:/Tramas acumuladas/}).waitFor();
 assert.equal(await panel.locator('tbody tr').count(),4);
 await panel.locator('tbody summary').first().click();
 for(const width of [390,1280]){await page.setViewportSize({width,height:844});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);}
 await panel.getByRole('button',{name:'Detener monitoreo'}).click();
 await panel.getByText('Detenida',{exact:true}).waitFor();
 await page.evaluate(async()=>{const {nodeStore}=await import('/src/store.ts');nodeStore.getState().setLink('ble','disconnected');});
 assert.equal(await panel.getByRole('button',{name:'Escanear redes',exact:true}).isDisabled(),true);
 assert.equal(await panel.locator('tbody tr').count(),4);
 await page.setViewportSize({width:390,height:844});await page.screenshot({path:'/tmp/sentinel-escudo-mobile.png',fullPage:true});
 assert.deepEqual(failures,[]);
 console.log('PASS Escudo: default product screen, scan/pagination, hidden/hex SSID, rejected/applied target, capture, metrics, four detectors, benign duplicate, offline history and mobile/desktop width. Simulated BLE boundary; no physical claim.');
}finally{await browser.close();}
