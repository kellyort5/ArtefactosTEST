import { Ajv2020 } from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';
import common from '../schemas/v2/common.schema.json' with { type: 'json' };
import state from '../schemas/v2/state.schema.json' with { type: 'json' };
import metrics from '../schemas/v2/metrics.schema.json' with { type: 'json' };
import event from '../schemas/v2/event.schema.json' with { type: 'json' };
import command from '../schemas/v2/command.schema.json' with { type: 'json' };
import ack from '../schemas/v2/ack.schema.json' with { type: 'json' };
import metadata from '../schemas/v2/metadata.schema.json' with { type: 'json' };
import type { V2Message, V2Metadata, V2CommandRequest, V2CommandMessage, V2AckMessage, V2AckResult, V2Ssid } from './v2-types.js';
import { ContractError, parseJsonBounded, stringifyContract, record } from './codec-primitives.js';

export const MODULE_PROTOCOL_VERSION = 2 as const;
export const MAX_MODULE_MESSAGE_BYTES = 2048;
export const MAX_MODULE_METADATA_BYTES = 256;

const ajv = new Ajv2020({ allErrors: false, strict: true });
(addFormats as unknown as (a: Ajv2020, options: { mode: 'full' }) => void)(ajv, { mode: 'full' });
for (const schema of [common, state, metrics, event, command, ack, metadata]) ajv.addSchema(schema);
const validators = Object.fromEntries(
  Object.entries({ state, metrics, event, command, ack }).map(([name, schema]) => [name, ajv.getSchema(schema.$id)!]),
);
const validateMetadata = ajv.getSchema(metadata.$id)!;
const utf8 = new TextEncoder();

function invalid(): never {
  throw new ContractError('invalid_message');
}

function parseModuleJson(input: Uint8Array | string, limit: number): unknown {
  // TextEncoder replaces lone UTF-16 surrogates. Reject them before that lossy step.
  if (typeof input === 'string' && /[\uD800-\uDFFF]/u.test(input)) invalid();
  return parseJsonBounded(input, limit);
}

function urlAllowed(text: string, protocol: 'https:' | 'http:' | 'mqtt:', endpoint = false): void {
  try {
    const url = new URL(text);
    if (url.protocol !== protocol || !url.hostname || url.username || url.password || url.hash) invalid();
    if (url.port && (Number(url.port) < 1 || Number(url.port) > 65535)) invalid();
    if (endpoint && url.search) invalid();
    if (protocol === 'mqtt:' && (url.search || (url.pathname !== '' && url.pathname !== '/'))) invalid();
  } catch {
    invalid();
  }
}

// Schemas count characters; SSIDs and Wi-Fi keys have byte limits on ESP-IDF.
function byteLimits(value: unknown, depth = 0): void {
  if (depth > 32) invalid();
  if (typeof value === 'string') {
    // Escaped lone surrogates are valid JSON, but cannot represent UTF-8 data.
    if (/[\uD800-\uDFFF]/u.test(value)) invalid();
  } else if (Array.isArray(value)) {
    for (const item of value) byteLimits(item, depth + 1);
  } else if (record(value)) {
    for (const [key, child] of Object.entries(value)) {
      if (/[\uD800-\uDFFF]/u.test(key)) invalid();
      if (typeof child === 'string') {
        if (key === 'ssid' && utf8.encode(child).length > 32) invalid();
        if (key === 'wifi_password' && utf8.encode(child).length > 63) invalid();
      }
      byteLimits(child, depth + 1);
    }
  }
}

export function decodeModuleMessage(input: Uint8Array | string): V2Message {
  const data = parseModuleJson(input, MAX_MODULE_MESSAGE_BYTES);
  if (!record(data)) invalid();
  if (data.version !== MODULE_PROTOCOL_VERSION) throw new ContractError('unsupported_version');
  const validate = typeof data.type === 'string' && Object.hasOwn(validators, data.type)
    ? validators[data.type]
    : undefined;
  if (!validate || !validate(data)) invalid();
  byteLimits(data.payload);
  const message = data as unknown as V2Message;
  if (message.type === 'command') {
    const { operation, args } = message.payload;
    if (operation === 'configure_connectivity') urlAllowed(args.mqtt_uri, 'mqtt:');
    if (operation === 'sanitizador.configure_doh') urlAllowed(args.endpoint, 'https:', true);
    if (operation === 'sanitizador.audit') {
      urlAllowed(args.https_url, 'https:');
      const scheme = args.portal_url.startsWith('https:') ? 'https:' : 'http:';
      urlAllowed(args.portal_url, scheme);
    }
  }
  if (message.type === 'ack' && message.payload.result?.kind === 'job'
    && message.payload.result.job_id !== message.payload.command_id) invalid();
  return message;
}

