#!/usr/bin/env python3
"""Check messages produced by the real component against the team's codec."""
import pathlib,subprocess,tempfile
repo=pathlib.Path(__file__).resolve().parents[3]
subprocess.run(['python3',str(repo/'firmware/test/escudo/run_host.py')],cwd=repo,check=True)
subprocess.run(['npm','run','build','-w','contracts'],cwd=repo,check=True)
wires=[]
for name in ('discovery_test','capture_test','detectors_test'):
    wires.append(subprocess.check_output([str(pathlib.Path(tempfile.gettempdir())/'sentinel-escudo-host'/name),'wire'],cwd=repo))
for phase in ('wire','wire-gap'):
    with tempfile.TemporaryDirectory(prefix='sentinel-recovery-wire-') as directory:
        binary=pathlib.Path(tempfile.gettempdir())/'sentinel-escudo-host/recovery_test'
        wires.append(subprocess.check_output([str(binary),phase,directory],cwd=repo))
node=r'''
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {decodeModuleMessage,checkModuleAckForCommand,makeModuleCommand,eventIdentity} from '@sentinel/contracts';
let text='';for await(const part of process.stdin)text+=part;
const commands=new Map();let count=0;
const history=new Map(),received=new Set(),boots=new Set();
let repeated=0,recovered=0,pages=0,gaps=0;
for(const line of text.trim().split('\n')) {
  const message=decodeModuleMessage(line);count++;
  if(message.type==='command') {
    commands.set(message.payload.command_id,message);
    if(message.payload.operation==='read_events')received.clear();
  }
  if(message.type==='event' && message.device_id==='node-recovery') {
    assert.equal(message.payload.origin,'simulated');
    const identity=eventIdentity(message),prior=history.get(identity);
    if(prior){assert.deepEqual(prior,message);repeated++;}
    history.set(identity,message);received.add(message.payload.journal_seq);
    boots.add(message.boot_id);recovered++;
  }
  if(message.type==='ack') {
    const command=commands.get(message.payload.command_id);assert(command);
    assert.equal(checkModuleAckForCommand(command,message),null);
    if(command.payload.operation==='read_events' && message.payload.status==='applied') {
      const result=message.payload.result;
      assert.equal(result.kind,'events');pages++;
      if(result.gap){gaps++;assert(result.retained_from>1);}
      else for(let c=Math.max(command.payload.args.after_cursor+1,result.retained_from);c<=result.next_cursor;c++)
        assert(received.has(c),'A page ACK alone cannot confirm missing events');
    }
  }
}
if(recovered) {
  assert(pages>0);
  if(boots.has('boot-offline')) {
    assert(boots.has('boot-reconnected'));assert(repeated>0);
    assert(history.size<recovered,'Repeated delivery must keep identity');
    console.log(`Recovery cross-boot identity/dedup and complete pages: PASS (${history.size} unique, ${repeated} repeats)`);
  }
  if(boots.has('boot-gap'))assert(gaps>0);
}
const meta={version:2,device_id:'node-example',boot_id:'boot-example',revision:1,uptime_ms:1000,max_message_bytes:2048};
let examples=0;
for(const doc of ['docs/modules/escudo-discovery.md','docs/modules/escudo-capture.md','docs/modules/escudo-detectors.md','docs/modules/escudo-recovery.md'])
for(const [,body] of readFileSync(doc,'utf8').matchAll(/```json\n([\s\S]*?)\n```/g)) {
  makeModuleCommand(meta,JSON.parse(body),`doc-example-${++examples}`);
}
assert(examples>=5);
console.log(`Documentation requests -> v2 contract: PASS (${examples})`);
console.log(`Escudo C++ -> shared v2 codec and ACK correlation: PASS (${count} messages; host)`);
'''
for wire in wires:
    subprocess.run(['node','--input-type=module','-e',node],input=wire,cwd=repo,check=True)
