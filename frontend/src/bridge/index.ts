import type { CommandName, CommandParams, BridgeBackend } from '../types';

type EventCallback = (payload: unknown) => void;

const INVOKE_EVENT_ID = '__juce__invoke';
const COMPLETE_EVENT_ID = '__juce__complete';

type CompletionPayload = {
  promiseId: number;
  result: unknown;
};

class Bridge {
  private backend: BridgeBackend | null = null;
  private isInitialized = false;
  private nextPromiseId = 1;
  private pendingCompletions = new Map<number, (result: unknown) => void>();
  private completionListenerToken: unknown = null;

  init(): boolean {
    if (typeof window === 'undefined') return false;

    const juce = window.__JUCE__;
    if (!juce?.backend) {
      console.warn('[Bridge] JUCE backend not available, using mock');
      return false;
    }

    this.backend = juce.backend;

    if (!this.isInitialized) {
      this.completionListenerToken = this.backend.addEventListener(COMPLETE_EVENT_ID, (payload) => {
        const completion = payload as CompletionPayload;
        const resolve = this.pendingCompletions.get(completion.promiseId);

        if (!resolve) {
          return;
        }

        this.pendingCompletions.delete(completion.promiseId);
        resolve(completion.result);
      });

      this.isInitialized = true;
    }

    return true;
  }

  addEventListener(eventId: string, callback: EventCallback): unknown {
    if (!this.backend) {
      console.warn(`[Bridge] Cannot add listener for "${eventId}" - no backend`);
      return null;
    }
    return this.backend.addEventListener(eventId, callback);
  }

  removeEventListener(token: unknown): void {
    if (!this.backend) return;
    this.backend.removeEventListener(token);
  }

  emit<K extends CommandName>(command: K, params?: CommandParams[K]): void {
    if (!this.backend) {
      console.warn(`[Bridge] Cannot emit "${command}" - no backend`);
      return;
    }

    void this.invoke(command, params);
  }

  request<K extends CommandName>(command: K, params?: CommandParams[K]): Promise<unknown> {
    if (!this.backend) {
      return Promise.resolve(null);
    }

    return this.invoke(command, params);
  }

  isReady(): boolean {
    return this.isInitialized && this.backend !== null;
  }

  private invoke<K extends CommandName>(command: K, params?: CommandParams[K]): Promise<unknown> {
    if (!this.backend) {
      return Promise.resolve(null);
    }

    const promiseId = this.nextPromiseId++;
    const nativeParams = this.toNativeParams(command, params);

    return new Promise((resolve) => {
      this.pendingCompletions.set(promiseId, resolve);
      this.backend!.emitEvent(INVOKE_EVENT_ID, {
        name: command,
        params: nativeParams,
        resultId: promiseId,
      });
    });
  }

  private toNativeParams<K extends CommandName>(command: K, params?: CommandParams[K]): unknown[] {
    switch (command) {
      case 'frontendReady':
      case 'scanPlugins':
      case 'openAudioSettings':
        return [];

      case 'previewAudioDeviceSetup':
        return [params?.inputDeviceId ?? '', params?.outputDeviceId ?? ''];

      case 'setAudioDeviceType':
        return [params?.deviceType ?? ''];

      case 'setLanguage':
        return [params?.languageCode ?? 'en'];

      case 'setTheme':
        return [params?.themeName ?? 'dark'];

      case 'setBpm':
        return [params?.bpm ?? 120];

      case 'setInputGain':
      case 'setOutputGain':
        return [params?.gainDb ?? 0];

      case 'addPlugin':
        return [params?.pluginDescriptorId ?? ''];

      case 'removePlugin':
      case 'togglePlugin':
      case 'openPluginEditor':
        return [params?.chainPluginId ?? ''];

      case 'reorderPlugins':
        return [params?.chainPluginId ?? '', params?.newIndex ?? 0];

      case 'toggleTuner':
        return [params?.isOpen ?? false];

      case 'setAudioDeviceSetup':
        return [
          params?.inputDeviceId ?? '',
          params?.outputDeviceId ?? '',
          params?.sampleRate ?? 0,
          params?.bufferSize ?? 0,
          params?.leftInputChannelId ?? '',
          params?.rightInputChannelId ?? '',
          params?.leftMonitorChannelId ?? '',
          params?.rightMonitorChannelId ?? '',
        ];
    }
  }
}

export const bridge = new Bridge();
