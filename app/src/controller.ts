import {makeCommand,type CommandRequest,type Message,type PageResult} from '@sentinel/contracts';
import {nodeStore} from './store';
import {EventJournal} from './journal';
import {BleTransport} from './transports/ble';
import {MqttTransport,type MqttSettings} from './transports/mqtt';
import {PRIVATE_OPERATIONS,type Channel,type TransportSink} from './transports/types';
const journal=new EventJournal();
let receiveQueue=Promise.resolve();
let serial:Promise<unknown>=Promise.resolve();
let sessionSeq=0;
const sink:TransportSink={
 status:(c,s)=>nodeStore.getState().setLink(c,s),error:e=>nodeStore.getState().setError(e),metadata:m=>nodeStore.getState().noteMetadata(m,performance.now()),
 message:(m,c,retained)=>{receiveQueue=receiveQueue.then(()=>receive(m,c,retained)).catch(()=>nodeStore.getState().setError('No se pudo guardar/recuperar el historial. Repite la página; el cursor no avanza.'));},
};
export const ble=new BleTransport(sink);
export const mqtt=new MqttTransport(sink);
export async function loadHistory(){const id=nodeStore.getState().deviceId;if(!id)return;const h=await journal.read(id);nodeStore.getState().setHistory(id,h.events,h.cursor,h.gap);}
async function receive(message:Message,channel:Channel,retained:boolean){
 const state=nodeStore.getState();if(message.device_id!==state.deviceId)return;
 if(message.type==='event'){await journal.append(message);await loadHistory();return;}
 const pending=message.type==='ack'?state.commands.find(c=>c.id===message.payload.command_id):undefined;
 // Persist completed pages only after preceding event writes in this queue.
 if(message.type==='ack'&&message.payload.status==='applied'&&message.payload.result&&pending?.page&&pending.boot===message.boot_id){
  await journal.confirmPage(message.device_id,message.payload.result,pending.afterCursor??0);await loadHistory();
  const page=message.payload.result;
  if(page.more&&page.next_cursor>state.cursor)setTimeout(()=>{void send({operation:'read_events',args:{after_cursor:page.next_cursor,limit:8}},channel).catch(()=>{});},0);
 }
 nodeStore.getState().receive(message,channel,retained,performance.now());
}
export async function connectBle(){
 const meta=await ble.connect();const s=nodeStore.getState();
 if(s.deviceId&&s.deviceId!==meta.device_id){await mqtt.disconnect();}
 s.select(meta.device_id);nodeStore.getState().noteMetadata(meta,performance.now());await loadHistory();
 await send({operation:'get_state',args:{}},'ble');
 ble.startPolling(async()=>{await send({operation:'get_state',args:{}},'ble');});
 await recover('ble');
}
export async function connectMqtt(settings:MqttSettings){
 const s=nodeStore.getState();if(s.deviceId&&s.deviceId!==settings.deviceId)await ble.disconnect();
 s.select(settings.deviceId);nodeStore.getState().setRole(settings.role);await loadHistory();await mqtt.connect(settings);
 // Wait for a non-retained state (periodic every5s) before issuing fresh commands.
}
export function send(request:CommandRequest,preferred?:Channel):Promise<string>{
 const operation=serial.then(async()=>{
  const s=nodeStore.getState();
  const channel=preferred??(s.links.ble==='connected'?'ble':'mqtt');
  if(s.links[channel]!=='connected')throw new Error('Conecta un canal autorizado.');
  if(PRIVATE_OPERATIONS.has(request.operation)&&channel!=='ble')throw new Error('Usa BLE protegido para configurar o sincronizar hora.');
  if(channel==='mqtt'&&s.role==='viewer')throw new Error('Observador: solo lectura.');
  let metadata=s.metadata,metadataAt=s.metadataAt;
  if(channel==='ble'){metadata=await ble.readMetadata();metadataAt=performance.now();}
  else if(s.stale)throw new Error('Estado antiguo: espera una actualización en vivo del nodo.');
  if(!metadata)throw new Error('Consulta el estado del nodo antes de enviar órdenes.');
  const message=makeCommand(metadata,request,crypto.randomUUID(),Math.floor(performance.now()-metadataAt));
  message.seq=++sessionSeq;
  nodeStore.getState().track(message,performance.now());
  try{await (channel==='ble'?ble:mqtt).send(message);nodeStore.getState().sent(message.payload.command_id);}
  catch{nodeStore.getState().uncertain(message.payload.command_id);throw new Error('Envío interrumpido. Consulta estado por un canal disponible; no se confirma aplicación.');}
  return message.payload.command_id;
 });
 serial=operation.catch(()=>{});return operation;
}
export async function recover(channel?:Channel){await send({operation:'read_events',args:{after_cursor:nodeStore.getState().cursor,limit:8}},channel);}
export async function disconnectAll(){await Promise.all([ble.disconnect(),mqtt.disconnect()]);nodeStore.getState().setError(null);}
export type {PageResult};
