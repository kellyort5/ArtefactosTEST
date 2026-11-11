export type ErrorCode = 'invalid_message'|'unsupported_version'|'unauthorized'|'wrong_boot'|'expired'|'revision_conflict'|'busy'|'id_conflict'|'not_implemented'|'resource_error';
export type Profile = 'idle'|'escudo'|'cebo';
export type LinkState = 'disconnected'|'connecting'|'connected'|'error';
export type StatePayload = {
 profile: Profile; phase: 'idle'|'transitioning'|'ready'|'error'; revision: number; last_command_id: string|null;
 transport: {ble:'stopped'|'advertising'|'connected';wifi:LinkState;mqtt:LinkState};
 peripherals:{oled:boolean;button:boolean};
 capabilities:{ble_control:boolean;mqtt_control:boolean;event_journal:boolean;oled:boolean;button:boolean;capture:false;detectors:false;honeypot_http:false;honeypot_telnet:false;notifications:false;sanitizador:false}; error: ErrorCode|null;
};
export type CommandRequest =
 | {operation:'get_state'|'start_capture'|'start_honeypot';args:Record<string,never>}
 | {operation:'set_mode';args:{mode:Profile|'sanitizador'}}
 | {operation:'configure_connectivity';args:{ssid:string;wifi_password:string;mqtt_uri:string;mqtt_username:string;mqtt_password:string;ca_certificate:string}}
 | {operation:'set_time';args:{timestamp:string}}
 | {operation:'read_events';args:{after_cursor:number;limit:number}};
export type CommandPayload = CommandRequest & {command_id:string;target_boot_id:string;expected_revision:number;expires_at_uptime_ms:number};
export type PageResult = {next_cursor:number;retained_from:number;retained_to:number;more:boolean;gap:boolean};
export type AckPayload = {command_id:string;status:'accepted'|'applied'|'rejected';revision:number;error:ErrorCode|null;result:PageResult|null};
export type EventPayload = {journal_seq:number;name:'system.booted'|'system.profile_changed'|'system.error';profile:Profile;code:ErrorCode|null};
export type Envelope<T extends string,P> = {version:1;type:T;device_id:string;boot_id:string;message_id:string;seq:number;uptime_ms:number;timestamp:string|null;time_quality:'unknown'|'app_sync'|'network_sync';payload:P};
export type StateMessage = Envelope<'state',StatePayload>;
export type EventMessage = Envelope<'event',EventPayload>;
export type AckMessage = Envelope<'ack',AckPayload>;
export type CommandMessage = Envelope<'command',CommandPayload>;
export type MetricsMessage = Envelope<'metrics',{free_heap_bytes:number;min_free_heap_bytes:number;journal_count:number}>;
export type Message = StateMessage|EventMessage|AckMessage|CommandMessage|MetricsMessage;
export type Metadata = {version:1;device_id:string;boot_id:string;revision:number;uptime_ms:number;max_message_bytes:2048};
