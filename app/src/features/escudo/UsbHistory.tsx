import {useEffect,useState} from 'react';
import {useStore} from 'zustand';
import {controller} from '../../controller';
import {nodeStore} from '../../store';
import type {ArchivedHistory} from '../../journal';
import {eventSummary} from '../../ModuleSections';
export default function UsbHistory(){
 const state=useStore(nodeStore),[archives,setArchives]=useState<ArchivedHistory[]>([]),[confirmed,setConfirmed]=useState(false),[busy,setBusy]=useState(false);
 const id=state.deviceId;
 useEffect(()=>{let active=true;setArchives([]);setConfirmed(false);if(id)void controller.journal.archives(id).then(items=>{if(active)setArchives(items);}).catch(()=>{if(active)state.setError('No se pudieron leer los historiales anteriores.');});return()=>{active=false;};},[id,state.setError]);
 async function reset(){
  setBusy(true);
  try{await controller.restartHistoryAfterUsb();if(id)setArchives(await controller.journal.archives(id));setConfirmed(false);}
  catch(e){state.setError(e instanceof Error?e.message:'No se pudo separar el historial.');}finally{setBusy(false);}
 }
 return <details className="escudo-usb"><summary>Después de reinstalar el firmware por USB</summary><p>Solo si la carga USB reinicializó los datos: desconecta BLE y MQTT, guarda el historial anterior con esta acción y vuelve a emparejar/conectar el nodo. La siguiente recuperación comenzará en cero. No uses esta acción tras un reinicio o corte normal.</p><label><input type="checkbox" checked={confirmed} onChange={e=>setConfirmed(e.target.checked)}/> Confirmo que reinstalé por USB y el historial del nodo se reinicializó.</label><p><button disabled={!id||!confirmed||busy||Object.values(state.links).some(v=>v==='connected'||v==='connecting')} onClick={()=>void reset()}>Guardar historial anterior e iniciar uno nuevo</button></p><p className="muted">Las copias siguientes permanecen en este dispositivo, separadas del historial actual.</p>{archives.map(a=><details key={a.id}><summary>Anterior · {new Date(a.archived_at).toLocaleString()} · {a.events.length} eventos</summary><ul>{a.events.map(e=><li key={e.message_id}>{eventSummary(e)} · {e.timestamp??`uptime ${e.uptime_ms/1000}s`} · {e.time_quality} · {e.boot_id} · {e.version===2?e.payload.origin:'v1'}<details><summary>Registro original</summary><pre>{JSON.stringify(e,null,2)}</pre></details></li>)}</ul></details>)}</details>;
}
