import {createStore} from 'zustand/vanilla';
import type {CommandMessage,Message,Metadata,StateMessage,EventMessage,PageResult} from '@sentinel/contracts';
import type {Channel,TransportStatus} from './transports/types';
export type PendingCommand={id:string;operation:string;boot:string;expectedRevision:number;status:'sending'|'pending'|'accepted'|'applied'|'rejected';error:string|null;at:number;page:boolean;afterCursor:number|null};
export type NodeStore={
 deviceId:string|null;snapshot:StateMessage|null;metadata:Metadata|null;metadataAt:number;seenBoots:string[];lastLiveAt:number|null;stale:boolean;
 links:Record<Channel,TransportStatus>;role:'operator'|'viewer';commands:PendingCommand[];events:EventMessage[];cursor:number;gap:PageResult|null;error:string|null;
 select:(id:string)=>void;setLink:(c:Channel,s:TransportStatus)=>void;setRole:(r:'operator'|'viewer')=>void;setError:(e:string|null)=>void;
 noteMetadata:(m:Metadata,now:number)=>void;receive:(m:Message,c:Channel,retained:boolean,now:number)=>void;tick:(now:number)=>void;
 track:(m:CommandMessage,now:number)=>void;sent:(id:string)=>void;uncertain:(id:string)=>void;
 setHistory:(id:string,events:EventMessage[],cursor:number,gap:PageResult|null)=>void;
};
export function createNodeStore(){return createStore<NodeStore>((set,get)=>({
 deviceId:null,snapshot:null,metadata:null,metadataAt:0,seenBoots:[],lastLiveAt:null,stale:true,links:{ble:'disconnected',mqtt:'disconnected'},role:'operator',commands:[],events:[],cursor:0,gap:null,error:null,
 select:id=>{if(get().deviceId===id)return;set({deviceId:id,snapshot:null,metadata:null,metadataAt:0,seenBoots:[],lastLiveAt:null,stale:true,commands:[],events:[],cursor:0,gap:null,error:null});},
 setLink:(channel,status)=>set(s=>({links:{...s.links,[channel]:status},...(status!=='connected'&&!Object.entries(s.links).some(([c,v])=>c!==channel&&v==='connected')?{stale:true}: {})})),
 setRole:role=>set({role}),setError:error=>set({error}),
 noteMetadata:(metadata,now)=>{if(metadata.device_id!==get().deviceId)return;set({metadata,metadataAt:now});},
 receive:(m,_channel,retained,now)=>{
  const current=get();if(m.device_id!==current.deviceId||m.type==='command'||m.type==='event'||m.type==='metrics')return;
  if(m.type==='state'){
   const old=current.snapshot;
   if(retained&&old&&old.boot_id!==m.boot_id)return;
   if(old?.boot_id!==m.boot_id&&current.seenBoots.includes(m.boot_id))return;
   if(old?.boot_id===m.boot_id&&(m.payload.revision<old.payload.revision||m.uptime_ms<old.uptime_ms||(m.uptime_ms===old.uptime_ms&&m.seq<old.seq)))return;
   const reboot=old&&old.boot_id!==m.boot_id;
   const commands=current.commands.map(cmd=>cmd.status!=='applied'&&cmd.status!=='rejected'&&cmd.boot===m.boot_id&&cmd.id===m.payload.last_command_id&&m.payload.revision>cmd.expectedRevision?{...cmd,status:'applied' as const,error:null}:cmd);
   set({snapshot:m,seenBoots:[...new Set([...current.seenBoots,m.boot_id])].slice(-16),commands,
    ...(!retained?{lastLiveAt:now,stale:false}:{}),
    ...(reboot?{metadata:null,stale:retained}:{}),
    ...(!retained?{metadata:{version:1,device_id:m.device_id,boot_id:m.boot_id,revision:m.payload.revision,uptime_ms:m.uptime_ms,max_message_bytes:2048},metadataAt:now}:{}),
   });
  }else if(m.type==='ack'){
   set({commands:current.commands.map(cmd=>{
    if(cmd.id!==m.payload.command_id)return cmd;
    if(m.payload.status!=='rejected'&&(cmd.boot!==m.boot_id||m.payload.revision<cmd.expectedRevision))return cmd;
    if(cmd.status==='applied'||cmd.status==='rejected')return cmd;
    return {...cmd,status:m.payload.status,error:m.payload.error};
   })});
  }
 },
 tick:now=>{const s=get();set({stale:s.lastLiveAt===null||now-s.lastLiveAt>=15000||!Object.values(s.links).includes('connected')});},
 track:(m,now)=>set(s=>({commands:[...s.commands,{id:m.payload.command_id,operation:m.payload.operation,boot:m.boot_id,expectedRevision:m.payload.expected_revision,status:'sending' as const,error:null,at:now,page:m.payload.operation==='read_events',afterCursor:m.payload.operation==='read_events'?m.payload.args.after_cursor:null}].slice(-32)})),
 sent:id=>set(s=>({commands:s.commands.map(c=>c.id===id&&c.status==='sending'?{...c,status:'pending'}:c)})),
 uncertain:id=>set(s=>({commands:s.commands.map(c=>c.id===id&&c.status==='sending'?{...c,status:'pending',error:'Entrega interrumpida: resultado pendiente de reconciliación.'}:c)})),
 setHistory:(id,events,cursor,gap)=>{if(get().deviceId===id)set({events,cursor,gap});},
}));}
export const nodeStore=createNodeStore();
