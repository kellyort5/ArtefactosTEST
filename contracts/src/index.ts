import { Ajv2020 } from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';
import state from '../schemas/v1/state.schema.json' with {type:'json'};
import metrics from '../schemas/v1/metrics.schema.json' with {type:'json'};
import event from '../schemas/v1/event.schema.json' with {type:'json'};
import command from '../schemas/v1/command.schema.json' with {type:'json'};
import ack from '../schemas/v1/ack.schema.json' with {type:'json'};
import metadata from '../schemas/v1/metadata.schema.json' with {type:'json'};
import type {Message, Metadata, CommandRequest, CommandMessage} from './types.js';
import {ContractError, parseJsonBounded as parse} from './codec-primitives.js';
import {record, stringifyContract} from './codec-primitives.js';
import {decodeModuleMessage, encodeModuleMessage, decodeModuleMetadata} from './module-codec.js';
import type {V2Message, V2Metadata} from './v2-types.js';
export {ContractError} from './codec-primitives.js';
export * from './types.js';
export * from './ble.js';
export * from './v2-types.js';
export * from './module-codec.js';
export * from './notifications.js';
export const MAX_MESSAGE_BYTES = 2048;
const ajv = new Ajv2020({allErrors:false,strict:true});
// Full format validation checks Gregorian calendar dates; schema patterns restrict canonical UTC syntax.
(addFormats as unknown as (a:Ajv2020,options:{mode:'full'})=>void)(ajv,{mode:'full'});
const validators = Object.fromEntries(Object.entries({state,metrics,event,command,ack}).map(([k,s])=>[k,ajv.compile(s)]));
const validateMetadata=ajv.compile(metadata);
const utf8 = new TextEncoder();
export function decodeMessage(input:Uint8Array|string):Message {
 const data=parse(input,MAX_MESSAGE_BYTES) as Message;
 if(!data||typeof data!=='object') throw new ContractError('invalid_message');
 if(data.version!==1) throw new ContractError('unsupported_version');
 const validate=typeof data.type==='string'&&Object.hasOwn(validators,data.type)?validators[data.type]:undefined;
 if(!validate||!validate(data)) throw new ContractError('invalid_message');
 if(data.type==='command'&&data.payload.operation==='configure_connectivity') {
  if(utf8.encode(data.payload.args.ssid).length>32||utf8.encode(data.payload.args.wifi_password).length>63) throw new ContractError('invalid_message');
  try {
   const url=new URL(data.payload.args.mqtt_uri);
   if(url.port&&(Number(url.port)<1||Number(url.port)>65535)) throw new Error();
  } catch {throw new ContractError('invalid_message');}
 }
 return data;
}
export function encodeMessage(message:Message):Uint8Array {
 let text:string;
 try {text=JSON.stringify(message);} catch {throw new ContractError('invalid_message');}
 decodeMessage(text);
 return utf8.encode(text);
}
export function decodeMetadata(bytes:Uint8Array|string):Metadata {
 const data=parse(bytes,256);
 if(!validateMetadata(data)) throw new ContractError('invalid_message');
 return data as Metadata;
}
export function makeCommand(meta:Metadata, request:CommandRequest, commandId:string, elapsedMs=0):CommandMessage {
 if(!Number.isSafeInteger(elapsedMs)||elapsedMs<0||elapsedMs>15000) throw new ContractError('invalid_message');
 const uptime=meta.uptime_ms+elapsedMs;
 const result:CommandMessage={version:1,type:'command',device_id:meta.device_id,boot_id:meta.boot_id,message_id:commandId,seq:0,uptime_ms:uptime,timestamp:null,time_quality:'unknown',payload:{command_id:commandId,target_boot_id:meta.boot_id,expected_revision:meta.revision,expires_at_uptime_ms:uptime+30000,...request}};
 encodeMessage(result);return result;
}

/** Explicit mixed-version entry points; legacy APIs retain their v1 contracts. */
export type AnyMessage = Message | V2Message;
export type AnyMetadata = Metadata | V2Metadata;

export function decodeAnyMessage(input:Uint8Array|string):AnyMessage {
 const value=parse(input,MAX_MESSAGE_BYTES);
 if(!record(value)) throw new ContractError('invalid_message');
 if(value.version===1) return decodeMessage(input);
 if(value.version===2) return decodeModuleMessage(input);
 throw new ContractError('unsupported_version');
}

export function encodeAnyMessage(message:AnyMessage):Uint8Array {
 const text=stringifyContract(message);
 decodeAnyMessage(text);
 return utf8.encode(text);
}

export function decodeAnyMetadata(input:Uint8Array|string):AnyMetadata {
 const value=parse(input,256);
 if(!record(value)) throw new ContractError('invalid_message');
 if(value.version===1) return decodeMetadata(input);
 if(value.version===2) return decodeModuleMetadata(input);
 throw new ContractError('unsupported_version');
}

export const MQTT_POLICY = {
 state:{qos:1,retain:true}, availability:{qos:1,retain:true},
 metrics:{qos:0,retain:false}, events:{qos:1,retain:false},
 commands:{qos:1,retain:false}, acks:{qos:1,retain:false},
} as const;

export type TopicKind=keyof typeof MQTT_POLICY;

export function mqttTopic(version:1|2,deviceId:string,kind:TopicKind):string {
 if((version!==1&&version!==2)||typeof deviceId!=='string'||typeof kind!=='string'
   ||!/^[A-Za-z0-9._:-]{1,64}(?![\s\S])/.test(deviceId)||!Object.hasOwn(MQTT_POLICY,kind)) {
  throw new ContractError('invalid_message');
 }
 return `sentinel/v${version}/nodes/${deviceId}/${kind}`;
}

/** Historical events keep both their wire version and their original identity. */
export function eventIdentity(message:AnyMessage):string {
 const event=decodeAnyMessage(stringifyContract(message));
 if(event.type!=='event') throw new ContractError('invalid_message');
 return `${event.device_id}/${event.message_id}`;
}
