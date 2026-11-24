// Adaptador de Firebase Cloud Messaging con el SDK Admin oficial. Es el único
// archivo que conoce el proveedor: el resto del componente habla con
// `provider.send({token, push})` y recibe un resultado reducido a códigos del
// contrato. La credencial (JSON de cuenta de servicio) vive fuera de Git; ni su
// contenido, ni los tokens, ni los mensajes de error del proveedor se registran.
import {readFileSync, statSync} from 'node:fs';
import {cert, initializeApp} from 'firebase-admin/app';
import {getMessaging} from 'firebase-admin/messaging';

/** Aviso si el archivo de credencial es legible por otros usuarios (solo POSIX). */
export function credentialPermissionWarning(file) {
  if (process.platform === 'win32') return null;
  try {
    return statSync(file).mode & 0o077 ? 'el archivo de credencial debería ser 0600' : null;
  } catch { return null; } // si no existe, createFcmProvider lo informa
}

export class CredentialError extends Error {
  constructor(problem) {
    super(`credencial FCM inválida: ${problem}`);
    this.name = 'CredentialError';
  }
}

/** Valida la forma de una cuenta de servicio; los mensajes nombran el campo, nunca su valor. */
export function parseCredential(text) {
  let json;
  try { json = JSON.parse(text); } catch { throw new CredentialError('no es JSON'); }
  if (!json || typeof json !== 'object' || json.type !== 'service_account') throw new CredentialError('type');
  for (const field of ['project_id', 'client_email', 'private_key'])
    if (typeof json[field] !== 'string' || !json[field]) throw new CredentialError(field);
  if (!json.private_key.includes('BEGIN PRIVATE KEY')) throw new CredentialError('private_key');
  return json;
}

/**
 * Traduce un error del SDK a un código del contrato.
 *   drop   → la alerta no se reintenta
 *   remove → el token ya no es válido: hay que borrar la instalación
 */
export function mapFcmError(error) {
  const code = String(error?.code ?? error?.errorInfo?.code ?? '');
  switch (code) {
    case 'messaging/registration-token-not-registered':
    case 'messaging/invalid-registration-token':
      return {ok: false, error: 'invalid_registration', drop: true, remove: true};
    case 'messaging/invalid-argument':
    case 'messaging/invalid-recipient':
    case 'messaging/invalid-payload':
    case 'messaging/invalid-data-payload-key':
    case 'messaging/payload-size-limit-exceeded':
    case 'messaging/invalid-options':
      // El contenido es fijo y está probado: no se borra la instalación por esto.
      return {ok: false, error: 'provider_rejected', drop: true};
    case 'messaging/message-rate-exceeded':
    case 'messaging/device-message-rate-exceeded':
    case 'messaging/topics-message-rate-exceeded':
      return {ok: false, error: 'rate_limited'};
    case 'messaging/authentication-error':
    case 'messaging/mismatched-credential':
    case 'messaging/third-party-auth-error':
    case 'messaging/invalid-apns-credentials':
    case 'messaging/sender-id-mismatch':
      // Credencial o proyecto mal configurados: se reintenta por si el operador lo corrige.
      return {ok: false, error: 'provider_rejected'};
    default:
      // Servidor no disponible, error interno, red o cualquier otro fallo.
      return {ok: false, error: 'provider_unavailable'};
  }
}

/**
 * @param credentialFile ruta de la cuenta de servicio (fuera de Git)
 * @param ttlMs          vida del mensaje en FCM: una alerta tardía deja de ser útil
 * @param messaging      sustituto del cliente de mensajería (pruebas)
 */
export function createFcmProvider({credentialFile, ttlMs = 3600 * 1000, messaging = null, appName = 'sentinel-push'}) {
  let client = messaging;
  let credentialProblem = null;
  if (!client) {
    try {
      const app = initializeApp({credential: cert(parseCredential(readFileSync(credentialFile, 'utf8')))}, appName);
      client = getMessaging(app);
    } catch (error) {
      // Ni el motivo del SDK ni su mensaje (puede citar la clave) llegan al registro.
      credentialProblem = error instanceof CredentialError ? error.message : 'credencial FCM no utilizable';
    }
  }
  return {
    configured: client !== null,
    problem: credentialProblem,
    /** @param push {notification, data} ya construido con makePushNotification del contrato */
    async send({token, push}) {
      if (!client) return {ok: false, error: 'internal_error'};
      try {
        await client.send({
          token,
          notification: push.notification,
          data: push.data,
          // Prioridad alta para entregar con la app cerrada y el teléfono en reposo.
          android: {priority: 'high', ttl: ttlMs},
        });
        return {ok: true}; // aceptado por FCM; no implica recepción ni lectura
      } catch (error) {
        return mapFcmError(error);
      }
    },
  };
}
