#!/usr/bin/env python3
"""Compile the existing v2 JSON schemas to cJSON predicates (no runtime schema heap).
No generated source is versioned. Unsupported schema keywords fail the build.
"""
import json, pathlib, sys
root=pathlib.Path(__file__).resolve().parents[2]/'contracts/schemas/v2'
defs=json.loads((root/'common.schema.json').read_text())['$defs']
nodes={}; bodies=[]
def q(x): return json.dumps(x, ensure_ascii=True)
def emit(s):
    key=json.dumps(s,sort_keys=True)
    if key in nodes:return nodes[key]
    name='v'+str(len(nodes));nodes[key]=name
    checks=[]
    known={'$ref','type','const','enum','minimum','maximum','minLength','maxLength','pattern','format','properties','required','additionalProperties','items','minItems','maxItems','uniqueItems','anyOf','oneOf','allOf','if','then','else','$schema','$id','$comment'}
    assert not set(s)-known,set(s)-known
    if '$ref'in s: checks.append(emit(defs[s['$ref'].split('/')[-1]])+'(j)')
    typ=s.get('type')
    if typ:
        checks.append({'object':'cJSON_IsObject(j)','array':'cJSON_IsArray(j)','string':'cJSON_IsString(j)','null':'cJSON_IsNull(j)','boolean':'cJSON_IsBool(j)','integer':'(cJSON_IsNumber(j) && std::isfinite(j->valuedouble) && std::floor(j->valuedouble)==j->valuedouble)','number':'(cJSON_IsNumber(j) && std::isfinite(j->valuedouble))'}[typ])
    for op in ('const','enum'):
        if op in s:
            vs=s[op] if op=='enum' else [s[op]]
            checks.append('('+' || '.join('('+('cJSON_IsString(j) && !strcmp(j->valuestring,'+q(v)+')' if isinstance(v,str) else 'cJSON_IsNull(j)' if v is None else 'cJSON_IsTrue(j)' if v is True else 'cJSON_IsFalse(j)' if v is False else 'cJSON_IsNumber(j) && j->valuedouble=='+str(v)) +')' for v in vs)+')')
    for k,cmp in [('minimum','>='),('maximum','<=')]:
        if k in s:checks.append(f'(!cJSON_IsNumber(j) || j->valuedouble {cmp} {s[k]}.0)')
    for k,cmp in [('minLength','>='),('maxLength','<=')]:
        if k in s and not (k=='minLength' and s[k]==0):checks.append(f'(!cJSON_IsString(j) || characters(j->valuestring) {cmp} {s[k]})')
    if 'pattern'in s:
        p=s['pattern']
        tag= ('utc' if p.startswith('^(?!0000-') else 'id' if p.startswith('^[A-Za-z0-9._:-]') else 'cursor' if p.startswith('^[A-Za-z0-9._:~-]') else 'mac' if p.startswith('^(?:[0-9a-f]{2}:)') else 'hex' if p.startswith('^(?:[0-9a-f]{2})') else 'domain' if p.startswith('^(?:[a-z0-9]') else 'mqtt' if p.startswith('^mqtts://') else 'https' if p.startswith('^https://') else 'http' if p.startswith('^https?://') else 'text' if '\x00' in p else None)
        assert tag is not None,p
        checks.append('(!cJSON_IsString(j) || pattern(j->valuestring,'+q(tag)+'))')
    if 'format'in s:checks.append('(!cJSON_IsString(j) || format(j->valuestring,'+q(s['format'])+'))')
    if 'properties'in s or 'required'in s:
        p=s.get('properties',{})
        if s.get('additionalProperties') is False:checks.append('object_keys(j,'+q(','.join(p))+')')
        for k in s.get('required',[]):checks.append('cJSON_GetObjectItemCaseSensitive(j,'+q(k)+')')
        for k,v in p.items():
            sub=emit(v);a='cJSON_GetObjectItemCaseSensitive(j,'+q(k)+')'
            checks.append('(!'+a+' || '+sub+'('+a+'))')
    elif s.get('additionalProperties') is False: checks.append('fields(j,"")')
    if 'items'in s:checks.append('array_items(j,'+emit(s['items'])+')')
    for k,cmp in [('minItems','>='),('maxItems','<=')]:
        if k in s:checks.append(f'(!cJSON_IsArray(j) || cJSON_GetArraySize(j) {cmp} {s[k]})')
    if s.get('uniqueItems'):checks.append('unique(j)')
    for k,join in [('anyOf',' || '),('allOf',' && ')]:
        if k in s:checks.append('('+join.join(emit(x)+'(j)' for x in s[k])+')')
    if 'oneOf'in s:checks.append('('+' + '.join('int('+emit(x)+'(j))' for x in s['oneOf'])+' == 1)')
    if 'if'in s:checks.append('('+emit(s['if'])+'(j) ? '+emit(s.get('then',{}))+'(j) : '+emit(s.get('else',{}))+'(j))')
    bodies.append('static bool '+name+'(const cJSON *j) { (void)j; return '+' && '.join('('+c+')' for c in checks or ['true'])+'; }')
    return name
exports={k:emit(v) for k,v in defs.items()}
for k in ['command','state','metrics','event','ack']:
    exports[k]=emit(json.loads((root/(k+'.schema.json')).read_text()))
out=['// Generated from contracts/schemas/v2; do not edit.']
out+=['static bool '+n+'(const cJSON*);' for n in nodes.values()]
out+=bodies
out+=['static bool schema(const char *type,const cJSON *j) {']
out+=['if (!strcmp(type,'+q(k)+')) return '+v+'(j);' for k,v in exports.items()]
out+=['return false; }']
pathlib.Path(sys.argv[1]).write_text('\n'.join(out)+'\n')
