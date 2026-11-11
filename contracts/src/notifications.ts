import { Ajv2020 } from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';
import registrationRequest from '../schemas/v2/notification-registration-request.schema.json' with {type:'json'};
import registrationResponse from '../schemas/v2/notification-registration-response.schema.json' with {type:'json'};
import unregistrationRequest from '../schemas/v2/notification-unregistration-request.schema.json' with {type:'json'};
import unregistrationResponse from '../schemas/v2/notification-unregistration-response.schema.json' with {type:'json'};
import statusRequest from '../schemas/v2/notification-status-request.schema.json' with {type:'json'};
import statusResponse from '../schemas/v2/notification-status-response.schema.json' with {type:'json'};
import push from '../schemas/v2/notification-push.schema.json' with {type:'json'};

export const MAX_NOTIFICATION_BODY_BYTES = 4096;
export const MAX_NOTIFICATION_TOKEN_BYTES = 2048;
export const MAX_NOTIFICATION_REGISTRATIONS = 32;
export const NOTIFICATION_REGISTRATION_TTL_DAYS = 30;

export type NotificationInput = Uint8Array | string;
export type NotificationSeverity = 'info' | 'warning' | 'critical';
export type NotificationContractErrorCode = 'invalid_notification' | 'notification_too_large' | 'token_too_large' | 'unauthorized_device' | 'event_identity_mismatch';
export class NotificationContractError extends Error {
  constructor(readonly code: NotificationContractErrorCode) {
    super(code);
    this.name = 'NotificationContractError';
  }
}

export interface NotificationRegistrationRequest {
  device_id: string;
  installation_id: string;
  platform: 'android';
  token: string;
}
export interface NotificationRegistrationResponse {
  device_id: string;
  installation_id: string;
  status: 'registered';
}
/** Resolved DELETE path installation plus its node, never an extra DELETE body. */
export interface NotificationUnregistrationRequest {
  device_id: string;
  installation_id: string;
}
export interface NotificationUnregistrationResponse {
  device_id: string;
  installation_id: string;
  status: 'removed';
}
/** The normalized GET query. Duplicate/unknown query keys must be rejected by the adapter. */
export interface NotificationStatusRequest { device_id: string }
export type NotificationDeliveryError = 'provider_unavailable' | 'provider_rejected' | 'invalid_registration' | 'rate_limited' | 'expired' | 'internal_error';
export type NotificationDelivery = {
  event_id: string;
  timestamp: string;
  read_status: 'unknown';
} & ({status: 'provider_accepted'; error: null} | {status: 'failed'; error: NotificationDeliveryError});
export interface NotificationChannelStatus {
  configured: boolean;
  last_delivery: NotificationDelivery | null;
  pending: number;
  suppressed: number;
}
export interface NotificationStatusResponse {
  device_id: string;
  channels: {webhook: NotificationChannelStatus; push: NotificationChannelStatus};
}
/** A projection of an already validated original Cebo event; no new ID is generated. */
export interface NotificationEventReference {
  device_id: string;
  message_id: string;
  payload: {name: 'cebo.interaction'; profile: 'cebo'; severity: NotificationSeverity};
}
export interface PushNotification {
  notification: {
    title: 'Sentinel Node';
    body: 'Actividad detectada en Cebo. Abre la app para consultar el incidente.';
  };
  data: {device_id: string; event_id: string; severity: NotificationSeverity; route: 'cebo'};
}

