import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {Ajv2020} from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';

// Independent fixtures: this suite does not import the v2 codec or generator.
const ajv = new Ajv2020({strict:true,allErrors:true});
addFormats(ajv,{mode:'full'});
const root='https://sentinel.local/contracts/v2/';
for(const name of ['common','state','metrics','event','command','ack','metadata']) ajv.addSchema(JSON.parse(await readFile(new URL(`../schemas/v2/${name}.schema.json`,import.meta.url),'utf8')));
const validators=Object.fromEntries(['state','metrics','event','command','ack','metadata'].map(n=>[n,ajv.getSchema(`${root}${n}.schema.json`)]));
const check=(type,value,valid=true)=>assert.equal(validators[type](value),valid,`${type}: ${JSON.stringify(validators[type].errors)}`);
const clone=structuredClone;
const mutate=(v,path,value)=>{const c=clone(v);let obj=c;for(const key of path.slice(0,-1))obj=obj[key];obj[path.at(-1)]=value;return c;};
const message=(type,payload)=>({version:2,type,device_id:'node-test',boot_id:'boot-test',message_id:'msg-test',seq:1,uptime_ms:1000,timestamp:null,time_quality:'unknown',payload});
const target={ssid:'Lab ficticio',bssid:'02:00:00:00:00:01',channel:6};
const network={...target,rssi:-55,auth:'wpa2_personal',cipher:'ccmp',pmf:'unknown',seen_uptime_ms:900};
const coverage={channel:6,pause_reason:'none',started_uptime_ms:500,duration_ms:500,dropped_frames:0,dropped_events:0};
const service={available:true,status:'stopped'};
const capabilities=Object.fromEntries(['ble_control','mqtt_control','event_journal','oled','button','capture','detectors','honeypot_http','honeypot_telnet','notifications','sanitizador'].map(k=>[k,false]));
const state=(profile='idle')=>({profile,phase:'ready',revision:2,last_command_id:null,transport:{ble:'connected',wifi:'disconnected',mqtt:'disconnected'},peripherals:{oled:false,button:false},capabilities:clone(capabilities),error:null,module:clone({
 idle:{kind:'idle'},escudo:{kind:'escudo',scan_status:'idle',scan_id:null,target:null,capture_status:'stopped',coverage},
 sanitizador:{kind:'sanitizador',upstream:'disconnected',audit_status:'idle',audit_job_id:null,gateway_status:'stopped',ap:service,dhcp:service,nat:service,dns:service,doh:service,client_count:0,rule_count:0},
 cebo:{kind:'cebo',http:service,telnet:service,active_connections:0,max_connections:6,idle_timeout_ms:10000}
 }[profile])});
