import type {CommandMessage,Message,Metadata} from '@sentinel/contracts';
export type Channel='ble'|'mqtt';
export type TransportStatus='disconnected'|'connecting'|'connected'|'error';
export type TransportSink={message:(message:Message,channel:Channel,retained:boolean)=>void;status:(channel:Channel,status:TransportStatus)=>void;error:(code:string)=>void;metadata:(metadata:Metadata)=>void};
export interface Transport {readonly channel:Channel;send(message:CommandMessage):Promise<void>;disconnect():Promise<void>}
export const PRIVATE_OPERATIONS=new Set(['configure_connectivity','set_time']);
