// Explicit opt-in laboratory integration check. Secrets are read from ignored files, never arguments/logs.
import {readFileSync,readdirSync} from 'node:fs';
import {resolve} from 'node:path';
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {connectAsync} from 'mqtt';
import {makeCommand,decodeMessage} from '@sentinel/contracts';
import {MqttTransport} from '../src/transports/mqtt';
import type {TransportSink} from '../src/transports/types';
const root=resolve(import.meta.dirname,'../..');
const lab=JSON.parse(readFileSync(resolve(root,'broker/.secrets/clients.json'),'utf8'));
const sink:TransportSink={message:()=>{},status:()=>{},error:()=>{},metadata:()=>{}};
const settings=(role:'operator'|'viewer')=>({endpoint:'ws://localhost:9001/mqtt',deviceId:lab.node_id,role,...lab.accounts[role]});
test('real local WS operator connects and publishes a validated unretained command',async()=>{
 const node=await connectAsync('mqtt://localhost:1883',{protocolVersion:5,clean:true,reconnectPeriod:0,connectTimeout:5000,...lab.accounts.node});
 const adapter=new MqttTransport(sink);
 try {
  const topic=`sentinel/v1/nodes/${lab.node_id}/commands`;await node.subscribeAsync(topic,{qos:1});
  await adapter.connect(settings('operator'));
  const received=new Promise<void>((resolve,reject)=>{
   const timer=setTimeout(()=>reject(new Error('No se recibió comando WS en listener TCP.')),5000);
   node.once('message',(actualTopic,bytes,packet)=>{try{assert.equal(actualTopic,topic);assert.equal(packet.retain,false);assert.equal(decodeMessage(bytes).type,'command');clearTimeout(timer);resolve();}catch{clearTimeout(timer);reject(new Error('Contrato/canal de comando incorrecto.'));}});
  });
  const message=makeCommand({version:1,device_id:lab.node_id,boot_id:'integration-check-only',revision:0,uptime_ms:0,max_message_bytes:2048},{operation:'get_state',args:{}},crypto.randomUUID());
  await adapter.send(message);await received;
 }finally{await adapter.disconnect();await node.endAsync(true);}
});
test('real WS rejects incorrect credentials and viewer cannot send',async()=>{
 const adapter=new MqttTransport(sink);
 try{const wrong=settings('operator');wrong.password='invalid-test-'+crypto.randomUUID();await assert.rejects(()=>adapter.connect(wrong));}
 finally{await adapter.disconnect();}
 const viewer=new MqttTransport(sink);
 try{await viewer.connect(settings('viewer'));await assert.rejects(()=>viewer.send(makeCommand({version:1,device_id:lab.node_id,boot_id:'integration-check-only',revision:0,uptime_ms:0,max_message_bytes:2048},{operation:'get_state',args:{}},crypto.randomUUID())));}
 finally{await viewer.disconnect();}
});
test('no private laboratory credentials in web bundle or maintained source',()=>{
 const passwords=[...Object.values(lab.accounts),...Object.values(lab.peer_accounts??{})].map((a:any)=>a.password as string);
 function scan(dir:string){for(const entry of readdirSync(dir,{withFileTypes:true})){const path=resolve(dir,entry.name);if(entry.isDirectory())scan(path);else{const content=readFileSync(path,'utf8');for(const secret of passwords)if(secret&&content.includes(secret))throw new Error('Credencial privada encontrada en fuente/bundle.');}}}
 scan(resolve(root,'app/dist'));scan(resolve(root,'app/src'));scan(resolve(root,'contracts/src'));
});