export function encodeModuleMessage(message: V2Message): Uint8Array {
  const text = stringifyContract(message);
  decodeModuleMessage(text);
  return utf8.encode(text);
}

export function decodeModuleMetadata(input: Uint8Array | string): V2Metadata {
  const data = parseModuleJson(input, MAX_MODULE_METADATA_BYTES);
  if (!validateMetadata(data)) invalid();
  return data as V2Metadata;
}

export function makeModuleCommand(
  meta: V2Metadata,
  request: V2CommandRequest,
  commandId: string,
  elapsedMs = 0,
): V2CommandMessage {
  decodeModuleMetadata(stringifyContract(meta));
  if (!Number.isSafeInteger(elapsedMs) || elapsedMs < 0 || elapsedMs > 15000) invalid();
  const uptime = meta.uptime_ms + elapsedMs;
  const result: V2CommandMessage = {
    version: 2, type: 'command', device_id: meta.device_id, boot_id: meta.boot_id,
    message_id: commandId, seq: 0, uptime_ms: uptime, timestamp: null, time_quality: 'unknown',
    payload: {
      command_id: commandId, target_boot_id: meta.boot_id, expected_revision: meta.revision,
      expires_at_uptime_ms: uptime + 30000, ...request,
    },
  };
  encodeModuleMessage(result);
  return result;
}

export type ModuleCommandRejection =
  | 'unauthorized' | 'invalid_message' | 'wrong_boot' | 'expired' | 'revision_conflict' | 'invalid_state';

export type ModuleCommandContext = {
  channel: 'ble' | 'mqtt';
  authenticated: boolean;
  authorized: boolean;
  encrypted: boolean;
  deviceId: string;
  bootId: string;
  revision: number;
  uptimeMs: number;
  profile: 'idle' | 'escudo' | 'sanitizador' | 'cebo';
  retained?: boolean;
  targetSelected?: boolean;
};

/** Pure admission check, before the consumer's duplicate cache and execution. */
export function checkModuleCommandAdmission(
  message: V2CommandMessage,
  context: ModuleCommandContext,
): ModuleCommandRejection | null {
  return admission(validatedCommand(message), context);
}

function validatedCommand(message: V2CommandMessage): V2CommandMessage {
  const command = decodeModuleMessage(encodeModuleMessage(message));
  if (command.type !== 'command') invalid();
  return command;
}

function admission(message: V2CommandMessage, context: ModuleCommandContext): ModuleCommandRejection | null {
  if (message.device_id !== context.deviceId || (context.channel === 'ble' && (!context.authenticated || !context.authorized || !context.encrypted))) return 'unauthorized';
  if (context.channel !== 'ble' && context.channel !== 'mqtt') return 'unauthorized';
  if (context.retained) return 'invalid_message';
  if (message.boot_id !== context.bootId || message.payload.target_boot_id !== context.bootId) return 'wrong_boot';
  if (!Number.isSafeInteger(context.uptimeMs) || context.uptimeMs < 0) return 'invalid_message';
  const ttl = message.payload.expires_at_uptime_ms - context.uptimeMs;
  if (ttl <= 0 || ttl > 30000) return 'expired';
  const operation = message.payload.operation;
  if (context.channel === 'mqtt' && context.profile !== 'cebo') return 'invalid_state';
  const bleOnly = operation === 'configure_connectivity' || operation === 'set_time'
    || operation.startsWith('escudo.') || operation.startsWith('sanitizador.');
  if (bleOnly && context.channel !== 'ble') return 'unauthorized';
  return null;
}

/** After admission and cache lookup, call only for a NEW command, then execute. */
export function checkNewModuleCommandContext(
  message: V2CommandMessage,
  context: ModuleCommandContext,
): ModuleCommandRejection | null {
  message = validatedCommand(message);
  const rejected = admission(message, context);
  if (rejected) return rejected;
  if (!Number.isSafeInteger(context.revision) || context.revision < 0) return 'invalid_message';
  if (message.payload.expected_revision !== context.revision) return 'revision_conflict';
  for (const profile of ['escudo', 'sanitizador', 'cebo'] as const) {
    if (message.payload.operation.startsWith(profile + '.') && context.profile !== profile) return 'invalid_state';
  }
  if (message.payload.operation === 'escudo.capture_start' && !context.targetSelected) return 'invalid_state';
  return null;
}

