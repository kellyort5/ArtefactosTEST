import {createStore} from 'zustand/vanilla';
import type {V2Network,V2Ssid,V2Detector,V2EscudoMetricValues} from '@sentinel/contracts';
import type {NodeController} from '../../controller';
import {ackMatches,type NodeEvent} from '../../protocol';

export const detectorLabels:Record<V2Detector,string>={deauth_burst:'Ráfaga de desautenticación',suspicious_disconnect:'Desconexión sospechosa',evil_twin:'Posible Evil Twin',karma_indication:'Indicio de Karma'};
export const ssidLabel=(ssid:V2Ssid)=>typeof ssid==='string'?(ssid||'SSID oculto'):`SSID hexadecimal: ${ssid.value}`;
export function detections(events:NodeEvent[]){return events.filter(e=>e.version===2&&e.payload.name==='escudo.detection');}
export type Sample={id:string;uptime:number;interval:number;values:V2EscudoMetricValues};
export type EscudoView={networks:V2Network[];next:string|null;loaded:boolean;samples:Sample[]};
const emptyCatalog={networks:[] as V2Network[],next:null,loaded:false};

// A bounded projection of validated, correlated messages. The shared controller owns BLE.
export function createEscudoSession(controller:NodeController){
 const view=createStore<EscudoView>(()=>({...emptyCatalog,samples:[]}));
 function start(){
  let session='',catalogScope='',metricId='',generation=0;const seen=new Set<string>(),contexts=new Map<string,number>();
  const sync=()=>{
   const s=controller.store.getState(),boot=s.metadata?.boot_id??s.snapshot?.boot_id;
   const key=JSON.stringify([s.deviceId,boot]);
   if(key!==session){session=key;metricId='';view.setState({samples:[]});seen.clear();}
   const mod=s.snapshot?.version===2&&s.snapshot.payload.profile==='escudo'?s.snapshot.payload.module:null;
   const scope=JSON.stringify([key,s.links.ble,mod?.scan_id,mod?.scan_status]);
   if(scope!==catalogScope){catalogScope=scope;generation++;view.setState(emptyCatalog);}
   for(const cmd of s.commands)if(!contexts.has(cmd.id))contexts.set(cmd.id,generation);
   for(const id of contexts.keys())if(!s.commands.some(c=>c.id===id))contexts.delete(id);

  };
  sync();const unsub=controller.store.subscribe(sync);
  const off=controller.observeMessages((m,channel)=>{
   const s=controller.store.getState();
   if(channel==='ble'&&s.links.ble==='connected'&&m.version===2&&m.type==='metrics'&&m.payload.scope==='escudo'&&m.boot_id===s.metadata?.boot_id&&m.device_id===s.deviceId&&s.metrics.escudo?.message_id===m.message_id&&m.message_id!==metricId){
    metricId=m.message_id;const values=m.payload.values;
    view.setState(v=>({samples:[...v.samples,{id:m.message_id,uptime:m.uptime_ms,interval:m.payload.interval_ms,values}].slice(-60)}));
   }
   if(channel!=='ble'||s.links.ble!=='connected'||m.version!==2||m.type!=='ack'||m.device_id!==s.deviceId||m.boot_id!==s.metadata?.boot_id||m.payload.status!=='applied')return;
   const cmd=s.commands.find(c=>c.id===m.payload.command_id);
   if(!cmd||cmd.status!=='applied'||!ackMatches(cmd.correlation,cmd.version,cmd.operation,cmd.boot,cmd.expectedRevision,m)||seen.has(cmd.id))return;
   seen.add(cmd.id);if(seen.size>64)seen.delete(seen.values().next().value!);
   const result=m.payload.result;
   if(result?.kind==='escudo.networks'&&cmd.correlation?.payload.operation==='escudo.networks'){
    const p=s.snapshot?.version===2?s.snapshot.payload:null;
    if(p?.profile!=='escudo'||p.module.kind!=='escudo'||p.module.scan_status!=='complete'||p.module.scan_id!==result.scan_id)return;
    if(contexts.get(cmd.id)!==generation)return;
    const after=cmd.correlation.payload.args.after,current=view.getState();
    if(after!==null&&(!current.loaded||after!==current.next))return;
    const rows=new Map((after===null?[]:current.networks).map(n=>[n.bssid,n]));
    for(const network of result.networks)rows.set(network.bssid,network);
    view.setState({networks:[...rows.values()],next:result.more?result.next:null,loaded:true});
   }
   if(['set_mode','escudo.scan','escudo.select_target','escudo.capture_start','escudo.capture_stop'].includes(cmd.operation)){
    // Queue after receiving the ACK, never await a send inside the receive queue.
    queueMicrotask(()=>{void controller.send({operation:'get_state',args:{}},'ble').catch(()=>{});});
   }
  });
  return()=>{unsub();off();};
 }
 return {view,start};
}
