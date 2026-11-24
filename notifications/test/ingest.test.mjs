import assert from 'node:assert/strict';
import {test} from 'node:test';
import {eventTopics, ingest} from '../src/ingest.js';
import {NODE_A, NODE_B, bytes, ceboEvent, fixture, topicFor} from './helpers.mjs';

const allowed = new Set([NODE_A]);

test('los topics de suscripción cubren v1 y v2 de cada nodo, solo eventos', () => {
  assert.deepEqual(eventTopics([NODE_A, NODE_B]), [
    'sentinel/v1/nodes/node-example/events', 'sentinel/v2/nodes/node-example/events',
    'sentinel/v1/nodes/node-other/events', 'sentinel/v2/nodes/node-other/events']);
  assert.throws(() => eventTopics(['no válido']));
});

test('un evento Cebo v2 válido se reduce a la identidad y los campos permitidos', () => {
  const event = ceboEvent({id: 'boot-1:7', severity: 'critical', protocol: 'telnet'});
  const result = ingest(topicFor(), bytes(event), allowed);
  assert.equal(result.kind, 'event');
  assert.equal(result.event.identity, 'node-example/boot-1:7');
  assert.equal(result.event.severity, 'critical');
  assert.equal(result.event.details.protocol, 'telnet');
  assert.equal(result.event.details.destination_port, 23);
  assert.deepEqual(Object.keys(result.event).sort(), ['boot_id', 'details', 'device_id', 'identity', 'message_id', 'name',
    'origin', 'severity', 'time_quality', 'timestamp', 'uptime_ms']);
});

test('los fixtures oficiales de interacciones Cebo se aceptan tal cual', () => {
  for (const name of ['cebo.http-benign', 'cebo.http-repeated-auth', 'cebo.telnet-repeated-auth']) {
    const result = ingest(topicFor(), bytes(fixture(`events/${name}.valid.json`)), allowed);
    assert.equal(result.kind, 'event', name);
  }
});

test('un evento v2 que no es de Cebo se valida y se ignora', () => {
  assert.equal(ingest(topicFor(), bytes(fixture('events/system.booted.valid.json')), allowed).kind, 'ignored');
  assert.equal(ingest(topicFor(), bytes(fixture('events/escudo.coverage.valid.json')), allowed).kind, 'ignored');
});

test('un evento v1 válido se acepta en su topic y se descarta: v1 no define interacciones Cebo', () => {
  const v1 = fixture('../event.valid.json');
  assert.equal(v1.version, 1);
  const result = ingest(topicFor(v1.device_id, 1), bytes(v1), new Set([v1.device_id]));
  assert.equal(result.kind, 'ignored');
});

test('lo que no cumple el contrato es inválido y no se interpreta', () => {
  const event = ceboEvent();
  const bad = [
    bytes('{no es json'),
    bytes(''),
    bytes({...event, extra: 'campo'}),                                              // objeto cerrado
    bytes({...event, payload: {...event.payload, details: {...event.payload.details, password: 'FICTIONAL'}}}),
    bytes({...event, payload: {...event.payload, severity: 'critical'}}),            // sospecha/severidad no coinciden
    bytes({...event, version: 3}),
    Buffer.from([0xff, 0xfe, 0x00]),                                                 // UTF-8 inválido
    Buffer.alloc(3000, 0x20),                                                        // por encima de 2048 bytes
  ];
  for (const payload of bad) {
    const result = ingest(topicFor(), payload, allowed);
    assert.equal(result.kind, 'invalid', payload.toString('utf8').slice(0, 40));
    assert.equal(typeof result.code, 'string');
    assert.ok(!JSON.stringify(result).includes('FICTIONAL'), 'el resultado no repite el contenido');
  }
});

test('el topic enruta pero no autentica: debe coincidir con el nodo y la versión del mensaje', () => {
  const event = ceboEvent();
  assert.equal(ingest(topicFor(NODE_B), bytes(event), allowed).kind, 'foreign');             // nodo no autorizado
  assert.equal(ingest('sentinel/v2/nodes/node-example/commands', bytes(event), allowed).kind, 'foreign');
  assert.equal(ingest('sentinel/v2/nodes/node-example/events/extra', bytes(event), allowed).kind, 'foreign');
  assert.equal(ingest('otro/topic', bytes(event), allowed).kind, 'foreign');
  // Un nodo autorizado no puede publicar eventos con la identidad de otro.
  const spoofed = ceboEvent({device: NODE_B});
  assert.deepEqual(ingest(topicFor(NODE_A), bytes(spoofed), allowed), {kind: 'invalid', code: 'topic_mismatch'});
  // La versión del topic debe ser la del mensaje.
  assert.deepEqual(ingest(topicFor(NODE_A, 1), bytes(event), allowed), {kind: 'invalid', code: 'topic_mismatch'});
});