/** Applied results accepted by the consumer for each pending operation. */
export const MODULE_COMMAND_RESULTS = {
  get_state: [null, 'state'], set_mode: [null], configure_connectivity: [null], set_time: [null],
  read_events: ['events'],
  'escudo.scan': ['job'], 'escudo.networks': ['escudo.networks'],
  'escudo.select_target': ['escudo.target'], 'escudo.capture_start': [null], 'escudo.capture_stop': [null],
  'sanitizador.configure_upstream': [null], 'sanitizador.configure_ap': [null], 'sanitizador.configure_doh': [null],
  'sanitizador.audit': ['job'], 'sanitizador.audit_result': ['sanitizador.audit'],
  'sanitizador.gateway_start': [null], 'sanitizador.gateway_stop': [null],
  'sanitizador.clients': ['sanitizador.clients'], 'sanitizador.rules.put': ['sanitizador.rule'],
  'sanitizador.rules.remove': ['sanitizador.rule_removed'], 'sanitizador.rules.list': ['sanitizador.rules'],
  'cebo.start': [null], 'cebo.stop': [null], 'cebo.configure': [null], 'cebo.stats': ['cebo.stats'],
} as const satisfies Record<V2CommandRequest['operation'], readonly (V2AckResult['kind'] | null)[]>;

function ssidBytes(ssid: V2Ssid): Uint8Array {
  if (typeof ssid === 'string') return utf8.encode(ssid);
  return Uint8Array.from(ssid.value.match(/../g) ?? [], pair => Number.parseInt(pair, 16));
}

/** Correlate a validated ACK to its pending command, without applying effects. */
export function checkModuleAckForCommand(
  command: V2CommandMessage,
  acknowledgement: V2AckMessage,
): ModuleCommandRejection | null {
  command = validatedCommand(command);
  const ack = decodeModuleMessage(encodeModuleMessage(acknowledgement));
  if (ack.type !== 'ack') invalid();
  if (ack.device_id !== command.device_id || ack.payload.command_id !== command.payload.command_id) return 'invalid_message';
  if (ack.payload.status === 'rejected') return null;
  if (ack.boot_id !== command.payload.target_boot_id) return 'wrong_boot';
  if (ack.payload.status === 'accepted') return null;
  const result = ack.payload.result;
  const allowed: readonly (V2AckResult['kind'] | null)[] = MODULE_COMMAND_RESULTS[command.payload.operation];
  if (!allowed.includes(result?.kind ?? null)) return 'invalid_message';
  if (result?.kind === 'escudo.networks' && command.payload.operation === 'escudo.networks'
    && (result.networks.length > command.payload.args.limit || result.next !== null && result.next === command.payload.args.after)) return 'invalid_message';
  if (result?.kind === 'sanitizador.clients' && command.payload.operation === 'sanitizador.clients'
    && (result.clients.length > command.payload.args.limit || result.next !== null && result.next === command.payload.args.after)) return 'invalid_message';
  if (result?.kind === 'sanitizador.rules' && command.payload.operation === 'sanitizador.rules.list'
    && (result.rules.length > command.payload.args.limit || result.next !== null && result.next === command.payload.args.after)) return 'invalid_message';
  if (result?.kind === 'events' && command.payload.operation === 'read_events') {
    const { after_cursor: after, limit } = command.payload.args;
    const { next_cursor: next, retained_from: from, retained_to: to, more, gap } = result;
    if (next < after || from > to || (from === 0) !== (to === 0)) return 'invalid_message';
    if (next > after && (from === 0 || next < from || next > to)) return 'invalid_message';
    if (more && (next <= after || next >= to)) return 'invalid_message';
    if (!gap && (next - after > limit || more !== (next < to))) return 'invalid_message';
  }
  if (result?.kind === 'sanitizador.audit' && result.audit !== null
    && command.payload.operation === 'sanitizador.audit_result'
    && result.audit.job_id !== command.payload.args.job_id) return 'invalid_message';
  if (result?.kind === 'sanitizador.rule_removed' && command.payload.operation === 'sanitizador.rules.remove'
    && result.rule_id !== command.payload.args.rule_id) return 'invalid_message';
  if (result?.kind === 'sanitizador.rule' && command.payload.operation === 'sanitizador.rules.put') {
    const requested = command.payload.args;
    if (result.rule.rule_id !== requested.rule_id || result.rule.domain !== requested.domain
      || result.rule.action !== requested.action || result.rule.include_subdomains !== requested.include_subdomains) return 'invalid_message';
  }
  if (result?.kind === 'escudo.target' && command.payload.operation === 'escudo.select_target') {
    const requested = command.payload.args;
    const actual = ssidBytes(result.target.ssid), expected = ssidBytes(requested.ssid);
    if (result.target.bssid !== requested.bssid || result.target.channel !== requested.channel
      || actual.length !== expected.length || actual.some((byte, index) => byte !== expected[index])) return 'invalid_message';
  }
  return null;
}
