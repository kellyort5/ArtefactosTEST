/** Source for the module v2 schemas and TypeScript declarations. Run with Node.
 * Schema strings limit Unicode code points; the codec must also enforce UTF-8
 * byte limits (SSID 32, password 63, message 2048, metadata 256), valid URLs,
 * country channels, normalized domains, subnet separation, observed targets,
 * authorized destinations, BLE encryption, freshness, revision and idempotency.
 * Counts are integers; times/durations are ms, RSSI dBm, volumes IPv4 bytes.
 * Pages contain <=4 records. History ACKs describe <=8 separately sent events.
 * No schema announces an implemented service; available and status are explicit.
 * CA fields only validate bounded strings here and in the codec; the real TLS
 * transport validates the certificate chain and CA. Test CA values are fictional.
 */
import {mkdir, writeFile} from 'node:fs/promises';
import {fileURLToPath} from 'node:url';

const base = 'https://sentinel.local/contracts/v2/';
const defs = {};
const ref = name => ({$ref:`${base}common.schema.json#/$defs/${name}`});
const en = (...values) => ({enum:values});
const str = (maxLength, minLength=0, extra={}) => ({type:'string',minLength,maxLength,...extra});
const integer = (maximum=Number.MAX_SAFE_INTEGER, minimum=0) => ({type:'integer',minimum,maximum});
const bool = {type:'boolean'};
const nil = {type:'null'};
const nullable = schema => ({anyOf:[schema,nil]});
const obj = (properties, optional=[]) => ({type:'object',properties,required:Object.keys(properties).filter(k=>!optional.includes(k)),additionalProperties:false});
const list = (items,maxItems,minItems=0) => ({type:'array',items,minItems,maxItems});
const union = (...oneOf) => ({oneOf});
const add = (name,schema) => (defs[name]=schema,ref(name));
const N = integer();
const ID = add('V2Identifier',str(64,1,{pattern:'^[A-Za-z0-9._:-]+(?![\\s\\S])'}));
const profile = add('V2Profile',en('idle','escudo','sanitizador','cebo'));
const error = add('V2ErrorCode',en('invalid_message','unsupported_version','unauthorized','wrong_boot','expired','revision_conflict','busy','id_conflict','not_implemented','resource_error','invalid_state','invalid_args','timeout','upstream_unavailable','subnet_conflict','capacity_exceeded'));
const utc = add('V2UtcTimestamp',{type:'string',format:'date-time',pattern:'^(?!0000-)[0-9]{4}-(?:0[1-9]|1[0-2])-(?:0[1-9]|[12][0-9]|3[01])T(?:[01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9](?:\\.[0-9]{1,12})?Z(?![\\s\\S])',maxLength:32});
const text = (max,min=0) => str(max,min,{pattern:'^[^\u0000]*(?![\\s\\S])'});
const ssid = add('V2Ssid',union(text(32),obj({encoding:{const:'hex'},value:str(64,0,{pattern:'^(?:[0-9a-f]{2})*(?![\\s\\S])'})})));
const wifiSsid = text(32,1);
const password = text(63);
const channel = integer(14,1);
const rssi = integer(0,-127);
const mac = add('V2MacAddress',str(17,17,{pattern:'^(?:[0-9a-f]{2}:){5}[0-9a-f]{2}(?![\\s\\S])'}));
const ip = {type:'string',format:'ipv4',maxLength:15};
const domain = add('V2Domain',str(253,1,{pattern:'^(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\\.)*[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?(?![\\s\\S])'}));
const url = (httpsOnly=true) => str(256,1,{format:'uri',pattern:`^${httpsOnly?'https':'https?'}://(?:[A-Za-z0-9](?:[A-Za-z0-9.-]*[A-Za-z0-9])?|\\[[0-9A-Fa-f:]+\\])(?::[0-9]{1,5})?(?:/[^\\s?#@]*)?(?![\\s\\S])`});
const cursor = add('V2CatalogCursor',str(128,1,{pattern:'^[A-Za-z0-9._:~-]+(?![\\s\\S])'}));
const page = add('V2PageRequest',obj({after:nullable(cursor),limit:integer(4,1)}));
const link = add('V2LinkState',en('disconnected','connecting','connected','error'));
const capture = add('V2CaptureStatus',en('stopped','starting','running','paused','error'));
const detector = add('V2Detector',en('deauth_burst','suspicious_disconnect','evil_twin','karma_indication'));
const suspicion = add('V2Suspicion',en('informational','medium','high'));
const severity = add('V2Severity',en('info','warning','critical'));
const origin = add('V2EventOrigin',en('real','simulated'));
const security = {
 auth:en('open','wep','wpa_personal','wpa2_personal','wpa3_personal','wpa_wpa2_personal','wpa2_wpa3_personal','enterprise','unknown'),
 cipher:en('none','wep','tkip','ccmp','gcmp','mixed','unknown'),pmf:en('disabled','capable','required','unknown')
};
const network = add('V2Network',obj({ssid,bssid:mac,channel,rssi,...security,seen_uptime_ms:N}));
const target = add('V2Target',obj({ssid,bssid:mac,channel}));
const targetDetails = add('V2TargetDetails',obj({ssid,bssid:mac,channel,capture_status:capture}));
const pause = add('V2CoverageReason',en('none','ble_sync','channel_hop','target_change','resource_error','stopped'));
const coverage = add('V2Coverage',obj({channel:nullable(channel),pause_reason:pause,started_uptime_ms:N,duration_ms:N,dropped_frames:N,dropped_events:N}));
const service = add('V2ServiceState',union(obj({available:{const:false},status:{const:'unavailable'}}),obj({available:{const:true},status:en('stopped','starting','running','error')})));
const evidence = (name,values) => add(name,{...list(en(...values),8,1),uniqueItems:true});
const detectionEvidence = evidence('V2DetectionEvidence',['deauth_rate','disassoc_rate','disconnect_sequence','same_ssid','different_bssid','channel_change','rssi_change','security_downgrade','pmf_change','unsolicited_probe_response','multiple_ssids','passive_only']);
const auditEvidence = evidence('V2AuditEvidence',['dns_answer','dns_timeout','dns_mismatch','https_valid','tls_invalid','https_timeout','portal_redirect','portal_content_mismatch','portal_unreachable','no_upstream','not_run']);
const ceboEvidence = evidence('V2CeboEvidence',['connection_observed','canonical_access','auth_attempt','input_observed','repeated_connections','repeated_access','repeated_auth','rate_threshold','idle_timeout','peer_closed','capacity_limit']);
const auditCheck = add('V2AuditCheck',obj({status:en('ok','warning','failed','inconclusive'),evidence:auditEvidence,response_ms:nullable(N)}));
const audit = add('V2AuditResult',obj({job_id:ID,dns:auditCheck,https:auditCheck,portal:auditCheck,connectivity:en('online','limited','offline','unknown'),completed_uptime_ms:N}));
const client = add('V2Client',{
 ...obj({mac,ipv4:ip,rssi:nullable(rssi),rssi_reason:en(null,'not_available','not_associated','driver_unsupported'),ipv4_upload_bytes:N,ipv4_download_bytes:N,seen_uptime_ms:N}),
 oneOf:[{properties:{rssi:{type:'null'},rssi_reason:en('not_available','not_associated','driver_unsupported')}},{properties:{rssi,rssi_reason:nil}}]
});
const ruleId = add('V2RuleId',str(32,1,{pattern:'^[A-Za-z0-9._:-]+(?![\\s\\S])'}));
const rule = add('V2Rule',obj({rule_id:ruleId,domain,action:en('allow','block'),include_subdomains:bool}));
const protocolMetrics = add('V2ProtocolMetrics',obj({connections:N,accesses:N,auth_attempts:N,inputs:N,active_connections:integer(6),interactions_per_minute:{type:'number',minimum:0,maximum:Number.MAX_SAFE_INTEGER},alerts:N}));
const metricValues = {
 system:add('V2SystemMetricValues',obj({free_heap_bytes:N,min_free_heap_bytes:N,queue_count:N,journal_count:integer(64),dropped_messages:N,dropped_events:N})),
 escudo:add('V2EscudoMetricValues',obj({beacons:N,deauth:N,disassoc:N,alerts:obj({deauth_burst:N,suspicious_disconnect:N,evil_twin:N,karma_indication:N}),channel:nullable(channel),coverage_ms:N,paused_ms:N,dropped_frames:N})),
 sanitizador:add('V2SanitizadorMetricValues',obj({clients:N,ipv4_upload_bytes:N,ipv4_download_bytes:N,dns_queries:N,dns_blocked:N,dns_cache_hits:N,dns_failures:N,dns_latency_ms:nullable(N)})),
 cebo:add('V2CeboMetricValues',obj({http:protocolMetrics,telnet:protocolMetrics,rejected_connections:N,alerts:N}))
};
const metrics = add('V2MetricsPayload',union(...Object.entries(metricValues).map(([scope,values])=>obj({scope:{const:scope},interval_ms:integer(Number.MAX_SAFE_INTEGER,1),values}))));
const capabilities = add('V2Capabilities',obj(Object.fromEntries(['ble_control','mqtt_control','event_journal','oled','button','capture','detectors','honeypot_http','honeypot_telnet','notifications','sanitizador'].map(k=>[k,bool]))));
const modules = {
 idle:add('V2IdleModule',obj({kind:{const:'idle'}})),
 escudo:add('V2EscudoModule',obj({kind:{const:'escudo'},scan_status:en('idle','running','complete','error'),scan_id:nullable(ID),target:nullable(target),capture_status:capture,coverage})),
 sanitizador:add('V2SanitizadorModule',obj({kind:{const:'sanitizador'},upstream:link,audit_status:en('idle','running','complete','error'),audit_job_id:nullable(ID),gateway_status:en('stopped','starting','running','degraded','error'),ap:service,dhcp:service,nat:service,dns:service,doh:service,client_count:N,rule_count:integer(32)})),
 cebo:add('V2CeboModule',obj({kind:{const:'cebo'},http:service,telnet:service,active_connections:integer(6),max_connections:integer(6,1),idle_timeout_ms:integer(60000,1000)}))
};
// A running gateway only describes the complete IPv4 pipeline. It is no claim
// about IPv6, external DNS, or notification delivery.
defs.V2SanitizadorModule.allOf=[{if:{properties:{gateway_status:{const:'running'}}},then:{properties:{upstream:{const:'connected'},...Object.fromEntries(['ap','dhcp','nat','dns','doh'].map(k=>[k,{type:'object',properties:{available:{const:true},status:{const:'running'}},required:['available','status']}]))}}}];
defs.V2EscudoModule.allOf=[{if:{properties:{capture_status:en('starting','running','paused')}},then:{properties:{target}}}];
const stateBase = {phase:en('idle','transitioning','ready','error'),revision:N,last_command_id:nullable(ID),transport:obj({ble:en('stopped','advertising','connected'),wifi:link,mqtt:link}),peripherals:obj({oled:bool,button:bool}),capabilities,error:nullable(error)};
const state = add('V2StatePayload',union(...Object.entries(modules).map(([p,module])=>{
 const branch=obj({...stateBase,profile:{const:p},module});
 if(p==='escudo') branch.properties.transport=obj({ble:en('stopped','advertising','connected'),wifi:{const:'disconnected'},mqtt:{const:'disconnected'}});
 const runningCapability = (field,cap,values=['running']) => ({if:{properties:{module:{type:'object',properties:{[field]:en(...values)},required:[field]}}},then:{properties:{capabilities:{type:'object',properties:{[cap]:{const:true}},required:[cap]}}}});
 if(p==='escudo') branch.allOf=[runningCapability('capture_status','capture',['starting','running','paused'])];
 if(p==='sanitizador') branch.allOf=[runningCapability('gateway_status','sanitizador',['starting','running','degraded'])];
 if(p==='cebo') branch.allOf=['http','telnet'].map(field=>({if:{properties:{module:{type:'object',properties:{[field]:{type:'object',properties:{status:{const:'running'}},required:['status']}},required:[field]}}},then:{properties:{capabilities:{type:'object',properties:{[`honeypot_${field}`]:{const:true}},required:[`honeypot_${field}`]}}}}));
 return branch;
})));

