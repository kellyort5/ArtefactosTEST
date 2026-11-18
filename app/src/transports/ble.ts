import {Capacitor} from '@capacitor/core';
import {BleClient} from '@capacitor-community/bluetooth-le';
import {BLE_UUIDS,BleAssembler,decodeAnyMessage,decodeAnyMetadata,encodeAnyMessage,fragmentMessage,type AnyMessage,type AnyMetadata} from '@sentinel/contracts';
import type {Transport,TransportSink} from './types';
const bytes=(v:DataView)=>new Uint8Array(v.buffer,v.byteOffset,v.byteLength);
export class BleTransport implements Transport {
 readonly channel='ble' as const;
 private deviceId?:string;
 private assembler=new BleAssembler();
 private transferId=0;
 private outgoing:Promise<void>=Promise.resolve();
 private assemblyTimer?:ReturnType<typeof setTimeout>;
 private pollTimer?:ReturnType<typeof setInterval>;
 constructor(private sink:TransportSink){}
 async connect():Promise<AnyMetadata>{
  if(Capacitor.getPlatform()!=='android')throw new Error('BLE requiere la app Android; usa WSS en este navegador.');
  await this.disconnect();this.sink.status('ble','connecting');
  try {
   await BleClient.initialize({androidNeverForLocation:true});
   if(!await BleClient.isEnabled())await BleClient.requestEnable();
   const device=await BleClient.requestDevice({services:[BLE_UUIDS.service]});this.deviceId=device.deviceId;
   await BleClient.connect(device.deviceId,()=>this.lost(),{timeout:15000});
   if(!await BleClient.isBonded(device.deviceId))await BleClient.createBond(device.deviceId,{timeout:120000});
   // Firmware enforces SC+MITM on every protected operation. A bond alone is not security evidence.
   const metadata=await this.readMetadata();
   await BleClient.startNotifications(device.deviceId,BLE_UUIDS.service,BLE_UUIDS.output,value=>{
    try {
     if(!this.assemblyTimer)this.assemblyTimer=setTimeout(()=>{this.assembler.reset();this.assemblyTimer=undefined;this.sink.error('BLE: expiró un mensaje incompleto; vuelve a consultar estado.');},10000);
     const complete=this.assembler.push(bytes(value),performance.now());
     if(complete){clearTimeout(this.assemblyTimer);this.assemblyTimer=undefined;const message=decodeAnyMessage(complete);if(message.device_id!==metadata.device_id)throw new Error('wrong_node');this.sink.message(message,'ble',false);}
    }catch{clearTimeout(this.assemblyTimer);this.assemblyTimer=undefined;this.assembler.reset();this.sink.error('BLE: mensaje inválido descartado; consulta estado o repite recuperación.');}
   });
   this.sink.status('ble','connected');return metadata;
  }catch{await this.disconnect();this.sink.status('ble','error');throw new Error('BLE: revisa permisos de dispositivos cercanos, Bluetooth, ventana física y código de la pantalla local (OLED o serie de pruebas habilitada).');}
 }
 startPolling(refresh:()=>Promise<void>){clearInterval(this.pollTimer);this.pollTimer=setInterval(()=>{void refresh().catch(()=>this.sink.error('BLE: no se pudo refrescar el estado.'));},5000);}
 async readMetadata(){
  if(!this.deviceId)throw new Error('BLE desconectado.');
  const meta=decodeAnyMetadata(bytes(await BleClient.read(this.deviceId,BLE_UUIDS.service,BLE_UUIDS.metadata)));
  this.sink.metadata(meta);return meta;
 }
 async send(message:Extract<AnyMessage,{type:'command'}>){
  // Serialize whole logical transfers, not just individual GATT writes.
  const run=this.outgoing.then(async()=>{
   const id=this.deviceId;if(!id)throw new Error('BLE desconectado.');
   // MTU23 works with all peers, including the default ATT MTU. No large MTU dependency.
   for(const packet of fragmentMessage(encodeAnyMessage(message),++this.transferId>>>0,23)){
    if(this.deviceId!==id)throw new Error('BLE desconectado.');
    await BleClient.write(id,BLE_UUIDS.service,BLE_UUIDS.input,new DataView(packet.buffer,packet.byteOffset,packet.byteLength));
   }
  });
  this.outgoing=run.catch(()=>{});await run;
 }
 private lost(){this.deviceId=undefined;clearTimeout(this.assemblyTimer);clearInterval(this.pollTimer);this.assemblyTimer=undefined;this.assembler.reset();this.sink.status('ble','disconnected');}
 async disconnect(){const id=this.deviceId;this.lost();if(id)await BleClient.disconnect(id).catch(()=>{});}
}
