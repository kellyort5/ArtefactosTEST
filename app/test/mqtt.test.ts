import {test} from 'node:test';
import assert from 'node:assert/strict';
import {EventEmitter} from 'node:events';
import {readFileSync} from 'node:fs';
import type {IClientOptions,MqttClient} from 'mqtt';
import {mqttTopic,encodeAnyMessage} from '@sentinel/contracts';
import {MqttTransport} from '../src/transports/mqtt';
import {makeNodeCommand,type NodeMessage,type NodeState} from '../src/protocol';

const fixture=(path:string)=>JSON.parse(readFileSync(new URL(`../../contracts/fixtures/${path}`,import.meta.url),'utf8'));
const state=fixture('v2/states/cebo.valid.json') as NodeState;
const settings={endpoint:'ws://broker.example:9001/mqtt',deviceId:state.device_id,username:`operator-${state.device_id}`,password:'TEST-ONLY',role:'operator' as const};
class Client extends EventEmitter {
 connected=true;options:IClientOptions={};topics:string[]=[];published:{topic:string;payload:string;options:unknown}[]=[];
 constructor(private legacyOnly=false,private denied=false){super();}
 subscribe(topics:string[],_options:unknown,callback:Function){this.topics=topics;callback(null,topics.map(topic=>({topic,qos:this.denied||this.legacyOnly&&topic.startsWith('sentinel/v2/')?128:1})));}
 async publishAsync(topic:string,payload:string,options:unknown){this.published.push({topic,payload,options});}
 end(){this.connected=false;this.emit('close');}
}
function setup(legacyOnly=false,denied=false){
 const client=new Client(legacyOnly,denied),messages:NodeMessage[]=[],errors:string[]=[],statuses:string[]=[];
 const transport=new MqttTransport({message:m=>messages.push(m),metadata(){},error:e=>errors.push(e),status:(_,s)=>statuses.push(s)},(_url,options)=>{
  client.options=options;queueMicrotask(()=>client.emit('connect'));return client as unknown as MqttClient;
 });
 return {client,transport,messages,errors,statuses};
}
test('MQTT subscribes both versions, matches exact topics and rejects retained ACKs',async()=>{
 const {client,transport,messages,errors}=setup();await transport.connect({...settings});
 assert.equal(client.topics.length,10);
 for(const version of [1,2] as const)assert.ok(client.topics.includes(mqttTopic(version,state.device_id,'state')));
 client.emit('message',mqttTopic(2,state.device_id,'state'),encodeAnyMessage(state),{retain:false});
 client.emit('message',mqttTopic(1,state.device_id,'state'),encodeAnyMessage(state),{retain:false});
 const ack=fixture('v2/acks/events.valid.json');
 client.emit('message',mqttTopic(2,state.device_id,'acks'),encodeAnyMessage(ack),{retain:true});
 assert.equal(messages.length,1);assert.equal(messages[0].version,2);assert.equal(errors.length,2);
 const command=makeNodeCommand({version:2,device_id:state.device_id,boot_id:state.boot_id,revision:state.payload.revision,uptime_ms:state.uptime_ms,max_message_bytes:2048},{operation:'get_state',args:{}},'mqtt-command');
 await transport.send(command);assert.equal(client.published[0].topic,mqttTopic(2,state.device_id,'commands'));
 assert.equal(JSON.parse(client.published[0].payload).version,2);
 assert.deepEqual(client.published[0].options,{qos:1,retain:false,properties:{messageExpiryInterval:30}});
 client.emit('close');assert.equal(client.options.password,undefined);assert.equal(client.options.username,undefined);
 await transport.disconnect();assert.equal(client.options.password,undefined);assert.equal(client.options.username,undefined);
});
test('v1-only ACL remains usable while denied connections wipe session credentials',async()=>{
 const legacy=setup(true);await legacy.transport.connect({...settings});
 assert.ok(legacy.statuses.includes('connected'));await legacy.transport.disconnect();
 const denied=setup(false,true);await assert.rejects(()=>denied.transport.connect({...settings}));
 assert.equal(denied.client.options.password,undefined);assert.equal(denied.client.options.username,undefined);
 assert.equal(denied.statuses.at(-1),'error');
});
test('MQTT viewer and private module operations cannot publish',async()=>{
 const {client,transport}=setup();await transport.connect({...settings,username:`viewer-${state.device_id}`,role:'viewer'});
 const meta={version:2 as const,device_id:state.device_id,boot_id:state.boot_id,revision:state.payload.revision,uptime_ms:state.uptime_ms,max_message_bytes:2048 as const};
 await assert.rejects(()=>transport.send(makeNodeCommand(meta,{operation:'get_state',args:{}},'viewer')));
 await transport.connect({...settings});
 await assert.rejects(()=>transport.send(makeNodeCommand(meta,{operation:'sanitizador.configure_upstream',args:{ssid:'LAB',wifi_password:'TEST-ONLY'}},'private')));
 assert.equal(client.published.length,0);await transport.disconnect();
});


test('MVP WS accepts a LAN IP without certificates and rejects malformed endpoints or missing accounts',async()=>{
 const {transport,client}=setup();
 await transport.connect({...settings,endpoint:'ws://192.0.2.10:9001/mqtt'});
 assert.equal(client.options.protocolVersion,5);
 assert.equal(client.options.ca,undefined);
 await transport.disconnect();
 for(const endpoint of ['wss://192.0.2.10:9001/mqtt','mqtt://192.0.2.10','ws://u:p@192.0.2.10','ws://192.0.2.10/mqtt?password=x','ws://192.0.2.10/mqtt#x','ws://192.0.2.10:0/mqtt'])await assert.rejects(()=>transport.connect({...settings,endpoint}));
 await assert.rejects(()=>transport.connect({...settings,password:''}));
});
