#!/usr/bin/env python3
"""Check messages produced by the real component against the team's codec."""
import pathlib,subprocess,tempfile
repo=pathlib.Path(__file__).resolve().parents[3]
subprocess.run(['python3',str(repo/'firmware/test/escudo/run_host.py')],cwd=repo,check=True)
subprocess.run(['npm','run','build','-w','contracts'],cwd=repo,check=True)
wire=subprocess.check_output([str(pathlib.Path(tempfile.gettempdir())/'sentinel-escudo-host/discovery_test'),'wire'],cwd=repo)
node=r'''
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {decodeModuleMessage,checkModuleAckForCommand,makeModuleCommand} from '@sentinel/contracts';
let text='';for await(const part of process.stdin)text+=part;
const commands=new Map();let count=0;
for(const line of text.trim().split('\n')) {
  const message=decodeModuleMessage(line);count++;
  if(message.type==='command')commands.set(message.payload.command_id,message);
  if(message.type==='ack') {
    const command=commands.get(message.payload.command_id);assert(command);
    assert.equal(checkModuleAckForCommand(command,message),null);
  }
}
const meta={version:2,device_id:'node-example',boot_id:'boot-example',revision:1,uptime_ms:1000,max_message_bytes:2048};
let examples=0;
for(const [,body] of readFileSync('docs/modules/escudo-discovery.md','utf8').matchAll(/```json\n([\s\S]*?)\n```/g)) {
  makeModuleCommand(meta,JSON.parse(body),`doc-example-${++examples}`);
}
assert(examples>=5);
console.log(`Documentation requests -> v2 contract: PASS (${examples})`);
console.log(`Escudo C++ -> shared v2 codec and ACK correlation: PASS (${count} messages; host)`);
'''
subprocess.run(['node','--input-type=module','-e',node],input=wire,cwd=repo,check=True)