const auditCheck={status:'ok',evidence:['dns_answer'],response_ms:12};
const audit={job_id:'cmd-test',dns:auditCheck,https:{status:'ok',evidence:['https_valid'],response_ms:30},portal:{status:'inconclusive',evidence:['not_run'],response_ms:null},connectivity:'online',completed_uptime_ms:1000};
const client={mac:'02:00:00:00:00:02',ipv4:'192.0.2.2',rssi:-60,rssi_reason:null,ipv4_upload_bytes:123,ipv4_download_bytes:456,seen_uptime_ms:1000};
const rule={rule_id:'rule-test',domain:'example.invalid',action:'block',include_subdomains:true};
const page={after:null,limit:4};
const networkPage={scan_id:'cmd-test',networks:[network],next:null,more:false};
const clientPage={clients:[client],next:null,more:false};
const rulePage={rules:[rule],next:null,more:false};
const protocolMetrics={connections:1,accesses:0,auth_attempts:0,inputs:0,active_connections:1,interactions_per_minute:1.5,alerts:0};
const metricValues={
 system:{free_heap_bytes:10000,min_free_heap_bytes:5000,queue_count:0,journal_count:1,dropped_messages:0,dropped_events:0},
 escudo:{beacons:10,deauth:0,disassoc:0,alerts:{deauth_burst:0,suspicious_disconnect:0,evil_twin:0,karma_indication:0},channel:6,coverage_ms:1000,paused_ms:0,dropped_frames:0},
 sanitizador:{clients:1,ipv4_upload_bytes:123,ipv4_download_bytes:456,dns_queries:2,dns_blocked:1,dns_cache_hits:0,dns_failures:0,dns_latency_ms:20},
 cebo:{http:protocolMetrics,telnet:protocolMetrics,rejected_connections:0,alerts:0}
};
const commandArgs={
 get_state:{},set_mode:{mode:'sanitizador'},configure_connectivity:{ssid:'Lab ficticio',wifi_password:'fiction-only',mqtt_uri:'mqtt://broker.example.invalid:1883'},set_time:{timestamp:'2026-11-10T12:00:00Z'},read_events:{after_cursor:0,limit:8},
 'escudo.scan':{},'escudo.networks':page,'escudo.select_target':target,'escudo.capture_start':{},'escudo.capture_stop':{},
 'sanitizador.configure_upstream':{ssid:'Lab ficticio',wifi_password:''},'sanitizador.configure_ap':{ssid:'Lab AP',wifi_password:'fiction-only',ipv4:'192.0.2.1',netmask:'255.255.255.0'},'sanitizador.configure_doh':{endpoint:'https://dns.example.invalid/dns-query',ca_certificate:'fixture-public-ca'},'sanitizador.audit':{dns_name:'example.invalid',https_url:'https://example.invalid/',portal_url:'http://example.invalid/'},'sanitizador.audit_result':{job_id:'cmd-test'},'sanitizador.gateway_start':{},'sanitizador.gateway_stop':{},'sanitizador.clients':page,'sanitizador.rules.put':rule,'sanitizador.rules.remove':{rule_id:'rule-test'},'sanitizador.rules.list':page,
 'cebo.start':{},'cebo.stop':{},'cebo.configure':{http_enabled:true,telnet_enabled:true,max_connections:6,idle_timeout_ms:1000},'cebo.stats':{}
};
const command=(operation,args=commandArgs[operation])=>message('command',{command_id:'cmd-test',target_boot_id:'boot-test',expected_revision:2,expires_at_uptime_ms:30000,operation,args:clone(args)});
const detection={detector:'deauth_burst',...target,rssi:-55,auth:'wpa2_personal',cipher:'ccmp',pmf:'unknown',count:20,window_ms:1000,suspicion:'medium',evidence:['deauth_rate']};
const interaction={source_ipv4:'192.0.2.2',source_port:45000,destination_port:80,protocol:'http',interaction:'connect',path:null,count:1,window_ms:1000,suspicion:'informational',evidence:['connection_observed']};
const event=(name,details,profile=name.split('.')[0],severity='info',code=null)=>message('event',{journal_seq:1,origin:'simulated',name,profile,severity,code,details:clone(details)});
const eventFixtures=[
 event('escudo.networks',networkPage),event('escudo.target',{...target,capture_status:'stopped'}),event('escudo.detection',detection,'escudo','warning'),event('escudo.coverage',coverage),
 event('sanitizador.audit',audit),event('sanitizador.clients',clientPage),event('sanitizador.rules',rulePage),event('sanitizador.dns',{outcome:'blocked',domain:'example.invalid',client_ipv4:'192.0.2.2',cache_hit:false,doh:false,latency_ms:1,reason:'rule_block'}),event('cebo.interaction',interaction),
 event('system.status',{status_code:'recovered',resource:'dns'},'sanitizador'),event('system.booted',{},'idle'),event('system.profile_changed',{previous_profile:'idle'},'escudo'),event('system.error',{resource:'queue'},'cebo','critical','capacity_exceeded')
];
const results=[
 {kind:'state',state:state()}, {kind:'job',job_id:'cmd-test'}, {kind:'escudo.networks',...networkPage}, {kind:'escudo.target',target:{...target,capture_status:'stopped'}},
 {kind:'sanitizador.clients',...clientPage},{kind:'sanitizador.rules',...rulePage},{kind:'sanitizador.rule',rule},{kind:'sanitizador.rule_removed',rule_id:'rule-test'},{kind:'sanitizador.audit',audit},{kind:'sanitizador.audit',audit:null},
 {kind:'cebo.stats',interval_ms:1000,values:metricValues.cebo},{kind:'events',next_cursor:8,retained_from:1,retained_to:8,more:false,gap:false}
];
const ack=(status,result=null,error=null)=>message('ack',{command_id:'cmd-test',status,revision:2,error,result:clone(result)});

