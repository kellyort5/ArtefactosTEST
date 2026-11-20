import {decodeAnyMessage,eventIdentity,type PageResult} from '@sentinel/contracts';
import type {NodeEvent as EventMessage} from './protocol';
export type JournalSnapshot={events:EventMessage[];cursor:number;gap:PageResult|null};
export type ArchivedHistory=JournalSnapshot&{id:string;device_id:string;archived_at:string};
const request=<T>(r:IDBRequest<T>)=>new Promise<T>((resolve,reject)=>{r.onsuccess=()=>resolve(r.result);r.onerror=()=>reject(new Error('No se pudo leer el historial local.'));});
const completed=(t:IDBTransaction)=>new Promise<void>((resolve,reject)=>{t.oncomplete=()=>resolve();t.onabort=()=>reject(new Error('No se pudo guardar el historial local.'));t.onerror=()=>reject(new Error('No se pudo guardar el historial local.'));});
export class EventJournal {
 private database?:Promise<IDBDatabase>;
 constructor(private name='sentinel-journal-v1'){}
 private open(){
  return this.database??=new Promise<IDBDatabase>((resolve,reject)=>{
   const r=indexedDB.open(this.name,2);
   r.onupgradeneeded=()=>{const db=r.result;if(!db.objectStoreNames.contains('events')){const events=db.createObjectStore('events',{keyPath:['device_id','message_id']});events.createIndex('node','device_id');db.createObjectStore('cursors',{keyPath:'device_id'});}if(!db.objectStoreNames.contains('archives'))db.createObjectStore('archives',{keyPath:'id'}).createIndex('node','device_id');};
   r.onsuccess=()=>{const db=r.result;db.onversionchange=()=>{db.close();this.database=undefined;};resolve(db);};r.onerror=()=>{this.database=undefined;reject(new Error('IndexedDB no disponible. Revisa almacenamiento de la app.'));};
  });
 }
 async archives(deviceId:string):Promise<ArchivedHistory[]>{
  const db=await this.open(),tx=db.transaction('archives','readonly'),done=completed(tx);
  const records=await request(tx.objectStore('archives').index('node').getAll(deviceId));await done;
  return records.sort((a:ArchivedHistory,b:ArchivedHistory)=>b.archived_at.localeCompare(a.archived_at));
 }
 // Explicit USB reinitialization only. Archive and cursor reset are one transaction;
 // normal reconnections and reboots never call this method.
 async archiveAfterUsb(deviceId:string){
  const db=await this.open(),tx=db.transaction(['events','cursors','archives'],'readwrite'),done=completed(tx);
  const eventsStore=tx.objectStore('events'),cursors=tx.objectStore('cursors');
  const [events,meta]=await Promise.all([request(eventsStore.index('node').getAll(deviceId)),request(cursors.get(deviceId))]);
  if(events.length||meta)tx.objectStore('archives').put({id:crypto.randomUUID(),device_id:deviceId,archived_at:new Date().toISOString(),events,cursor:meta?.cursor??0,gap:meta?.gap??null});
  for(const event of events)eventsStore.delete([deviceId,event.message_id]);
  cursors.delete(deviceId);await done;
 }
 async append(event:EventMessage){
  // Only validated, public event envelopes ever enter IndexedDB.
  event=decodeAnyMessage(JSON.stringify(event)) as EventMessage;
  eventIdentity(event);
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
  for(const raw of events){try{const m=decodeAnyMessage(JSON.stringify(raw));if(m.type==='event'&&m.device_id===deviceId)safe.push(m);}catch{/* Ignore corrupt local entries; do not erase source device journal. */}}
  return {events:safe.sort((a,b)=>b.payload.journal_seq-a.payload.journal_seq).slice(0,256),cursor:meta?.cursor??0,gap:meta?.gap??null};
 }
}
