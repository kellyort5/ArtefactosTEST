import {
  makeCommand, makeModuleCommand, MODULE_COMMAND_RESULTS,
  checkModuleAckForCommand, decodeAnyMessage, encodeAnyMessage,
  type AnyMessage, type AnyMetadata, type CommandRequest, type V2CommandRequest,
  type V2Profile, type PageResult,
} from '@sentinel/contracts';
export type NodeMessage = AnyMessage;
export type NodeMetadata = AnyMetadata;
export type NodeState = Extract<NodeMessage, {type:'state'}>;
export type NodeEvent = Extract<NodeMessage, {type:'event'}>;
export type NodeAck = Extract<NodeMessage, {type:'ack'}>;
export type NodeCommand = Extract<NodeMessage, {type:'command'}>;
export type NodeMetrics = Extract<NodeMessage, {type:'metrics'}>;
export type NodeRequest = CommandRequest | V2CommandRequest;
export type Profile = V2Profile;
export type {PageResult};
export const PRIVATE_OPERATIONS = new Set([
  'configure_connectivity', 'sanitizador.configure_upstream',
  'sanitizador.configure_ap', 'sanitizador.configure_doh',
]);
export function bleOnly(operation:string) {
  return PRIVATE_OPERATIONS.has(operation) || operation==='set_time' ||
    operation.startsWith('escudo.') || operation.startsWith('sanitizador.');
}
export function makeNodeCommand(meta:NodeMetadata,request:NodeRequest,id:string,elapsed=0):NodeCommand {
  if(meta.version===2) {
    if(request.operation==='start_capture'||request.operation==='start_honeypot')
      throw new Error('Esta operación reservada de v1 no está disponible en v2.');
    return makeModuleCommand(meta,request as V2CommandRequest,id,elapsed);
  }
  if(request.operation.includes('.') || request.operation==='set_mode' && request.args.mode==='sanitizador')
    throw new Error('El nodo v1 no admite esta función de módulos.');
  return makeCommand(meta,request as CommandRequest,id,elapsed);
}
export function modeAvailable(snapshot:NodeState|null,profile:Profile):boolean {
  if(!snapshot)return false;
  if(profile==='idle')return true;
  if(snapshot.version===1)return profile==='escudo'||profile==='cebo'; // v1 resources only
  const p=snapshot.payload,c=p.capabilities;
  return p.profile===profile || (profile==='escudo' ? c.capture||c.detectors
    : profile==='cebo' ? c.honeypot_http||c.honeypot_telnet : c.sanitizador);
}
export function operationAvailable(snapshot:NodeState|null,request:NodeRequest):boolean {
  if(['get_state','read_events','set_time','configure_connectivity'].includes(request.operation))return true;
  if(request.operation==='set_mode')return modeAvailable(snapshot,request.args.mode as Profile);
  if(!snapshot || snapshot.version!==2 || snapshot.payload.phase!=='ready')return false;
  if(request.operation.startsWith('escudo.'))return snapshot.payload.profile==='escudo' &&
    (!request.operation.startsWith('escudo.capture_') || snapshot.payload.capabilities.capture);
  if(request.operation.startsWith('sanitizador.'))return snapshot.payload.profile==='sanitizador'&&snapshot.payload.capabilities.sanitizador;
  if(request.operation.startsWith('cebo.'))return snapshot.payload.profile==='cebo'&&
    (snapshot.payload.capabilities.honeypot_http||snapshot.payload.capabilities.honeypot_telnet);
  return false;
}
export function eventPage(ack:NodeAck):PageResult|null {
  const result=ack.payload.result;
  if(ack.payload.status!=='applied'||!result)return null;
  if(ack.version===1)return result as PageResult;
  return 'kind' in result && result.kind==='events' ? result : null;
}
// Keep public command correlation in session memory, never private arguments.
export function safeCorrelation(command:NodeCommand):NodeCommand|null {
  return PRIVATE_OPERATIONS.has(command.payload.operation) ? null : structuredClone(command);
}
export function ackMatches(command:NodeCommand|null,version:1|2,operation:string,
  boot:string,revision:number,ack:NodeAck):boolean {
  if(ack.version!==version || ack.payload.status!=='rejected' &&
      (ack.boot_id!==boot||ack.payload.revision<revision))return false;
  if(ack.payload.status==='rejected')return true;
  if(command && command.version===2 && ack.version===2)
    return checkModuleAckForCommand(command,ack)===null;
  if(ack.version===2) {
    const allowed:readonly (string|null)[]=MODULE_COMMAND_RESULTS[operation as V2CommandRequest['operation']]??[];
    return ack.payload.status==='accepted'||allowed.includes(ack.payload.result?.kind??null);
  }
  return operation==='read_events' ? ack.payload.status==='accepted'||eventPage(ack)!==null
    : ack.payload.result===null;
}
export function validateMessage(message:NodeMessage):NodeMessage {
  return decodeAnyMessage(encodeAnyMessage(message));
}
