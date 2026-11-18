import {nodeStore,matchingAck,type NodeStore} from './store';
import type {StoreApi} from 'zustand/vanilla';
import {EventJournal} from './journal';
import {BleTransport} from './transports/ble';
import {MqttTransport,type MqttSettings} from './transports/mqtt';
import type {Channel,Transport,TransportSink} from './transports/types';
import {makeNodeCommand,bleOnly,operationAvailable,eventPage,validateMessage,type NodeRequest,type NodeMessage,type NodeMetadata} from './protocol';
type BleConnection=Transport & {connect():Promise<NodeMetadata>;readMetadata():Promise<NodeMetadata>;startPolling(refresh:()=>Promise<void>):void};
type MqttConnection=Transport & {connect(settings:MqttSettings):Promise<void>};
export type DiagnosticFault='normal'|'boot'|'revision'|'expired';
const diagnosticOperations=new Set(['get_state','set_mode','read_events','escudo.scan','escudo.networks','escudo.select_target','escudo.capture_start','escudo.capture_stop']);
function checkDiagnostic(request:NodeRequest,enabled:boolean){
 if(!enabled||!diagnosticOperations.has(request.operation))throw new Error('Operación de diagnóstico no permitida.');
}
type Factory=(sink:TransportSink)=>{ble:BleConnection;mqtt:MqttConnection};
export class NodeController {
 readonly ble:BleConnection;readonly mqtt:MqttConnection;
 private observers=new Set<(m:NodeMessage,c:Channel)=>void>();
 observeMessages(listener:(m:NodeMessage,c:Channel)=>void){this.observers.add(listener);return()=>{this.observers.delete(listener);};}
 private notify(message:NodeMessage,channel:Channel){for(const listener of this.observers)try{listener(message,channel);}catch{/* Diagnostics must never interrupt production delivery. */}}
 private receiveQueue=Promise.resolve();private serial:Promise<unknown>=Promise.resolve();private sessionSeq=0;
 constructor(readonly store:StoreApi<NodeStore>,readonly journal:EventJournal,
  factory:Factory=sink=>({ble:new BleTransport(sink),mqtt:new MqttTransport(sink)}),private now=()=>performance.now()){
  const sink:TransportSink={
   status:(c,status)=>{
    const before=this.store.getState();before.setLink(c,status);
    if(before.links[c]==='connected'&&status!=='connected'&&before.commands.some(command=>
     !['get_state','read_events'].includes(command.operation)&&['sending','pending','accepted'].includes(command.status)))this.reconcile();
   },error:e=>this.store.getState().setError(e),
   metadata:m=>this.store.getState().noteMetadata(m,this.now()),
   message:(m,c,retained)=>{void this.receive(m,c,retained).catch(()=>this.store.getState().setError('No se pudo validar o guardar el historial. Repite la página; el cursor no avanza.'));},
  };
  const transports=factory(sink);this.ble=transports.ble;this.mqtt=transports.mqtt;
 }
 private reconcile(){
  if(Object.values(this.store.getState().links).includes('connected'))queueMicrotask(()=>{
   void this.send({operation:'get_state',args:{}}).catch(()=>{});
  });
 }
 async loadHistory(){const id=this.store.getState().deviceId;if(!id)return;const h=await this.journal.read(id);this.store.getState().setHistory(id,h.events,h.cursor,h.gap);}
 receive(input:NodeMessage,channel:Channel,retained=false):Promise<void>{
  const run=this.receiveQueue.then(async()=>{
   const message=validateMessage(input),state=this.store.getState();if(message.device_id!==state.deviceId)return;
   if(message.type==='event'){
    if(retained)return;await this.journal.append(message);await this.loadHistory();this.notify(message,channel);return;
   }
   const pending=message.type==='ack'?matchingAck(state,message,retained):undefined;
   if(message.type==='ack'&&pending?.page&&pending.status!=='applied'&&message.payload.status==='applied'){
    const page=eventPage(message);if(!page)return;
    await this.journal.confirmPage(message.device_id,page,pending.afterCursor??0);await this.loadHistory();
    if(page.more&&page.next_cursor>(pending.afterCursor??0))queueMicrotask(()=>{
     const current=this.store.getState();
     if(current.deviceId===message.device_id&&current.metadata?.boot_id===pending.boot)
      void this.send({operation:'read_events',args:{after_cursor:page.next_cursor,limit:8}},channel).catch(()=>this.store.getState().setError('La sincronización sigue pendiente; reconecta y recupera por cursor.'));
    });
   }
   this.store.getState().receive(message,channel,retained,this.now());
   if(!retained)this.notify(message,channel);
   if(message.type==='ack'&&pending?.operation==='set_mode'&&message.payload.status==='rejected')this.reconcile();
  });
  this.receiveQueue=run.catch(()=>{});return run;
 }
 async connectBle(){
  const meta=await this.ble.connect();const s=this.store.getState();
  if(s.deviceId&&s.deviceId!==meta.device_id)await this.mqtt.disconnect();
  this.store.getState().select(meta.device_id);this.store.getState().noteMetadata(meta,this.now());await this.loadHistory();
  await this.send({operation:'get_state',args:{}},'ble');
  this.ble.startPolling(async()=>{await this.send({operation:'get_state',args:{}},'ble');});
  await this.recover('ble');
 }
 async connectMqtt(settings:MqttSettings){
  if(this.store.getState().deviceId&&this.store.getState().deviceId!==settings.deviceId)await this.ble.disconnect();
  this.store.getState().select(settings.deviceId);this.store.getState().setRole(settings.role);await this.loadHistory();await this.mqtt.connect(settings);
 }
 send(request:NodeRequest,preferred?:Channel):Promise<string>{return this.dispatch(request,preferred);}
 sendDiagnostic(request:NodeRequest,fault:DiagnosticFault,enabled:boolean):Promise<string>{
  try{checkDiagnostic(request,enabled);}catch{return Promise.reject(new Error('Operación de diagnóstico no permitida.'));}
  return this.dispatch(request,'ble',fault);
 }
 replayDiagnostic(command:import('./protocol').NodeCommand,enabled:boolean):Promise<string>{
  const operation=this.serial.then(async()=>{
   checkDiagnostic(command.payload as NodeRequest,enabled);
   const message=validateMessage(command);
   if(message.type!=='command'||message.version!==2)throw new Error('Se requiere una orden pública v2.');
   const state=this.store.getState();
   if(state.links.ble!=='connected'||state.deviceId!==message.device_id)throw new Error('Conecta BLE protegido al nodo de la orden.');
   const meta=await this.ble.readMetadata();
   if(this.store.getState().deviceId!==state.deviceId||meta.device_id!==state.deviceId||meta.boot_id!==message.boot_id||
      meta.version!==2||meta.uptime_ms>=message.payload.expires_at_uptime_ms)throw new Error('La orden ya no pertenece al arranque/plazo actual. Envía una nueva.');
   // Keep the original ID, sequence, envelope and args. Do not retrack duplicates.
   await this.ble.send(message);return message.payload.command_id;
  });
  this.serial=operation.catch(()=>{});return operation;
 }
 private dispatch(request:NodeRequest,preferred?:Channel,diagnostic?:DiagnosticFault):Promise<string>{
  const requestedNode=this.store.getState().deviceId;
  const requestedBoot=this.store.getState().metadata?.boot_id;
  const operation=this.serial.then(async()=>{
   let s=this.store.getState();
   if(s.deviceId!==requestedNode)throw new Error('El nodo seleccionado cambió; repite la operación.');
   const channel=preferred??(s.links.ble==='connected'?'ble':'mqtt');
   if(s.links[channel]!=='connected')throw new Error('Conecta un canal autorizado.');
   if(bleOnly(request.operation)&&channel!=='ble')throw new Error('Usa BLE protegido para esta operación.');
   if(channel==='mqtt'&&s.role==='viewer')throw new Error('Observador: solo lectura.');
   let metadata=s.metadata,metadataAt=s.metadataAt;
   if(channel==='ble'){
    metadata=await this.ble.readMetadata();metadataAt=this.now();
    if(this.store.getState().deviceId!==requestedNode||metadata.device_id!==requestedNode)throw new Error('El nodo conectado cambió; vuelve a conectar.');
    this.store.getState().noteMetadata(metadata,metadataAt);s=this.store.getState();
   }else if(s.stale||s.snapshot?.payload.profile!=='cebo')throw new Error('Espera un estado vivo de Cebo antes de enviar por MQTT.');
   if(!metadata)throw new Error('Consulta el estado del nodo antes de enviar órdenes.');
   if(requestedBoot&&metadata.boot_id!==requestedBoot&&!['get_state','read_events'].includes(request.operation))throw new Error('El nodo reinició; confirma su estado y repite la operación.');
   if(diagnostic===undefined&&!['get_state','read_events','set_time','configure_connectivity'].includes(request.operation)&&
      (s.snapshot?.boot_id!==metadata.boot_id||s.snapshot?.version!==metadata.version))throw new Error('Consulta el estado del arranque actual antes de controlar el módulo.');
   if(diagnostic===undefined&&!operationAvailable(s.snapshot,request))throw new Error('Esta función no está disponible en el estado/capacidades actuales.');
   const message=makeNodeCommand(metadata,request,crypto.randomUUID(),Math.floor(this.now()-metadataAt));message.seq=++this.sessionSeq;
   if(diagnostic!==undefined){
    if(message.version!==2)throw new Error('El diagnóstico P1 requiere contrato v2.');
    if(diagnostic==='boot'){message.boot_id='boot-p1-invalid';message.payload.target_boot_id='boot-p1-invalid';}
    if(diagnostic==='revision')message.payload.expected_revision=metadata.revision+1;
    if(diagnostic==='expired'){message.uptime_ms=0;message.payload.expires_at_uptime_ms=1;}
    validateMessage(message);
   }
   this.store.getState().track(message,this.now());
   try{await (channel==='ble'?this.ble:this.mqtt).send(message);this.store.getState().sent(message.payload.command_id);}
   catch{
    this.store.getState().uncertain(message.payload.command_id);
    const other=channel==='ble'?'mqtt':'ble';
    if(this.store.getState().links[other]==='connected')queueMicrotask(()=>{void this.send({operation:'get_state',args:{}},other).catch(()=>{});});
    throw new Error('Envío interrumpido. Resultado pendiente hasta recibir confirmación o consultar estado.');
   }
   return message.payload.command_id;
  });
  this.serial=operation.catch(()=>{});return operation;
 }
 async recover(channel?:Channel){await this.send({operation:'read_events',args:{after_cursor:this.store.getState().cursor,limit:8}},channel);}
 async disconnectAll(){await Promise.all([this.ble.disconnect(),this.mqtt.disconnect()]);this.store.getState().setError(null);}
}
export const controller=new NodeController(nodeStore,new EventJournal());
export const ble=controller.ble,mqtt=controller.mqtt;
export const loadHistory=()=>controller.loadHistory();
export const connectBle=()=>controller.connectBle();
export const connectMqtt=(settings:MqttSettings)=>controller.connectMqtt(settings);
export const send=(request:NodeRequest,preferred?:Channel)=>controller.send(request,preferred);
export const recover=(channel?:Channel)=>controller.recover(channel);
export const disconnectAll=()=>controller.disconnectAll();
export type {PageResult} from './protocol';
