import {decodeMessage,type EventMessage,type PageResult} from '@sentinel/contracts';
export type JournalSnapshot={events:EventMessage[];cursor:number;gap:PageResult|null};
const request=<T>(r:IDBRequest<T>)=>new Promise<T>((resolve,reject)=>{r.onsuccess=()=>resolve(r.result);r.onerror=()=>reject(new Error('No se pudo leer el historial local.'));});
const completed=(t:IDBTransaction)=>new Promise<void>((resolve,reject)=>{t.oncomplete=()=>resolve();t.onabort=()=>reject(new Error('No se pudo guardar el historial local.'));t.onerror=()=>reject(new Error('No se pudo guardar el historial local.'));});
export class EventJournal {
 private database?:Promise<IDBDatabase>;
 constructor(private name='sentinel-journal-v1'){}
 private open(){
  return this.database??=new Promise<IDBDatabase>((resolve,reject)=>{
   const r=indexedDB.open(this.name,1);
   r.onupgradeneeded=()=>{const db=r.result;const events=db.createObjectStore('events',{keyPath:['device_id','message_id']});events.createIndex('node','device_id');db.createObjectStore('cursors',{keyPath:'device_id'});};
   r.onsuccess=()=>{const db=r.result;db.onversionchange=()=>{db.close();this.database=undefined;};resolve(db);};r.onerror=()=>{this.database=undefined;reject(new Error('IndexedDB no disponible. Revisa almacenamiento de la app.'));};
  });
 }
 async append(event:EventMessage){
  // Only validated, public event envelopes ever enter IndexedDB.
  if(decodeMessage(JSON.stringify(event)).type!=='event')throw new Error('Evento inválido.');
  const db=await this.open();const tx=db.transaction('events','readwrite');const done=completed(tx);const store=tx.objectStore('events');
  const prior=await request(store.get([event.device_id,event.message_id]));
  if(!prior)store.put(event);
  const records=await request(store.index('node').getAll(event.device_id)) as EventMessage[];
  records.sort((a,b)=>b.payload.journal_seq-a.payload.journal_seq);
  for(const old of records.slice(256))store.delete([old.device_id,old.message_id]);
  await done;
 }
 async confirmPage(deviceId:string,page:PageResult,afterCursor:number){
  const history=await this.read(deviceId);
  const first=Math.max(afterCursor+1,page.retained_from);
  if(page.retained_from>page.retained_to||page.next_cursor<afterCursor||(!page.gap&&page.next_cursor-first>=8))throw new Error('Página inválida.');
  const available=new Set(history.events.map(e=>e.payload.journal_seq));
  if(page.gap){
   // A recovered journal may have holes from invalid slots. The node explicitly reports that loss.
   if(page.next_cursor>afterCursor&&!available.has(page.next_cursor))throw new Error('Página incompleta: falta el evento del cursor final.');
  }else{
   for(let cursor=first;cursor<=page.next_cursor;cursor++)if(!available.has(cursor))throw new Error('Página incompleta: repite la consulta.');
  }
  const db=await this.open();const tx=db.transaction('cursors','readwrite');const done=completed(tx);const store=tx.objectStore('cursors');
  const old=await request(store.get(deviceId)) as {cursor:number;gap:PageResult|null}|undefined;
  store.put({device_id:deviceId,cursor:Math.max(old?.cursor??0,page.next_cursor),gap:page.gap?page:(old?.gap??null)});await done;
 }
 async read(deviceId:string):Promise<JournalSnapshot>{
  const db=await this.open();const tx=db.transaction(['events','cursors'],'readonly');const done=completed(tx);
  const [events,meta]=await Promise.all([request(tx.objectStore('events').index('node').getAll(deviceId)),request(tx.objectStore('cursors').get(deviceId))]);await done;
  const safe:EventMessage[]=[];
  for(const raw of events){try{const m=decodeMessage(JSON.stringify(raw));if(m.type==='event'&&m.device_id===deviceId)safe.push(m);}catch{/* Ignore corrupt local entries; do not erase source device journal. */}}
  return {events:safe.sort((a,b)=>b.payload.journal_seq-a.payload.journal_seq).slice(0,256),cursor:meta?.cursor??0,gap:meta?.gap??null};
 }
}
