#!/usr/bin/env python3
"""Check effective generated security/memory configuration and partition bounds."""
from pathlib import Path
import csv
root=Path(__file__).resolve().parents[1]
config=(root/'.pio/build/sentinel_s3/config/sdkconfig.h').read_text()
required={'CONFIG_ESPTOOLPY_FLASHSIZE_16MB':'1','CONFIG_SPIRAM_MODE_OCT':'1',
          'CONFIG_SPIRAM_SPEED_80M':'1','CONFIG_BT_NIMBLE_SM_SC_ONLY':'1',
          'CONFIG_BT_NIMBLE_SM_LVL':'4','CONFIG_BT_NIMBLE_MAX_BONDS':'4',
          'CONFIG_BT_NIMBLE_NVS_PERSIST':'1','CONFIG_MQTT_PROTOCOL_5':'1',
          'CONFIG_MBEDTLS_HAVE_TIME_DATE':'1','CONFIG_SENTINEL_SDA':'8',
          'CONFIG_SENTINEL_SCL':'9','CONFIG_SENTINEL_BUTTON':'4'}
for key,value in required.items():
    assert f'#define {key} {value}\n' in config,key
for key in ('CONFIG_BT_NIMBLE_SM_LEGACY','CONFIG_BT_NIMBLE_SM_SC_DEBUG_KEYS',
            'CONFIG_ESP_TLS_INSECURE','CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY'):
    assert f'#define {key} ' not in config,key
last=0
for row in csv.reader(line for line in (root/'partitions.csv').read_text().splitlines() if line and not line.startswith('#')):
    name,kind,sub,offset,size,*_=map(str.strip,row)
    begin,length=int(offset,0),int(size,0)
    assert begin>=last and begin+length<=16*1024*1024
    if name=='event_log':assert kind=='data' and sub=='nvs' and length==256*1024
    last=begin+length
print('Effective config and 16 MiB partition bounds: PASS (not physical memory detection)')
