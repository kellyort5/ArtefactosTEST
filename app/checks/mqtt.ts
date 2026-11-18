// Real anonymous WS adapter -> TCP broker test. No radio or node execution claim.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {connectAsync} from 'mqtt';
import {makeCommand,decodeMessage} from '@sentinel/contracts';
import {MqttTransport} from '../src/transports/mqtt';
const nodeId='adapter-'+crypto.randomUUID();
test('real anonymous WS connects and publishes a validated unretained command to TCP',async()=>{
 const node=await connectAsync('mqtt://127.0.0.1:1883',{protocolVersion:5,clean:true,reconnectPeriod:0,connectTimeout:5000});
 const adapter=new MqttTransport({message(){},metadata(){},error(){},status(){}});
 try{
  const topic=`sentinel/v1/nodes/${nodeId}/commands`;await node.subscribeAsync(topic,{qos:1});
  await adapter.connect({endpoint:'ws://127.0.0.1:9001/mqtt',deviceId:nodeId});
  const received=new Promise<void>((resolve,reject)=>{
   const timer=setTimeout(()=>reject(new Error('No WS to TCP command')),5000);
   node.once('message',(actual,bytes,packet)=>{clearTimeout(timer);try{assert.equal(actual,topic);assert.equal(packet.retain,false);assert.equal(decodeMessage(bytes).type,'command');resolve();}catch{reject(new Error('Invalid command contract'));}});
  });
  await adapter.send(makeCommand({version:1,device_id:nodeId,boot_id:'test-boot',revision:0,uptime_ms:0,max_message_bytes:2048},{operation:'get_state',args:{}},crypto.randomUUID()));await received;
 }finally{await adapter.disconnect();await node.endAsync(true);}
});
