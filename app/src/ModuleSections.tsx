import type {NodeState,NodeMetrics,NodeEvent,Profile} from './protocol';
export const moduleLabels:Record<Exclude<Profile,'idle'>,string>={escudo:'Escudo Pasivo',sanitizador:'Auditor y Sanitizador',cebo:'Cebo de Red Local'};
export function ModuleSection({profile,snapshot,metrics}:{profile:Exclude<Profile,'idle'>;snapshot:NodeState|null;metrics:Record<string,NodeMetrics>}) {
 const label=moduleLabels[profile];
 if(snapshot?.version!==2)return <section className="card"><h2>{label}</h2><p>Este módulo requiere un nodo con protocolo v2.</p></section>;
 const p=snapshot.payload;
 if(p.profile!==profile)return <section className="card"><h2>{label}</h2><p>Modo detenido. El estado del módulo aparecerá cuando el nodo confirme su activación.</p></section>;
 const m=p.module;
 return <section className="card" aria-label={`Estado de ${label}`}><h2>{label}</h2>
  {m.kind==='escudo'&&<><dl className="facts"><div><dt>Captura</dt><dd>{m.capture_status}</dd></div><div><dt>Exploración</dt><dd>{m.scan_status}</dd></div><div><dt>Objetivo</dt><dd>{m.target?(typeof m.target.ssid==='string'?m.target.ssid||'(oculta)':`hex:${m.target.ssid.value}`):'Sin seleccionar'}</dd></div><div><dt>BSSID</dt><dd>{m.target?.bssid??'Sin seleccionar'}</dd></div><div><dt>Canal observado</dt><dd>{m.coverage.channel??'Sin captura'}</dd></div><div><dt>Cobertura</dt><dd>{m.coverage.pause_reason} · {m.coverage.duration_ms} ms</dd></div></dl></>}
  {m.kind==='sanitizador'&&<dl className="facts"><div><dt>Auditoría</dt><dd>{m.audit_status}</dd></div><div><dt>Pasarela</dt><dd>{m.gateway_status}</dd></div><div><dt>Upstream</dt><dd>{m.upstream}</dd></div><div><dt>Clientes / reglas</dt><dd>{m.client_count} / {m.rule_count}</dd></div><div><dt>DNS / DoH</dt><dd>{m.dns.status} / {m.doh.status}</dd></div></dl>}
  {m.kind==='cebo'&&<dl className="facts"><div><dt>HTTP</dt><dd>{m.http.status}</dd></div><div><dt>Telnet</dt><dd>{m.telnet.status}</dd></div><div><dt>Conexiones activas</dt><dd>{m.active_connections} / {m.max_connections}</dd></div></dl>}
  {metrics[profile]?.version===2&&<p className="muted">Métricas recibidas del mismo arranque; intervalo {metrics[profile].version===2?metrics[profile].payload.interval_ms:0} ms.</p>}
 </section>;
}
export function eventSummary(event:NodeEvent):string {
 if(event.version===1)return event.payload.code??'Evento de base v1';
 const p=event.payload;
 switch(p.name){
  case 'escudo.detection':return `Indicio ${p.details.detector} · ${p.details.bssid} · canal ${p.details.channel} · ${p.details.evidence.join(', ')}`;
  case 'escudo.coverage':return `Cobertura: ${p.details.pause_reason} · ${p.details.duration_ms} ms`;
  case 'cebo.interaction':return `${p.details.protocol} · ${p.details.source_ipv4}:${p.details.source_port} · ${p.details.interaction}`;
  case 'sanitizador.audit':return `Auditoría ${p.details.job_id} · ${p.details.connectivity}`;
  case 'system.status':return `${p.details.resource}: ${p.details.status_code}`;
  default:return p.code??'Evento del nodo';
 }
}
