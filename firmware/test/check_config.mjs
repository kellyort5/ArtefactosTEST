#!/usr/bin/env node
// Inspect effective headers and partition bounds; never infer physical memory.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {join, dirname} from 'node:path';
import {capture, sketch, firmware} from './host_support.mjs';
const index = process.argv.indexOf('--sdkconfig');
if (index < 0 || !process.argv[index + 1]) throw new Error('Use --sdkconfig <effective sdkconfig.h>');
const config = readFileSync(process.argv[index + 1],'utf8').replaceAll('\r\n','\n');
const required = {CONFIG_IDF_TARGET_ESP32S3:'1',CONFIG_ESPTOOLPY_FLASHSIZE_16MB:'1',
  CONFIG_SPIRAM:'1',CONFIG_SPIRAM_MODE_OCT:'1',CONFIG_SPIRAM_SPEED_80M:'1',
  CONFIG_BT_ENABLED:'1',CONFIG_BT_NIMBLE_ENABLED:'1',CONFIG_BT_NIMBLE_SM_SC:'1',
  CONFIG_BT_NIMBLE_MAX_BONDS:'3',CONFIG_BT_NIMBLE_NVS_PERSIST:'1',
  CONFIG_MBEDTLS_SSL_PROTO_TLS1_2:'1',
  CONFIG_LWIP_IP_FORWARD:'1',CONFIG_LWIP_IPV4_NAPT:'1'};
for (const [key,value] of Object.entries(required)) assert(config.includes(`#define ${key} ${value}\n`),key);
for (const key of ['CONFIG_ESP_PHY_INIT_DATA_IN_PARTITION','CONFIG_BT_NIMBLE_SM_SC_DEBUG_KEYS',
  'CONFIG_ESP_TLS_INSECURE','CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY'])
  assert(!config.includes('#define '+key+' '),key);
const application = capture('c++',['-dM','-E','-x','c++','-include',join(sketch,'SentinelConfig.h'),'-'],{input:''});
for (const [key,value] of Object.entries({SENTINEL_SDA:'8',SENTINEL_SCL:'9',SENTINEL_BUTTON:'4',
  SENTINEL_OLED_ADDRESS:'0x3c',SENTINEL_MODULE_ESCUDO:'1',SENTINEL_MODULE_SANITIZADOR:'0',SENTINEL_MODULE_CEBO:'0'}))
  assert(new RegExp('^#define '+key+' '+value+'$','m').test(application),key);
const partitions = [];
let last = 0x9000;
for (const line of readFileSync(join(sketch,'partitions.csv'),'utf8').split(/\r?\n/).filter(s => s.trim() && !s.startsWith('#'))) {
  const [name,kind,sub,offset,size,flags = ''] = line.split(',').map(s => s.trim());
  const begin = Number(offset), length = Number(size);
  assert(Number.isInteger(begin) && Number.isInteger(length) && length > 0);
  assert(begin >= last && begin+length <= 16*1024*1024,name);
  partitions.push([name,kind,sub,begin,length,flags]);
  last = begin+length;
}
assert.deepEqual(partitions,[
  ['otadata','data','ota',0xe000,0x2000,''],
  ['factory','app','factory',0x10000,0x400000,''],
  ['nvs','data','nvs',0x410000,0x6000,''],
  ['event_log','data','nvs',0x416000,0x40000,''],
],'Approved standard USB layout and unchanged storage capacities');
const prefix = 'esp32s3.menu.PartitionScheme.sentinel';
const settings = readFileSync(join(firmware,'arduino/boards.local.txt'),'utf8').split(/\r?\n/)
  .filter(line => line.trim() && !line.trimStart().startsWith('#'));
assert.deepEqual(settings,[
  prefix+'=Sentinel standard USB (4MiB factory, no OTA)',
  prefix+'.build.partitions=sentinel',
  prefix+'.upload.maximum_size=4194304',
  prefix+'.compiler.c.elf.extra_flags=-Wl,--wrap=nvs_flash_init',
],'Exactly four approved keys; upload/export use official recipes');
console.log('Official SDK config, four local keys and standard USB 16 MiB partition layout: PASS (not physical memory detection)');
console.log('SDK legacy/SC0 defaults do not authorize legacy links; runtime BLE and HTTPS enforcement is separate.');

// Optional verification with the already installed official core tool; never download tools.
const toolIndex = process.argv.indexOf('--partition-tool');
if (toolIndex >= 0) {
  assert(process.argv[toolIndex+1],'Use --partition-tool <official core tools/gen_esp32part.py>');
  const tool = process.argv[toolIndex+1];
  const pythonIndex = process.argv.indexOf('--python');
  const python = pythonIndex >= 0 ? process.argv[pythonIndex+1] : 'python3';
  const output = capture(python,['-c', String.raw`
import importlib.util, pathlib, sys
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('gen_esp32part', sys.argv[1])
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
table = module.PartitionTable.from_csv(pathlib.Path(sys.argv[2]).read_text())
table.verify()
table.verify_size_fits(16 * 1024 * 1024)
binary = table.to_binary()
parsed = module.PartitionTable.from_binary(binary)
parsed.verify()
parsed.verify_size_fits(16 * 1024 * 1024)
assert parsed.to_binary() == binary
assert module.PartitionTable.from_csv(parsed.to_csv()).to_binary() == binary
boot = pathlib.Path(sys.argv[1]).parent / 'partitions' / 'boot_app0.bin'
ota = parsed.find_by_name('otadata')
assert ota.offset == 0xe000 and boot.stat().st_size == ota.size == 0x2000
for partition in parsed:
    if partition.name != 'otadata':
        assert partition.offset + partition.size <= ota.offset or partition.offset >= ota.offset + boot.stat().st_size
print('Official gen_esp32part: CSV/binary parse, verify, verify_size_fits(16MiB), roundtrip and boot_app0 containment: PASS')
`,tool,join(sketch,'partitions.csv')]);
  const platform = readFileSync(join(dirname(dirname(tool)),'platform.txt'),'utf8');
  for (const key of ['tools.esptool_py.upload.pattern_args',
    'recipe.hooks.objcopy.postobjcopy.3.pattern_args',
    'recipe.hooks.objcopy.postobjcopy.4.pattern.windows']) {
    const line = platform.split(/\r?\n/).find(line => line.startsWith(key+'='));
    assert(line && line.includes('0xe000') && line.includes('boot_app0.bin'),key);
  }
  console.log(output.trim());
  console.log('Official upload, merged image and Windows flash_args reserve boot_app0 at 0xe000: PASS (source recipe inspection)');
}
