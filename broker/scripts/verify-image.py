#!/usr/bin/env python3
"""Verify the pinned tag's public OCI manifest without a Docker daemon."""
import hashlib
import json
import urllib.request
from common import IMAGE

name, expected = IMAGE.split('@')
tag = name.split(':')[1]
token_url = 'https://auth.docker.io/token?service=registry.docker.io&scope=repository:library/eclipse-mosquitto:pull'
with urllib.request.urlopen(token_url, timeout=20) as response:
    token = json.load(response)['token']
request = urllib.request.Request(
    'https://registry-1.docker.io/v2/library/eclipse-mosquitto/manifests/' + tag,
    headers={'Authorization': 'Bearer ' + token,
             'Accept': 'application/vnd.oci.image.index.v1+json, application/vnd.docker.distribution.manifest.list.v2+json'})
with urllib.request.urlopen(request, timeout=20) as response:
    manifest = response.read()
    actual = response.headers['Docker-Content-Digest']
assert actual == expected == 'sha256:' + hashlib.sha256(manifest).hexdigest(), 'Tag/digest mismatch; review image before changing the pin.'
print('PASS official registry manifest: ' + IMAGE)
