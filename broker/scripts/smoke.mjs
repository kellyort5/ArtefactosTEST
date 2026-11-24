// Real anonymous MQTT5 TCP/WS test; authorized local lab only.
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import mqtt from 'mqtt';
const host=process.env.SENTINEL_TEST_HOST??'127.0.0.1';
const clients=new Set();
const topic=`sentinel/v2/nodes/smoke-${crypto.randomUUID()}/state`;
async function connect(protocol){
 const port=protocol==='mqtt'?1883:9001;
 const client=await mqtt.connectAsync(`${protocol}://${host}:${port}${protocol==='ws'?'/mqtt':''}`,{protocolVersion:5,clean:true,reconnectPeriod:0,connectTimeout:5000});
 clients.add(client);return client;
}
function receive(client,expected){return new Promise((resolve,reject)=>{
 const timer=setTimeout(()=>{client.off('message',handler);reject(new Error('smoke timeout'));},6000);
 function handler(name,payload,packet){if(name===topic&&payload.toString()===expected){clearTimeout(timer);client.off('message',handler);resolve(packet);}}
 client.on('message',handler);
});}
try{
 const tcp=await connect('mqtt'),ws=await connect('ws');await ws.subscribeAsync(topic,{qos:1});
 const received=receive(ws,'anonymous-tcp-ws');await tcp.publishAsync(topic,'anonymous-tcp-ws',{qos:1,retain:true});await received;
 const fresh=await connect('mqtt');const replay=receive(fresh,'anonymous-tcp-ws');await fresh.subscribeAsync(topic,{qos:1});assert.equal((await replay).retain,true);
 await ws.publishAsync(topic,'anonymous-ws-tcp',{qos:1,retain:true});
 if(process.argv.includes('--restart')){
  for(const c of clients)await c.endAsync(true);clients.clear();
  const result=spawnSync('python3',['broker/scripts/manage.py','restart'],{cwd:new URL('../..',import.meta.url),encoding:'utf8'});
  assert.equal(result.status,0,'Compose restart');
  // Retry connectivity briefly while the same persisted broker starts.
  let reader;for(let n=0;n<20&&!reader;n++){try{reader=await connect('mqtt');}catch{await new Promise(r=>setTimeout(r,250));}}
  assert.ok(reader);const persisted=receive(reader,'anonymous-ws-tcp');await reader.subscribeAsync(topic,{qos:1});assert.equal((await persisted).retain,true);
 }
 const cleanup=await connect('mqtt');await cleanup.publishAsync(topic,'',{qos:1,retain:true});
 console.log('PASS MQTT5 anonymous TCP/WS, cross-transport delivery, retained'+(process.argv.includes('--restart')?' and persistent restart':'')+'. No accounts/ACL or radio claim.');
}finally{await Promise.all([...clients].map(c=>c.endAsync(true)));}
