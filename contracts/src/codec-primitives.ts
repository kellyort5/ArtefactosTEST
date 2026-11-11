/** Shared bounded JSON handling. Errors never echo a received payload. */
export class ContractError extends Error {
  constructor(readonly code: 'invalid_message' | 'unsupported_version' | 'message_too_large') {
    super(code);
    this.name = 'ContractError';
  }
}

const utf8 = new TextEncoder();

export function parseJsonBounded(input: Uint8Array | string, limit: number): unknown {
  const bytes = typeof input === 'string' ? utf8.encode(input) : input;
  if (bytes.byteLength > limit) throw new ContractError('message_too_large');
  try {
    return JSON.parse(new TextDecoder('utf-8', { fatal: true }).decode(bytes));
  } catch {
    throw new ContractError('invalid_message');
  }
}

export function stringifyContract(value: unknown): string {
  try {
    const text = JSON.stringify(value);
    if (typeof text !== 'string') throw new Error();
    return text;
  } catch {
    throw new ContractError('invalid_message');
  }
}

export function record(value: unknown): value is Record<string, unknown> {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}