const networkPage = add('V2NetworksPage',obj({scan_id:ID,networks:list(network,4),next:nullable(cursor),more:bool}));
const clientsPage = add('V2ClientsPage',obj({clients:list(client,4),next:nullable(cursor),more:bool}));
const rulesPage = add('V2RulesPage',obj({rules:list(rule,4),next:nullable(cursor),more:bool}));
// The terminal page has no continuation; a nonterminal page always supplies it.
for (const name of ['V2NetworksPage','V2ClientsPage','V2RulesPage']) defs[name].oneOf=[{properties:{more:{const:false},next:nil}},{properties:{more:{const:true},next:cursor}}];
const detection = add('V2DetectionDetails',obj({detector,ssid,bssid:mac,related_bssid:mac,channel,rssi,...security,count:integer(Number.MAX_SAFE_INTEGER,1),window_ms:integer(Number.MAX_SAFE_INTEGER,1),suspicion,evidence:detectionEvidence},['related_bssid']));
const dns = add('V2DnsDetails',obj({outcome:en('resolved','blocked','failed'),domain,client_ipv4:ip,cache_hit:bool,doh:bool,latency_ms:N,reason:en('allowed','rule_block','cache','upstream','timeout','invalid_response','tls_error','unavailable','capacity_exceeded')}));
defs.V2DnsDetails.oneOf=[{properties:{outcome:{const:'resolved'},reason:en('allowed','cache','upstream')}},{properties:{outcome:{const:'blocked'},reason:{const:'rule_block'},cache_hit:{const:false},doh:{const:false}}},{properties:{outcome:{const:'failed'},reason:en('timeout','invalid_response','tls_error','unavailable','capacity_exceeded'),cache_hit:{const:false}}}];
const cebo = add('V2CeboInteractionDetails',obj({source_ipv4:ip,source_port:integer(65535,1),destination_port:integer(65535,1),protocol:en('http','telnet'),interaction:en('connect','access','auth_attempt','input','disconnect'),path:en(null,'/','/login','/admin','/favicon.ico','/robots.txt','/other'),count:integer(Number.MAX_SAFE_INTEGER,1),window_ms:integer(Number.MAX_SAFE_INTEGER,1),suspicion,evidence:ceboEvidence}));
defs.V2CeboInteractionDetails.allOf=[{if:{properties:{protocol:{const:'telnet'}}},then:{properties:{destination_port:{const:23},path:nil}}}];
const resource = add('V2Resource',en('system','radio','ble','wifi','mqtt','journal','queue','oled','button','capture','detectors','audit','ap','dhcp','nat','dns','doh','http','telnet'));
const statusDetails = add('V2SystemStatusDetails',obj({status_code:en('recovered',...defs.V2ErrorCode.enum),resource}));
const eventBase = {journal_seq:integer(Number.MAX_SAFE_INTEGER,1),origin};
const eventBranch = (name,p,details,code=nil) => obj({...eventBase,name:{const:name},profile:typeof p==='string'?{const:p}:p,code,severity,details});
const ceboBranch = eventBranch('cebo.interaction','cebo',cebo);
ceboBranch.allOf=['informational','medium','high'].map((s,i)=>({if:{properties:{details:{type:'object',properties:{suspicion:{const:s}},required:['suspicion']}}},then:{properties:{severity:{const:['info','warning','critical'][i]}}}}));
const events = [
 eventBranch('escudo.networks','escudo',networkPage),eventBranch('escudo.target','escudo',targetDetails),eventBranch('escudo.detection','escudo',detection),eventBranch('escudo.coverage','escudo',coverage),
 eventBranch('sanitizador.audit','sanitizador',audit),eventBranch('sanitizador.clients','sanitizador',clientsPage),eventBranch('sanitizador.rules','sanitizador',rulesPage),eventBranch('sanitizador.dns','sanitizador',dns),ceboBranch,
 eventBranch('system.status',profile,statusDetails),eventBranch('system.booted',profile,obj({})),eventBranch('system.profile_changed',profile,obj({previous_profile:profile})),eventBranch('system.error',profile,obj({resource}),error)
];
add('V2EventPayload',union(...events));

