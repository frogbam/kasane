declare global {
  interface Window {
    __JUCE__?: {
      backend?: {
        addEventListener: (eventId: string, fn: (payload: unknown) => void) => unknown;
        removeEventListener: (token: unknown) => void;
        emitEvent: (eventId: string, payload: unknown) => void;
      };
    };
  }
}

const INVOKE_EVENT_ID = "__juce__invoke";
const COMPLETE_EVENT_ID = "__juce__complete";

type Resolver = (value: unknown) => void;

const pending = new Map<number, Resolver>();
let nextPromiseId = 1;
let isInitialised = false;

export function initialiseBridge() {
  if (isInitialised) {
    return;
  }

  isInitialised = true;
  window.__JUCE__?.backend?.addEventListener(COMPLETE_EVENT_ID, (payload) => {
    const completion = payload as { promiseId: number; result: unknown };
    const resolve = pending.get(completion.promiseId);

    if (!resolve) {
      return;
    }

    pending.delete(completion.promiseId);
    resolve(completion.result);
  });
}

export function addBackendListener<T>(eventId: string, handler: (payload: T) => void) {
  return window.__JUCE__?.backend?.addEventListener(eventId, handler as (payload: unknown) => void);
}

export function invokeNative<T>(name: string, ...params: unknown[]): Promise<T> {
  initialiseBridge();

  return new Promise<T>((resolve) => {
    const promiseId = nextPromiseId++;
    pending.set(promiseId, resolve as Resolver);
    window.__JUCE__?.backend?.emitEvent(INVOKE_EVENT_ID, {
      name,
      params,
      resultId: promiseId
    });
  });
}

