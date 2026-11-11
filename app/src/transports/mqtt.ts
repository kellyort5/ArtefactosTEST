import mqtt,{type MqttClient} from 'mqtt';
import {decodeMessage,encodeMessage,type CommandMessage} from '@sentinel/contracts';
import {PRIVATE_OPERATIONS,type Transport,type TransportSink} from './types';
export type MqttSettings={endpoint:string;deviceId:string;username:string;password:string;role:'operator'|'viewer'};
export class MqttTransport implements Transport {
 readonly channel='mqtt' as const;
 private client?:MqttClient;private deviceId?:string;private role?:'operator'|'viewer';
 constructor(private sink:TransportSink){}
 async connect(settings:MqttSettings){
  await this.disconnect();
  const url=new URL(settings.endpoint);
  if(url.protocol!=='wss:'||url.username||url.password)throw new Error('Usa wss:// con CA y nombre de servidor confiables, sin credenciales en URL.');
  if(!/^[A-Za-z0-9._:-]{1,64}$/.test(settings.deviceId))throw new Error('ID de nodo inválido.');
  const expected=`${settings.role==='operator'?'operator':'viewer'}-${settings.deviceId}`;
  if(settings.username!==expected)throw new Error('La cuenta debe corresponder al rol y nodo autorizado.');
  this.sink.status('mqtt','connecting');this.deviceId=settings.deviceId;this.role=settings.role;
  // mqtt.js keeps this session's credentials in RAM for reconnect. No persistence or offline command queue.
  const client=mqtt.connect(settings.endpoint,{protocolVersion:5,clientId:`app-${crypto.randomUUID()}`,username:settings.username,password:settings.password,clean:true,properties:{sessionExpiryInterval:0},reconnectPeriod:0,connectTimeout:10000,queueQoSZero:false,resubscribe:false});
  this.client=client;
  client.on('message',(topic,payload,packet)=>{
   if(!topic.startsWith(this.prefix()))return;
   const suffix=topic.slice(this.prefix().length);
   if(suffix==='availability')return; // presence alone does not make state fresh
   try{const message=decodeMessage(payload);const kinds:Record<string,string>={state:'state',metrics:'metrics',events:'event',acks:'ack'};
    if(kinds[suffix]!==message.type||message.device_id!==this.deviceId||message.type==='command')throw new Error('wrong_node');
    this.sink.message(message,'mqtt',packet.retain);
   }catch{this.sink.error('MQTT: mensaje inválido o de otro nodo descartado.');}
  });
  client.on('close',()=>{if(this.client===client)this.sink.status('mqtt','disconnected');});
  client.on('error',()=>{this.sink.error('MQTT: verifica credenciales, ACL, CA, SAN y fecha del sistema.');});
  await new Promise<void>((resolve,reject)=>{
   const timeout=setTimeout(()=>fail(),12000);
   const fail=()=>{clearTimeout(timeout);client.end(true);this.sink.status('mqtt','error');reject(new Error('WSS: conexión rechazada; revisa credenciales, CA/SAN y acceso LAN.'));};
   client.once('error',fail);client.once('close',fail);
   client.once('connect',()=>{
    client.subscribe(['state','availability','metrics','events','acks'].map(x=>this.prefix()+x),{qos:1},(error,granted)=>{
     if(error||!granted||granted.some(g=>g.qos===128)){fail();return;}
     clearTimeout(timeout);client.removeListener('error',fail);client.removeListener('close',fail);this.sink.status('mqtt','connected');resolve();
    });
   });
  });
 }
 private prefix(){return `sentinel/v1/nodes/${this.deviceId}/`;}
 async send(message:CommandMessage){
  if(!this.client?.connected)throw new Error('MQTT desconectado.');
  if(this.role!=='operator')throw new Error('Observador: solo lectura.');
  if(message.device_id!==this.deviceId||PRIVATE_OPERATIONS.has(message.payload.operation))throw new Error('Esta operación requiere BLE protegido.');
  // Callback is only a transport receipt. Application status comes from ack/state.
  await this.client.publishAsync(this.prefix()+'commands',new TextDecoder().decode(encodeMessage(message)),{qos:1,retain:false,properties:{messageExpiryInterval:30}});
 }
 async disconnect(){const client=this.client;this.client=undefined;this.deviceId=undefined;this.role=undefined;if(client){client.options.password=undefined;client.options.username=undefined;client.end(true);}this.sink.status('mqtt','disconnected');}
}
