import 'fake-indexeddb/auto';
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {EventJournal} from '../src/journal';
import {readFileSync} from 'node:fs';
import type {EventMessage} from '@sentinel/contracts';
const base=JSON.parse(readFileSync(new URL('../../contracts/fixtures/event.valid.json',import.meta.url),'utf8')) as EventMessage;
const event=(n:number):EventMessage=>({...base,message_id:`event-${n}`,payload:{...base.payload,journal_seq:n}});
test('duplicate event, app reopen, ring256 and durable gap/cursor',async()=>{
 const name=`journal-${crypto.randomUUID()}`,a=new EventJournal(name);
 await a.append(event(1));await a.append(event(1));assert.equal((await a.read(base.device_id)).events.length,1);
 assert.equal((await a.read(base.device_id)).cursor,0); // live events never skip missing pages
 await a.confirmPage(base.device_id,{next_cursor:1,retained_from:1,retained_to:1,more:false,gap:false},0);
 const b=new EventJournal(name);assert.equal((await b.read(base.device_id)).cursor,1);
 for(let n=2;n<=258;n++)await b.append(event(n));
 const h=await b.read(base.device_id);assert.equal(h.events.length,256);assert.equal(h.events.at(-1)?.payload.journal_seq,3);
 await b.confirmPage(base.device_id,{next_cursor:10,retained_from:3,retained_to:258,more:true,gap:true},1);
 const c=await new EventJournal(name).read(base.device_id);assert.equal(c.cursor,10);assert.equal(c.gap?.gap,true);
 await b.append(event(258));assert.equal((await b.read(base.device_id)).events.length,256);
});
test('incomplete page does not advance cursor; repetition repairs it',async()=>{
 const a=new EventJournal(`journal-${crypto.randomUUID()}`),page={next_cursor:2,retained_from:1,retained_to:2,more:false,gap:false};
 await a.append(event(2));await assert.rejects(()=>a.confirmPage(base.device_id,page,0));assert.equal((await a.read(base.device_id)).cursor,0);
 await a.append(event(1));await a.confirmPage(base.device_id,page,0);assert.equal((await a.read(base.device_id)).cursor,2);
});

test('reported internal gap advances past invalid slots without an endless retry',async()=>{
 const a=new EventJournal(`journal-${crypto.randomUUID()}`),page={next_cursor:3,retained_from:1,retained_to:3,more:false,gap:true};
 await a.append(event(1));await assert.rejects(()=>a.confirmPage(base.device_id,page,0));
 await a.append(event(3));await a.confirmPage(base.device_id,page,0);
 const h=await a.read(base.device_id);assert.equal(h.cursor,3);assert.equal(h.gap?.gap,true);assert.deepEqual(h.events.map(e=>e.payload.journal_seq),[3,1]);
});
