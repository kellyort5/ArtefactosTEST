#!/usr/bin/env node
// Maintenance only: compile the shared v2 schemas to the distributed cJSON predicates.
// Arduino builds consume the checked-in output and never execute this script.
import {readFileSync, writeFileSync} from 'node:fs';
import {resolve, dirname} from 'node:path';
import {fileURLToPath} from 'node:url';

export const repo = resolve(dirname(fileURLToPath(import.meta.url)), '../..');
export const validatorPath = resolve(repo, 'firmware/SentinelNode/src/sentinel/v2_validator.h');
const known = new Set(['$ref','type','const','enum','minimum','maximum','minLength','maxLength',
  'pattern','format','properties','required','additionalProperties','items','minItems','maxItems',
  'uniqueItems','anyOf','oneOf','allOf','if','then','else','$schema','$id','$comment']);
const quote = value => JSON.stringify(value).replace(/[\u007f-\uffff]/g,
  c => '\\u' + c.charCodeAt(0).toString(16).padStart(4, '0'));
const canonical = value => JSON.stringify(value, (_, v) => v && typeof v === 'object' && !Array.isArray(v)
  ? Object.fromEntries(Object.entries(v).sort(([a], [b]) => a < b ? -1 : a > b ? 1 : 0)) : v);

export function generateValidator(schemas = resolve(repo, 'contracts/schemas/v2')) {
  const read = name => JSON.parse(readFileSync(resolve(schemas, name + '.schema.json'), 'utf8'));
  const defs = read('common').$defs;
  const nodes = new Map(), bodies = [];
  function emit(s) {
    const key = canonical(s);
    if (nodes.has(key)) return nodes.get(key);
    const name = 'v' + nodes.size;
    nodes.set(key, name);
    const checks = [];
    const unsupported = Object.keys(s).filter(k => !known.has(k));
    if (unsupported.length) throw new Error('Unsupported schema keywords: ' + unsupported.join(', '));
    if ('$ref' in s) {
      const definition = s.$ref.split('/').at(-1);
      if (!Object.hasOwn(defs, definition)
        || s.$ref !== 'https://sentinel.local/contracts/v2/common.schema.json#/$defs/' + definition)
        throw new Error('Unsupported schema reference: ' + s.$ref);
      checks.push(emit(defs[definition]) + '(j)');
    }
    if (s.type) {
      const predicate = {object:'cJSON_IsObject(j)', array:'cJSON_IsArray(j)', string:'cJSON_IsString(j)',
        null:'cJSON_IsNull(j)', boolean:'cJSON_IsBool(j)',
        integer:'(cJSON_IsNumber(j) && std::isfinite(j->valuedouble) && std::floor(j->valuedouble)==j->valuedouble)',
        number:'(cJSON_IsNumber(j) && std::isfinite(j->valuedouble))'}[s.type];
      if (!predicate) throw new Error('Unsupported schema type: ' + s.type);
      checks.push(predicate);
    }
    for (const op of ['const', 'enum']) if (op in s) {
      const values = op === 'enum' ? s[op] : [s[op]];
      checks.push('(' + values.map(v => '(' + (typeof v === 'string'
        ? 'cJSON_IsString(j) && !strcmp(j->valuestring,' + quote(v) + ')'
        : v === null ? 'cJSON_IsNull(j)' : v === true ? 'cJSON_IsTrue(j)'
          : v === false ? 'cJSON_IsFalse(j)' : 'cJSON_IsNumber(j) && j->valuedouble==' + v) + ')').join(' || ') + ')');
    }
    for (const [k, cmp] of [['minimum','>='], ['maximum','<=']])
      if (k in s) checks.push(`(!cJSON_IsNumber(j) || j->valuedouble ${cmp} ${s[k]}.0)`);
    for (const [k, cmp] of [['minLength','>='], ['maxLength','<=']])
      if (k in s && !(k === 'minLength' && s[k] === 0))
        checks.push(`(!cJSON_IsString(j) || characters(j->valuestring) ${cmp} ${s[k]})`);
    if ('pattern' in s) {
      const p = s.pattern;
      const tag = p.startsWith('^(?!0000-') ? 'utc' : p.startsWith('^[A-Za-z0-9._:-]') ? 'id'
        : p.startsWith('^[A-Za-z0-9._:~-]') ? 'cursor' : p.startsWith('^(?:[0-9a-f]{2}:)') ? 'mac'
          : p.startsWith('^(?:[0-9a-f]{2})') ? 'hex' : p.startsWith('^(?:[a-z0-9]') ? 'domain'
            : p.startsWith('^mqtts://') ? 'mqtt' : p.startsWith('^https://') ? 'https'
              : p.startsWith('^https?://') ? 'http' : p.includes('\x00') ? 'text' : null;
      if (!tag) throw new Error('Unsupported schema pattern: ' + p);
      checks.push('(!cJSON_IsString(j) || pattern(j->valuestring,' + quote(tag) + '))');
    }
    if ('format' in s) checks.push('(!cJSON_IsString(j) || format(j->valuestring,' + quote(s.format) + '))');
    if ('properties' in s || 'required' in s) {
      const properties = s.properties ?? {};
      if (s.additionalProperties === false) checks.push('object_keys(j,' + quote(Object.keys(properties).join(',')) + ')');
      for (const k of s.required ?? []) checks.push('cJSON_GetObjectItemCaseSensitive(j,' + quote(k) + ')');
      for (const [k, v] of Object.entries(properties)) {
        const sub = emit(v), item = 'cJSON_GetObjectItemCaseSensitive(j,' + quote(k) + ')';
        checks.push('(!' + item + ' || ' + sub + '(' + item + '))');
      }
    } else if (s.additionalProperties === false) checks.push('fields(j,"")');
    if ('items' in s) checks.push('array_items(j,' + emit(s.items) + ')');
    for (const [k, cmp] of [['minItems','>='], ['maxItems','<=']])
      if (k in s) checks.push(`(!cJSON_IsArray(j) || cJSON_GetArraySize(j) ${cmp} ${s[k]})`);
    if (s.uniqueItems) checks.push('unique(j)');
    for (const [k, join] of [['anyOf',' || '], ['allOf',' && ']])
      if (k in s) checks.push('(' + s[k].map(x => emit(x) + '(j)').join(join) + ')');
    if ('oneOf' in s) checks.push('(' + s.oneOf.map(x => 'int(' + emit(x) + '(j))').join(' + ') + ' == 1)');
    if ('if' in s) checks.push('(' + emit(s.if) + '(j) ? ' + emit(s.then ?? {}) + '(j) : ' + emit(s.else ?? {}) + '(j))');
    bodies.push('static bool ' + name + '(const cJSON *j) { (void)j; return '
      + (checks.length ? checks : ['true']).map(c => '(' + c + ')').join(' && ') + '; }');
    return name;
  }
  const exports = Object.fromEntries(Object.entries(defs).map(([k, v]) => [k, emit(v)]));
  for (const k of ['command','state','metrics','event','ack']) exports[k] = emit(read(k));
  return ['// Generated from contracts/schemas/v2; do not edit.',
    ...[...nodes.values()].map(n => 'static bool ' + n + '(const cJSON*);'), ...bodies,
    'static bool schema(const char *type,const cJSON *j) {',
    ...Object.entries(exports).map(([k,v]) => 'if (!strcmp(type,' + quote(k) + ')) return ' + v + '(j);'),
    'return false; }', ''].join('\n');
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const args = process.argv.slice(2);
  let check = false, schemas, output;
  for (let i = 0; i < args.length; i++) {
    if (args[i] === '--check') check = true;
    else if (args[i] === '--schemas') {
      schemas = args[++i];
      if (!schemas || schemas.startsWith('--')) throw new Error('--schemas requires a directory');
    } else if (args[i].startsWith('--') || output) throw new Error('Unexpected argument: ' + args[i]);
    else output = args[i];
  }
  output ??= validatorPath;
  const generated = generateValidator(schemas);
  if (check) {
    if (readFileSync(output, 'utf8') !== generated) throw new Error('Validator differs from shared schemas; regenerate and review it.');
    console.log('Distributed v2 validator matches shared schemas: PASS');
  } else writeFileSync(output, generated);
}
