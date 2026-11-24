import mqtt,{type MqttClient,type IClientOptions} from 'mqtt';
import {decodeAnyMessage,encodeAnyMessage,mqttTopic,type TopicKind} from '@sentinel/contracts';
import {bleOnly,type NodeCommand,type NodeMessage} from '../protocol';
import type {Transport,TransportSink} from './types';
export type MqttSettings={endpoint:string;deviceId:string};
type Connect=(endpoint:string,options:IClientOptions)=>MqttClient;
export function decodeTopicMessage(topic:string,bytes:Uint8Array,device:string):NodeMessage {
 const message=decodeAnyMessage(bytes);
 if(message.device_id!==device||message.type==='command')throw new Error('wrong_node');
 const kind=message.type==='event'?'events':message.type==='ack'?'acks':message.type;
 if(topic!==mqttTopic(message.version,device,kind))throw new Error('wrong_topic');
 return message;
}
export class MqttTransport implements Transport {
 readonly channel='mqtt' as const;
 private client?:MqttClient;private deviceId?:string;
 constructor(private sink:TransportSink,private connectClient:Connect=(url,options)=>mqtt.connect(url,options)){}
 async connect(settings:MqttSettings){
  await this.disconnect();
  const deviceId=settings.deviceId;
  const url=new URL(settings.endpoint);
  if(url.protocol!=='ws:'||url.username||url.password||url.search||url.hash||!url.hostname||url.port==='0')throw new Error('Usa ws://IP-DEL-PC:9001/mqtt, sin credenciales, query ni fragmento en URL.');
  if(!/^[A-Za-z0-9._:-]{1,64}$/.test(deviceId))throw new Error('ID de nodo inválido.');
  this.sink.status('mqtt','connecting');this.deviceId=deviceId;
  const client=this.connectClient(settings.endpoint,{protocolVersion:5,clientId:`app-${crypto.randomUUID()}`,clean:true,properties:{sessionExpiryInterval:0},reconnectPeriod:0,connectTimeout:10000,queueQoSZero:false,resubscribe:false});
  this.client=client;
  client.on('message',(topic,payload,packet)=>{
   if(this.client!==client)return;
   const topics=([1,2] as const).map(v=>mqttTopic(v,deviceId,'availability'));
   if(topics.includes(topic))return;
   try{
    const message=decodeTopicMessage(topic,payload,deviceId);
    if(packet.retain&&message.type!=='state')throw new Error('unexpected_retained');
    this.sink.message(message,'mqtt',packet.retain);
   }catch{this.sink.error('MQTT: mensaje, versión o topic incorrectos; se descartó la entrada.');}
  });
  client.on('close',()=>{
   if(this.client===client)this.sink.status('mqtt','disconnected');
  });
  client.on('error',()=>{if(this.client===client)this.sink.error('MQTT: verifica IP del PC, red, puerto y firewall local.');});
  await new Promise<void>((resolve,reject)=>{
   let finished=false;
   const fail=()=>{
    if(finished)return;finished=true;clearTimeout(timeout);
    client.end(true);
    if(this.client===client){this.client=undefined;this.deviceId=undefined;this.sink.status('mqtt','error');}
    reject(new Error('MQTT/WS: no se pudo conectar o suscribir. Revisa IP del PC, red y firewall local.'));
   };
   const timeout=setTimeout(fail,12000);
   client.once('error',fail);client.once('close',fail);
   client.once('connect',()=>{
    const kinds:TopicKind[]=['state','availability','metrics','events','acks'];
    client.subscribe(([1,2] as const).flatMap(v=>kinds.map(kind=>mqttTopic(v,deviceId,kind))),{qos:1},(error,granted)=>{
     if(error||!granted||!granted.some(g=>g.qos!==128&&g.topic.endsWith('/state'))||this.client!==client){fail();return;}
     if(finished)return;finished=true;clearTimeout(timeout);client.removeListener('error',fail);client.removeListener('close',fail);
     this.sink.status('mqtt','connected');resolve();
    });
   });
  });
 }
 async send(message:NodeCommand){
  if(!this.client?.connected)throw new Error('MQTT desconectado.');
  if(message.device_id!==this.deviceId||bleOnly(message.payload.operation))throw new Error('Esta operación requiere BLE protegido.');
  await this.client.publishAsync(mqttTopic(message.version,message.device_id,'commands'),new TextDecoder().decode(encodeAnyMessage(message)),{qos:1,retain:false,properties:{messageExpiryInterval:30}});
 }
 async disconnect(){
  const client=this.client;this.client=undefined;this.deviceId=undefined;
  if(client){client.end(true);}
  this.sink.status('mqtt','disconnected');
 }
}
