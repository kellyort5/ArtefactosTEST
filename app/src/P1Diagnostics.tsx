import {useEffect,useState} from 'react';
import {useStore} from 'zustand';
import {controller,type DiagnosticFault} from './controller';
import {nodeStore} from './store';
import {type NodeCommand,type NodeMessage,type NodeRequest,validateMessage} from './protocol';
export const p1ToolsEnabled=import.meta.env?.VITE_SENTINEL_P1_TOOLS==='1';
const presets:Record<string,NodeRequest>={
 'Estado':{operation:'get_state',args:{}},
 'Escudo':{operation:'set_mode',args:{mode:'escudo'}},
 'Reposo':{operation:'set_mode',args:{mode:'idle'}},
 'Escanear':{operation:'escudo.scan',args:{}},
 'Catálogo':{operation:'escudo.networks',args:{after:null,limit:4}},
 'Seleccionar objetivo':{operation:'escudo.select_target',args:{ssid:'MI_RED',bssid:'00:11:22:33:44:55',channel:1}},
 'Iniciar captura':{operation:'escudo.capture_start',args:{}},
 'Detener captura':{operation:'escudo.capture_stop',args:{}},
 'Historial':{operation:'read_events',args:{after_cursor:0,limit:8}},
 'Cebo ausente (negativo)':{operation:'set_mode',args:{mode:'cebo'}},
 'Sanitizador ausente (negativo)':{operation:'set_mode',args:{mode:'sanitizador'}},
};
export function publicP1Request(operation:string,args:string):NodeRequest {
 if(!Object.values(presets).some(p=>p.operation===operation))throw new Error('Operación pública P1 no permitida.');
 try {
  const parsed=JSON.parse(args);
  // Schema validation is performed before transport using fresh metadata.
  const request={operation,args:parsed} as NodeRequest;
  if(!parsed||typeof parsed!=='object'||Array.isArray(parsed))throw new Error();
  return request;
 }catch{throw new Error('Argumentos JSON no válidos; usa el contrato público de la operación.');}
}
export default function P1Diagnostics(){
 const state=useStore(nodeStore);
 const [choice,setChoice]=useState('Estado'),[args,setArgs]=useState('{}');
 const [fault,setFault]=useState<DiagnosticFault>('normal');
 const [busy,setBusy]=useState(false),[feedback,setFeedback]=useState(''),[messages,setMessages]=useState<NodeMessage[]>([]);
 const [last,setLast]=useState<NodeCommand|null>(null);
 const ready=state.links.ble==='connected'&&state.metadata?.version===2;
 const identity=`${state.deviceId}/${state.metadata?.boot_id}/${state.links.ble}`;
 useEffect(()=>{setMessages([]);setLast(null);setFeedback('');},[identity]);
 useEffect(()=>controller.observeMessages((m,c)=>{
  if(c!=='ble'||m.type==='command'||!['ack','event','metrics'].includes(m.type))return;
  // Only expose ACKs of the closed public P1 operation set, never provisioning.
  if(m.type==='ack'){
   const command=nodeStore.getState().commands.find(v=>v.id===m.payload.command_id);
   if(!command||!Object.values(presets).some(p=>p.operation===command.operation))return;
  }
  setMessages(old=>[validateMessage(m),...old].slice(0,20));
 }),[]);
 async function execute(replay=false){
  setBusy(true);setFeedback('');
  try{
   const id=replay&&last?await controller.replayDiagnostic(last,p1ToolsEnabled):
    await controller.sendDiagnostic(publicP1Request(presets[choice].operation,args),fault,p1ToolsEnabled);
   const command=nodeStore.getState().commands.find(c=>c.id===id)?.correlation;
   if(command)setLast(structuredClone(command));
   setFeedback(`Envío ${id}. Espera ACK aplicado/rechazado; enviado no significa aplicado.`);
  }catch{setFeedback('No se pudo enviar. Revisa BLE, versión v2, JSON, arranque y plazo. No se reintenta automáticamente.');}
  finally{setBusy(false);}
 }
 if(!p1ToolsEnabled)return null;
 return <section className="card" aria-label="Diagnóstico P1 BLE">
  <p className="eyebrow">PRUEBAS PERSONA 1 · BLE REAL</p><h2>Diagnóstico P1</h2>
  <p className="notice">Herramienta de laboratorio. Usa únicamente tu red autorizada. Los negativos y reenvíos son deliberados; no habilitan módulos ausentes.</p>
  <p className="muted">BLE {state.links.ble} · boot {state.metadata?.boot_id??'—'} · revisión {state.metadata?.revision??'—'}</p>
  <label className="field"><span>Operación</span><select value={choice} onChange={e=>{setChoice(e.target.value);setArgs(JSON.stringify(presets[e.target.value].args,null,2));setFault('normal');}}>{Object.keys(presets).map(name=><option key={name}>{name}</option>)}</select></label>
  <label className="field"><span>Argumentos públicos (JSON). Catálogo: copia next en after; objetivo: copia ssid/bssid/channel observados.</span><textarea rows={6} value={args} onChange={e=>setArgs(e.target.value)} spellCheck={false}/></label>
  <label className="field"><span>Sobre: normal o prueba negativa explícita</span><select value={fault} onChange={e=>setFault(e.target.value as DiagnosticFault)}><option value="normal">Normal (metadatos frescos)</option><option value="boot">Boot incorrecto</option><option value="revision">Revisión incorrecta</option><option value="expired">Caducado</option></select></label>
  <div className="card-actions"><button disabled={!ready||busy} onClick={()=>void execute()}>Enviar por BLE</button><button className="subtle" disabled={!ready||busy||!last} onClick={()=>void execute(true)}>Reenviar última orden exacta</button><button className="subtle" onClick={()=>{setMessages([]);setLast(null);setFeedback('');}}>Limpiar diagnóstico</button></div>
  <p role="status">{feedback}</p>
  <details><summary>Estado completo y métricas actuales</summary><pre style={{overflowX:'auto',maxHeight:400}}>{JSON.stringify({state:state.snapshot,metrics:state.metrics},null,2)}</pre></details>
  <details open><summary>Últimas 20 respuestas BLE (efímeras)</summary><pre style={{overflowX:'auto',maxHeight:500}}>{JSON.stringify(messages,null,2)}</pre></details>
 </section>;
}