const connectivityArgs = JSON.parse(await (await import('node:fs/promises')).readFile(new URL('../schemas/v1/command.schema.json',import.meta.url),'utf8')).properties.payload.oneOf.find(b=>b.properties.operation.const==='configure_connectivity').properties.args;
// Reuse the v1 argument shape while giving only its v2 copy strict end anchors.
// In JavaScript, $ can also match just before a final line terminator.
for (const field of Object.values(connectivityArgs.properties)) if (field.pattern?.endsWith('$')) field.pattern=field.pattern.slice(0,-1)+'(?![\\s\\S])';
const operations = {
 get_state:obj({}),set_mode:obj({mode:profile}),configure_connectivity:connectivityArgs,set_time:obj({timestamp:utc}),read_events:obj({after_cursor:N,limit:integer(8,1)}),
 'escudo.scan':obj({}),'escudo.networks':page,'escudo.select_target':target,'escudo.capture_start':obj({}),'escudo.capture_stop':obj({}),
 'sanitizador.configure_upstream':obj({ssid:wifiSsid,wifi_password:password}),
 'sanitizador.configure_ap':obj({ssid:wifiSsid,wifi_password:text(63,8),ipv4:ip,netmask:ip}),
 'sanitizador.configure_doh':obj({endpoint:url(),ca_certificate:text(1536,1)}),
 'sanitizador.audit':obj({dns_name:domain,https_url:url(),portal_url:url(false)}),'sanitizador.audit_result':obj({job_id:ID}),
 'sanitizador.gateway_start':obj({}),'sanitizador.gateway_stop':obj({}),'sanitizador.clients':page,
 'sanitizador.rules.put':rule,'sanitizador.rules.remove':obj({rule_id:ruleId}),'sanitizador.rules.list':page,
 'cebo.start':obj({}),'cebo.stop':obj({}),'cebo.configure':obj({http_enabled:{const:true},telnet_enabled:{const:true},max_connections:integer(6,1),idle_timeout_ms:integer(60000,1000)}),'cebo.stats':obj({})
};
add('V2CommandRequest',union(...Object.entries(operations).map(([operation,args])=>obj({operation:{const:operation},args}))));
const commandCommon = {command_id:ID,target_boot_id:ID,expected_revision:N,expires_at_uptime_ms:N};
const command = add('V2CommandPayload',union(...Object.entries(operations).map(([operation,args])=>obj({...commandCommon,operation:{const:operation},args}))));
const history = add('V2HistoryPageResult',obj({kind:{const:'events'},next_cursor:N,retained_from:N,retained_to:N,more:bool,gap:bool}));
const result = add('V2AckResult',union(
 obj({kind:{const:'state'},state}),obj({kind:{const:'job'},job_id:ID}),
 obj({kind:{const:'escudo.networks'},...defs.V2NetworksPage.properties}),
 obj({kind:{const:'escudo.target'},target:targetDetails}),
 obj({kind:{const:'sanitizador.clients'},...defs.V2ClientsPage.properties}),
 obj({kind:{const:'sanitizador.rules'},...defs.V2RulesPage.properties}),
 obj({kind:{const:'sanitizador.rule'},rule}),obj({kind:{const:'sanitizador.rule_removed'},rule_id:ruleId}),
 obj({kind:{const:'sanitizador.audit'},audit:nullable(audit)}),obj({kind:{const:'cebo.stats'},interval_ms:integer(Number.MAX_SAFE_INTEGER,1),values:metricValues.cebo}),history
));
for (const [kind,name] of [['escudo.networks','V2NetworksPage'],['sanitizador.clients','V2ClientsPage'],['sanitizador.rules','V2RulesPage']]) defs.V2AckResult.oneOf.find(s=>s.properties.kind.const===kind).oneOf=defs[name].oneOf;
const ack = add('V2AckPayload',union(...['accepted','applied','rejected'].map(status=>obj({command_id:ID,status:{const:status},revision:N,error:status==='rejected'?error:nil,result:status==='applied'?nullable(result):nil}))));
const metadata = add('V2Metadata',obj({version:{const:2},device_id:ID,boot_id:ID,revision:N,uptime_ms:N,max_message_bytes:{const:2048}}));