test('all v2 schemas compile strictly and preserve metadata fields',()=>{
 check('metadata',{version:2,device_id:'node-test',boot_id:'boot-test',revision:2,uptime_ms:1000,max_message_bytes:2048});
 check('metadata',{version:2,device_id:'node-test',boot_id:'boot-test',revision:2,uptime_ms:1000,max_message_bytes:4096},false);
});
test('all 25 exact operations accept their args and reject unknown or missing args',()=>{
 assert.equal(Object.keys(commandArgs).length,25);
 for(const op of Object.keys(commandArgs)){
  const c=command(op);check('command',c);
  check('command',mutate(c,['payload','args','unknown'],true),false);
  for(const key of Object.keys(commandArgs[op])){const bad=clone(c);delete bad.payload.args[key];check('command',bad,false);}
 }
 check('command',command('start_capture',{}),false);check('command',command('start_honeypot',{}),false);
});
test('command bounds, credentials shape, rules, UTC and optional hex SSID',()=>{
 for(const [op,key,value] of [
  ['escudo.networks','limit',5],['escudo.networks','limit',0],['sanitizador.clients','limit',5],['sanitizador.rules.list','limit',5],['read_events','limit',9],
  ['cebo.configure','http_enabled',false],['cebo.configure','telnet_enabled',false],['cebo.configure','max_connections',7],['cebo.configure','idle_timeout_ms',999],
  ['sanitizador.configure_ap','wifi_password','short'],['sanitizador.configure_ap','ipv4','999.1.1.1'],['sanitizador.configure_doh','endpoint','http://example.invalid/'],['sanitizador.configure_doh','endpoint','https://user:pass@example.invalid/'],['sanitizador.configure_doh','endpoint','https://example.invalid/?token=secret'],['sanitizador.configure_doh','ca_certificate','x'.repeat(1537)],
  ['sanitizador.rules.put','domain','UPPER.invalid'],['sanitizador.rules.put','domain','bad..invalid'],['sanitizador.rules.put','rule_id','x'.repeat(33)],['sanitizador.rules.put','action','redirect'],['set_mode','mode','unknown'],['escudo.select_target','channel',15],['escudo.select_target','bssid','bad'],['set_time','timestamp','2026-02-30T12:00:00Z'],['set_time','timestamp','2026-11-10T12:00:00+00:00']
 ])check('command',mutate(command(op),['payload','args',key],value),false);
 check('command',mutate(command('escudo.select_target'),['payload','args','ssid'],{encoding:'hex',value:'ff00fe'}));
 check('command',mutate(command('escudo.select_target'),['payload','args','ssid'],{encoding:'hex',value:'f'}),false);
});
test('all module states stay coherent, available services do not imply running',()=>{
 for(const p of ['idle','escudo','sanitizador','cebo'])check('state',message('state',state(p)));
 check('state',mutate(message('state',state('escudo')),['payload','module','kind'],'cebo'),false);
 check('state',mutate(message('state',state('escudo')),['payload','transport','mqtt'],'connected'),false);
 const esc=state('escudo');esc.module.capture_status='running';check('state',message('state',esc),false);esc.module.target=target;check('state',message('state',esc),false);esc.capabilities.capture=true;check('state',message('state',esc));
 const san=state('sanitizador');san.module.gateway_status='running';san.capabilities.sanitizador=true;check('state',message('state',san),false);san.module.upstream='connected';for(const k of ['ap','dhcp','nat','dns','doh'])san.module[k]={available:true,status:'running'};check('state',message('state',san));
 san.module.doh.status='error';check('state',message('state',san),false);
 const ceb=state('cebo');ceb.module.http={available:false,status:'running'};check('state',message('state',ceb),false);ceb.module.http={available:true,status:'running'};check('state',message('state',ceb),false);ceb.capabilities.honeypot_http=true;check('state',message('state',ceb));
});
test('all four metric scopes use bounded values, units and closed scope-specific counters',()=>{
 for(const [scope,values] of Object.entries(metricValues)){
  const m=message('metrics',{scope,interval_ms:1000,values});check('metrics',m);
  check('metrics',mutate(m,['payload','interval_ms'],-1),false);check('metrics',mutate(m,['payload','values','throughput_rf'],123),false);
 }
 check('metrics',message('metrics',{scope:'system',interval_ms:1000,values:metricValues.escudo}),false);
 check('metrics',mutate(message('metrics',{scope:'sanitizador',interval_ms:1000,values:metricValues.sanitizador}),['payload','values','ipv4_upload_bytes'],1.5),false);
});
test('all 13 event variants, four detectors and benign/suspicious interactions',()=>{
 for(const e of eventFixtures)check('event',e);
 for(const [detector,evidence] of [['deauth_burst',['deauth_rate']],['suspicious_disconnect',['disconnect_sequence']],['evil_twin',['same_ssid','different_bssid','security_downgrade']],['karma_indication',['unsolicited_probe_response','multiple_ssids','passive_only']]])check('event',event('escudo.detection',{...detection,detector,evidence,related_bssid:'02:00:00:00:00:02'},'escudo','warning'));
 for(const [suspicion,severity] of [['informational','info'],['medium','warning'],['high','critical']]){
  check('event',event('cebo.interaction',{...interaction,suspicion},'cebo',severity));
  check('event',event('cebo.interaction',{...interaction,suspicion},'cebo',severity==='info'?'critical':'info'),false);
 }
 check('event',event('cebo.interaction',{...interaction,protocol:'telnet',destination_port:23,path:null,interaction:'auth_attempt',suspicion:'medium',evidence:['auth_attempt']},'cebo','warning'));
 check('event',event('cebo.interaction',{...interaction,protocol:'telnet',destination_port:23,path:'/login'},'cebo'),false);
 check('event',event('escudo.detection',{...detection,detector:'attacker_identified'},'escudo'),false);
 check('event',event('escudo.detection',{...detection,evidence:['attacker_confirmed']},'escudo'),false);
 for(const field of ['attacker','password','username','token','cookie','body','url'])check('event',event('cebo.interaction',{...interaction,[field]:'private'},'cebo'),false);
 check('event',mutate(eventFixtures[0],['payload','profile'],'cebo'),false);
 check('event',mutate(eventFixtures[4],['payload','details','dns','status'],'secure'),false);
 check('event',mutate(eventFixtures[7],['payload','details','reason'],'allowed'),false);
});
test('catalog/client/rule pages are at most four records; history is separate and at most eight',()=>{
 for(const [name,details,key,item] of [['escudo.networks',networkPage,'networks',network],['sanitizador.clients',clientPage,'clients',client],['sanitizador.rules',rulePage,'rules',rule]]){
  check('event',event(name,{...details,[key]:Array.from({length:4},()=>item)}));
  check('event',event(name,{...details,[key]:Array.from({length:5},()=>item)}),false);
  check('event',event(name,{...details,next:'snapshot:4',more:true}));
  check('event',event(name,{...details,next:null,more:true}),false);
  check('event',event(name,{...details,next:'snapshot:4',more:false}),false);
 }
 check('event',event('sanitizador.clients',{...clientPage,clients:[{...client,rssi:null,rssi_reason:'driver_unsupported'}]}));
 check('event',event('sanitizador.clients',{...clientPage,clients:[{...client,rssi:null,rssi_reason:null}]}),false);
 check('ack',ack('applied',{kind:'events',next_cursor:8,retained_from:1,retained_to:8,more:true,gap:true}));
 check('ack',ack('applied',{kind:'events',next_cursor:8,retained_from:1,retained_to:8,more:true,gap:true,events:[]}),false);
});
test('ACKs cover all typed results, asynchronous starts, null accepted and explicit rejection',()=>{
 for(const result of results)check('ack',ack('applied',result));
 check('ack',ack('applied'));check('ack',ack('accepted'));check('ack',ack('rejected',null,'invalid_state'));
 check('ack',ack('accepted',{kind:'job',job_id:'cmd-test'}),false);
 check('ack',ack('rejected'),false);check('ack',ack('rejected',results[0],'invalid_state'),false);
 check('ack',ack('applied',null,'timeout'),false);check('ack',ack('applied',{kind:'arbitrary',body:'text'}),false);
 check('ack',ack('rejected',null,'invented_error'),false);
});
test('UTC, IDs, safe integers and envelope fields preserve v1 restrictions',()=>{
 const c=command('get_state');
 for(const [field,value] of [['version',1],['device_id','☃'],['boot_id',''],['message_id','x'.repeat(65)],['seq',Number.MAX_SAFE_INTEGER+1],['uptime_ms',-1],['seq',1.5],['unknown',true],['timestamp','2026-11-10T12:00:00Z'],['time_quality','network_sync']])check('command',mutate(c,[field],value),false);
 const synced={...c,time_quality:'app_sync',timestamp:'2026-11-10T12:00:00.123Z'};check('command',synced);
 for(const timestamp of ['0000-01-01T00:00:00Z','2026-02-29T00:00:00Z','2026-11-10T12:00:60Z','2026-11-10T12:00:00+00:00'])check('command',{...synced,timestamp},false);
 check('command',mutate(c,['payload','expected_revision'],Number.MAX_SAFE_INTEGER));
});
test('UTF-8 byte ceilings and context are explicitly reserved for semantic codec validation',()=>{
 // JSON Schema maxLength counts code points. These shapes pass here; the
 // codec must reject the 64-byte SSID and the >2048-byte serialized command.
 const unicode=command('sanitizador.configure_upstream',{ssid:'é'.repeat(32),wifi_password:''});
 check('command',unicode);assert.equal(Buffer.byteLength(unicode.payload.args.ssid),64);
 const large=command('sanitizador.configure_doh');
 large.payload.args.ca_certificate='é'.repeat(1536);
 check('command',large);assert.ok(Buffer.byteLength(JSON.stringify(large))>2048);
 // Identity correlation, authorized transport/destinations, target observation,
 // event retention order, and job_id equality need live command/node context.
 // No nested events are packed in a history ACK; limit 8 is on read_events.
});
test('identifiers, domains, catalog cursors, MACs and UTC reject final line terminators',()=>{
 for(const suffix of ['\n','\r','\r\n','\u2028','\u2029']){
  for(const field of ['device_id','boot_id','message_id'])check('command',mutate(command('get_state'),[field],`node-test${suffix}`),false);
  for(const field of ['command_id','target_boot_id'])check('command',mutate(command('get_state'),['payload',field],`cmd-test${suffix}`),false);
  check('command',mutate(command('sanitizador.rules.put'),['payload','args','rule_id'],`rule-test${suffix}`),false);
  check('command',mutate(command('sanitizador.rules.put'),['payload','args','domain'],`example.invalid${suffix}`),false);
  check('command',mutate(command('sanitizador.audit'),['payload','args','dns_name'],`example.invalid${suffix}`),false);
  check('command',mutate(command('escudo.networks'),['payload','args','after'],`snapshot:4${suffix}`),false);
  check('event',event('escudo.networks',{...networkPage,next:`snapshot:4${suffix}`,more:true}),false);
  check('command',mutate(command('escudo.select_target'),['payload','args','bssid'],`${target.bssid}${suffix}`),false);
  check('command',mutate(command('escudo.select_target'),['payload','args','ssid'],{encoding:'hex',value:`ff${suffix}`}),false);
  check('command',mutate(command('set_time'),['payload','args','timestamp'],`2026-11-10T12:00:00Z${suffix}`),false);
  check('command',mutate(command('configure_connectivity'),['payload','args','mqtt_uri'],`mqtt://broker.example.invalid:1883${suffix}`),false);
 }
});
test('all fixture objects are closed recursively including nested results and details',()=>{
 const corpus=[...Object.keys(commandArgs).map(k=>command(k)),...eventFixtures,...results.map(r=>ack('applied',r)),...['idle','escudo','sanitizador','cebo'].map(p=>message('state',state(p))),...Object.entries(metricValues).map(([scope,values])=>message('metrics',{scope,interval_ms:1000,values}))];
 const paths=(value,path=[])=>{
  if(value===null||typeof value!=='object')return [];
  return [...(Array.isArray(value)?[]:[path]),...Object.entries(value).flatMap(([k,v])=>paths(v,[...path,k]))];
 };
 for(const m of corpus)for(const path of paths(m))check(m.type,mutate(m,[...path,'unexpected_private_field'],'redacted'),false);
});
