import type {NodeCommand,NodeMessage,NodeMetadata} from '../protocol';
export {PRIVATE_OPERATIONS} from '../protocol';
export type Channel='ble'|'mqtt';
export type TransportStatus='disconnected'|'connecting'|'connected'|'error';
export type TransportSink={message:(message:NodeMessage,channel:Channel,retained:boolean)=>void;status:(channel:Channel,status:TransportStatus)=>void;error:(code:string)=>void;metadata:(metadata:NodeMetadata)=>void};
export interface Transport {readonly channel:Channel;send(message:NodeCommand):Promise<void>;disconnect():Promise<void>}