const utf8 = new TextEncoder();
const ajv = new Ajv2020({strict: true, allErrors: false});
(addFormats as unknown as (a: Ajv2020, options: {mode: 'full'}) => void)(ajv, {mode: 'full'});
const validators = {
  registrationRequest: ajv.compile<NotificationRegistrationRequest>(registrationRequest),
  registrationResponse: ajv.compile<NotificationRegistrationResponse>(registrationResponse),
  unregistrationRequest: ajv.compile<NotificationUnregistrationRequest>(unregistrationRequest),
  unregistrationResponse: ajv.compile<NotificationUnregistrationResponse>(unregistrationResponse),
  statusRequest: ajv.compile<NotificationStatusRequest>(statusRequest),
  statusResponse: ajv.compile<NotificationStatusResponse>(statusResponse),
  push: ajv.compile<PushNotification>(push),
};
const idPattern = /^[A-Za-z0-9._:-]{1,64}(?![\s\S])/;
const loneSurrogate = /[\uD800-\uDFFF]/u;
function fail(code: NotificationContractErrorCode = 'invalid_notification'): never {
  // Never attach AJV errors, input values, parse exceptions or provider messages.
  throw new NotificationContractError(code);
}
function assertDevice(deviceId: unknown, authorizedDeviceId: string): void {
  if (typeof authorizedDeviceId !== 'string' || !idPattern.test(authorizedDeviceId)) fail();
  if (deviceId !== authorizedDeviceId) fail('unauthorized_device');
}
function assertUnicode(value: unknown): void {
  if (typeof value === 'string' && loneSurrogate.test(value)) fail();
  if (Array.isArray(value)) value.forEach(assertUnicode);
  else if (value && typeof value === 'object') {
    for (const [key, child] of Object.entries(value)) { assertUnicode(key); assertUnicode(child); }
  }
}
function parse(input: NotificationInput): unknown {
  if (typeof input !== 'string' && !(input instanceof Uint8Array)) fail();
  if (typeof input === 'string' && loneSurrogate.test(input)) fail();
  const bytes = typeof input === 'string' ? utf8.encode(input) : input;
  if (bytes.byteLength > MAX_NOTIFICATION_BODY_BYTES) fail('notification_too_large');
  let data: unknown;
  try { data = JSON.parse(new TextDecoder('utf-8', {fatal: true, ignoreBOM: true}).decode(bytes)); }
  catch { return fail(); }
  // Escaped lone surrogates are legal JSON syntax but cannot identify a UTF-8 token.
  try { assertUnicode(data); } catch { return fail(); }
  return data;
}
function decode<T extends {device_id: string}>(input: NotificationInput, authorizedDeviceId: string, validate: (value: unknown) => value is T): T {
  const data = parse(input);
  if (!validate(data)) fail();
  assertDevice(data.device_id, authorizedDeviceId);
  return data;
}

/** Validates contract context; the caller must authenticate the bearer separately. */
export function decodeNotificationRegistrationRequest(input: NotificationInput, authorizedDeviceId: string): NotificationRegistrationRequest {
  const data = decode(input, authorizedDeviceId, validators.registrationRequest);
  if (utf8.encode(data.token).byteLength > MAX_NOTIFICATION_TOKEN_BYTES) fail('token_too_large');
  return data;
}
export function decodeNotificationRegistrationResponse(input: NotificationInput, authorizedDeviceId: string): NotificationRegistrationResponse {
  return decode(input, authorizedDeviceId, validators.registrationResponse);
}
export function decodeNotificationUnregistrationRequest(input: NotificationInput, authorizedDeviceId: string): NotificationUnregistrationRequest {
  return decode(input, authorizedDeviceId, validators.unregistrationRequest);
}
export function decodeNotificationUnregistrationResponse(input: NotificationInput, authorizedDeviceId: string): NotificationUnregistrationResponse {
  return decode(input, authorizedDeviceId, validators.unregistrationResponse);
}
export function decodeNotificationStatusRequest(input: NotificationInput, authorizedDeviceId: string): NotificationStatusRequest {
  return decode(input, authorizedDeviceId, validators.statusRequest);
}
export function decodeNotificationStatusResponse(input: NotificationInput, authorizedDeviceId: string): NotificationStatusResponse {
  return decode(input, authorizedDeviceId, validators.statusResponse);
}
function checkEvent(event: NotificationEventReference, authorizedDeviceId: string): void {
  if (!event || typeof event !== 'object' || typeof event.message_id !== 'string' || !idPattern.test(event.message_id)
    || event.payload?.name !== 'cebo.interaction' || event.payload.profile !== 'cebo'
    || !['info', 'warning', 'critical'].includes(event.payload.severity)) fail();
  assertDevice(event.device_id, authorizedDeviceId);
}
export function decodePushNotification(input: NotificationInput, authorizedDeviceId: string, originalEvent: NotificationEventReference): PushNotification {
  const data = parse(input);
  if (!validators.push(data)) fail();
  assertDevice(data.data.device_id, authorizedDeviceId);
  checkEvent(originalEvent, authorizedDeviceId);
  if (data.data.event_id !== originalEvent.message_id || data.data.severity !== originalEvent.payload.severity) fail('event_identity_mismatch');
  return data;
}
export function makePushNotification(originalEvent: NotificationEventReference, authorizedDeviceId: string): PushNotification {
  checkEvent(originalEvent, authorizedDeviceId);
  return {
    notification: {title: 'Sentinel Node', body: 'Actividad detectada en Cebo. Abre la app para consultar el incidente.'},
    data: {device_id: originalEvent.device_id, event_id: originalEvent.message_id, severity: originalEvent.payload.severity, route: 'cebo'},
  };
}