const envelope = (type,payload) => ({...obj({version:{const:2},type:{const:type},device_id:ID,boot_id:ID,message_id:ID,seq:N,uptime_ms:N,timestamp:nullable(utc),time_quality:en('unknown','app_sync','network_sync'),payload}),allOf:[{if:{properties:{time_quality:{const:'unknown'}}},then:{properties:{timestamp:nil}},else:{properties:{timestamp:utc}}}]});
const schemas = Object.fromEntries(Object.entries({state,metrics,event:ref('V2EventPayload'),command,ack}).map(([type,payload])=>[type,envelope(type,payload)]));
schemas.metadata={$ref:metadata.$ref};
const rootDir = new URL('../',import.meta.url);
await mkdir(new URL('schemas/v2/',rootDir),{recursive:true});
for(const [name,schema] of Object.entries({common:{$defs:defs},...schemas})) await writeFile(new URL(`schemas/v2/${name}.schema.json`,rootDir),JSON.stringify({$schema:'https://json-schema.org/draft/2020-12/schema',$id:`${base}${name}.schema.json`,$comment:'Generated by contracts/tools/generate-v2-schemas.mjs. Semantic byte/context checks belong to the codec.',...schema},null,2)+'\n');

// A deliberately small schema-to-type printer for the constructs used above.
// Numeric bounds and conditional invariants remain runtime schema constraints.
const ts = s => {
 if(s.$ref) return s.$ref.split('/').at(-1);
 if('const' in s) return JSON.stringify(s.const);
 if(s.enum) return s.enum.map(v=>JSON.stringify(v)).join(' | ');
 if(s.type==='object') return Object.keys(s.properties).length?`{ ${Object.entries(s.properties).map(([k,v])=>`${JSON.stringify(k)}${s.required.includes(k)?'':'?'}: ${ts(v)}`).join('; ')} }`:'Record<string, never>';
 if(s.oneOf||s.anyOf) return (s.oneOf||s.anyOf).map(v=>`(${ts(v)})`).join(' | ');
 if(s.type==='array') return `Array<${ts(s.items)}>`;
 if(s.type==='integer'||s.type==='number') return 'number';
 if(['string','boolean','null'].includes(s.type)) return s.type;
 throw new Error(`Unhandled schema ${JSON.stringify(s)}`);
};
let types='/** Generated by contracts/tools/generate-v2-schemas.mjs.\n * Runtime bounds, UTC/calendar, conditional and byte constraints require validation.\n * Durations: ms; RSSI: dBm; volumes: IPv4 bytes. No captured credentials/bodies.\n */\n';
types+=Object.entries(defs).map(([name,s])=>`export type ${name} = ${ts(s)};`).join('\n')+'\n';
types+='export type V2Envelope<T extends string, P> = {version: 2; type: T; device_id: V2Identifier; boot_id: V2Identifier; message_id: V2Identifier; seq: number; uptime_ms: number; payload: P} & ({time_quality: "unknown"; timestamp: null} | {time_quality: "app_sync" | "network_sync"; timestamp: V2UtcTimestamp});\n';
for(const [name,payload] of Object.entries({State:'V2StatePayload',Metrics:'V2MetricsPayload',Event:'V2EventPayload',Command:'V2CommandPayload',Ack:'V2AckPayload'})) types+=`export type V2${name}Message = V2Envelope<'${name.toLowerCase()}', ${payload}>;\n`;
types+='export type V2Message = V2StateMessage | V2MetricsMessage | V2EventMessage | V2CommandMessage | V2AckMessage;\n';
await writeFile(new URL('src/v2-types.ts',rootDir),types);
console.log(`Generated 7 schemas and ${Object.keys(defs).length+7} type exports in ${fileURLToPath(rootDir)}`);
