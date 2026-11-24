import {createStore} from 'zustand/vanilla';
import {ackMatches,safeCorrelation,type NodeCommand,type NodeMessage,type NodeMetadata,type NodeState,type NodeEvent,type NodeAck,type NodeMetrics,type PageResult,type Profile} from './protocol';
import type {Channel,TransportStatus} from './transports/types';
export type PendingCommand={id:string;operation:string;version:1|2;boot:string;expectedRevision:number;status:'sending'|'pending'|'accepted'|'applied'|'rejected';error:string|null;at:number;page:boolean;afterCursor:number|null;correlation:NodeCommand|null};
export function matchingAck(state:NodeStore,ack:NodeAck,retained=false):PendingCommand|undefined {
 if(retained||ack.device_id!==state.deviceId)return;
 const command=state.commands.find(c=>c.id===ack.payload.command_id);
 if(!command||command.status==='rejected')return;
 const boot=state.metadata?.boot_id??state.snapshot?.boot_id;
 if(ack.payload.status!=='rejected'&&boot&&command.boot!==boot)return;
 if(!ackMatches(command.correlation,command.version,command.operation,command.boot,command.expectedRevision,ack))return;
 return command;
}
function confirms(command:PendingCommand,state:NodeState) {
 const p=state.payload;
 if(command.version!==state.version||command.boot!==state.boot_id||command.id!==p.last_command_id||
    p.revision<=command.expectedRevision||p.phase==='transitioning'||p.phase==='error'||p.error)return false;
 if(command.operation==='set_mode')return command.correlation?.payload.operation==='set_mode'&&p.profile===command.correlation.payload.args.mode;
 return ['configure_connectivity','set_time'].includes(command.operation);
}
export type NodeStore={
 deviceId:string|null;snapshot:NodeState|null;metadata:NodeMetadata|null;metadataAt:number;metadataSource:'ble'|'state'|null;seenBoots:string[];lastLiveAt:number|null;stale:boolean;
 links:Record<Channel,TransportStatus>;commands:PendingCommand[];events:NodeEvent[];metrics:Record<string,NodeMetrics>;cursor:number;gap:PageResult|null;error:string|null;
 select:(id:string)=>void;setLink:(c:Channel,s:TransportStatus)=>void;setError:(e:string|null)=>void;
 noteMetadata:(m:NodeMetadata,now:number)=>void;receive:(m:NodeMessage,c:Channel,retained:boolean,now:number)=>void;tick:(now:number)=>void;
 track:(m:NodeCommand,now:number)=>void;sent:(id:string)=>void;uncertain:(id:string)=>void;
 setHistory:(id:string,events:NodeEvent[],cursor:number,gap:PageResult|null)=>void;
};
export function createNodeStore(){return createStore<NodeStore>((set,get)=>({
 deviceId:null,snapshot:null,metadata:null,metadataAt:0,metadataSource:null,seenBoots:[],lastLiveAt:null,stale:true,
 links:{ble:'disconnected',mqtt:'disconnected'},commands:[],events:[],metrics:{},cursor:0,gap:null,error:null,
 select:id=>{if(get().deviceId===id)return;set({deviceId:id,snapshot:null,metadata:null,metadataAt:0,metadataSource:null,seenBoots:[],lastLiveAt:null,stale:true,commands:[],events:[],metrics:{},cursor:0,gap:null,error:null});},
 setLink:(channel,status)=>set(s=>({links:{...s.links,[channel]:status},...(status!=='connected'&&!Object.entries(s.links).some(([c,v])=>c!==channel&&v==='connected')?{stale:true}: {})})),
 setError:error=>set({error}),
 noteMetadata:(metadata,now)=>{
  const s=get();if(metadata.device_id!==s.deviceId)return;
  const changed=s.metadata&&(s.metadata.boot_id!==metadata.boot_id||s.metadata.version!==metadata.version);
  set({metadata,metadataAt:now,metadataSource:'ble',...(changed?{stale:true,lastLiveAt:null,metrics:{}}:{})});
 },
 receive:(m,channel,retained,now)=>{
  const current=get();if(m.device_id!==current.deviceId||m.type==='command'||m.type==='event')return;
  const metadata=current.metadata;
  const authoritative=current.metadataSource==='ble'&&current.links.ble==='connected'&&metadata;
  if(m.type==='metrics'){
   const boot=metadata?.boot_id??current.snapshot?.boot_id;
   const version=metadata?.version??current.snapshot?.version;
   if(retained||!boot||m.boot_id!==boot||m.version!==version)return;
   const scope=m.version===2?m.payload.scope:'system';
   const old=current.metrics[scope];if(old&&old.boot_id===m.boot_id&&(old.uptime_ms>m.uptime_ms||old.uptime_ms===m.uptime_ms&&old.seq>=m.seq))return;
   set({metrics:{...current.metrics,[scope]:m}});return;
  }
  if(m.type==='state'){
   const old=current.snapshot;
   if(authoritative&&(m.boot_id!==metadata.boot_id||m.version!==metadata.version))return;
   if(retained){
    if(current.lastLiveAt!==null||authoritative)return;
    set({snapshot:m,stale:true});return; // no negotiation, freshness or confirmation
   }
   if(old?.boot_id!==m.boot_id&&current.seenBoots.includes(m.boot_id)&&!authoritative)return;
   if(old?.boot_id===m.boot_id&&(m.version!==old.version||m.payload.revision<old.payload.revision||m.uptime_ms<old.uptime_ms||(m.uptime_ms===old.uptime_ms&&m.seq<old.seq)))return;
   const reboot=old&&(old.boot_id!==m.boot_id||old.version!==m.version);
   const commands=current.commands.map(cmd=>cmd.status!=='applied'&&cmd.status!=='rejected'&&confirms(cmd,m)?{...cmd,status:'applied' as const,error:null}:cmd);
   set({snapshot:m,seenBoots:[...new Set([...current.seenBoots,m.boot_id])].slice(-16),commands,lastLiveAt:now,stale:false,
    ...(reboot?{metrics:{}}:{}),metadata:{version:m.version,device_id:m.device_id,boot_id:m.boot_id,revision:m.payload.revision,uptime_ms:m.uptime_ms,max_message_bytes:2048},metadataAt:now,
    metadataSource:authoritative?'ble':'state'});
  }else if(m.type==='ack'){
   const pending=matchingAck(current,m,retained);if(!pending)return;
   if(m.version===2&&m.payload.status==='applied'&&m.payload.result?.kind==='state'){
    if(m.payload.result.state.revision!==m.payload.revision)return;
    get().receive({...m,type:'state',payload:m.payload.result.state},channel,false,now);
   }
   set(s=>({commands:s.commands.map(cmd=>cmd.id!==pending.id||cmd.status==='applied'||cmd.status==='rejected'?cmd:{...cmd,status:m.payload.status,error:m.payload.error})}));
  }
 },
 tick:now=>{const s=get();set({stale:s.lastLiveAt===null||now-s.lastLiveAt>=15000||!Object.values(s.links).includes('connected')});},
 track:(m,now)=>set(s=>({commands:[...s.commands,{id:m.payload.command_id,operation:m.payload.operation,version:m.version,boot:m.payload.target_boot_id,expectedRevision:m.payload.expected_revision,status:'sending' as const,error:null,at:now,page:m.payload.operation==='read_events',afterCursor:m.payload.operation==='read_events'?m.payload.args.after_cursor:null,correlation:safeCorrelation(m)}].slice(-32)})),
 sent:id=>set(s=>({commands:s.commands.map(c=>c.id===id&&c.status==='sending'?{...c,status:'pending'}:c)})),
 uncertain:id=>set(s=>({commands:s.commands.map(c=>c.id===id&&c.status!=='applied'&&c.status!=='rejected'?{...c,status:c.status==='sending'?'pending':c.status,error:'Entrega interrumpida: resultado pendiente de reconciliación.'}:c)})),
 setHistory:(id,events,cursor,gap)=>{if(get().deviceId===id)set({events,cursor,gap});},
}));}
export const nodeStore=createNodeStore();
