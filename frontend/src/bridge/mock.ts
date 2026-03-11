import type { AppState, AudioState, TunerState, MeterState, PluginDescriptor, ChainSlot, ChannelOption } from '../types';

type EventCallback = (payload: unknown) => void;

const INVOKE_EVENT_ID = '__juce__invoke';
const COMPLETE_EVENT_ID = '__juce__complete';

const mockPlugins: PluginDescriptor[] = [
  { id: 'plugin-1', name: 'Classic Amp', manufacturer: 'Kasane', category: 'Amp', format: 'VST3', isEnabled: true },
  { id: 'plugin-2', name: 'Tube Screamer', manufacturer: 'Kasane', category: 'Overdrive', format: 'VST3', isEnabled: true },
  { id: 'plugin-3', name: 'Digital Delay', manufacturer: 'Kasane', category: 'Delay', format: 'VST3', isEnabled: true },
  { id: 'plugin-4', name: 'Spring Reverb', manufacturer: 'Kasane', category: 'Reverb', format: 'VST3', isEnabled: true },
  { id: 'plugin-5', name: 'Chorus Ensemble', manufacturer: 'Kasane', category: 'Modulation', format: 'VST3', isEnabled: true },
  { id: 'plugin-6', name: 'Noise Gate', manufacturer: 'Kasane', category: 'Dynamics', format: 'VST3', isEnabled: true },
];

const mockChain: ChainSlot[] = [
  { pluginId: 'plugin-2', name: 'Tube Screamer', manufacturer: 'Kasane', category: 'Overdrive', order: 0, bypassed: false },
  { pluginId: 'plugin-1', name: 'Classic Amp', manufacturer: 'Kasane', category: 'Amp', order: 1, bypassed: false },
  { pluginId: 'plugin-4', name: 'Spring Reverb', manufacturer: 'Kasane', category: 'Reverb', order: 2, bypassed: true },
];

const mockAudioState: AudioState = {
  inputGainDb: 0,
  outputGainDb: -6,
  audioDeviceType: 'ASIO',
  availableDeviceTypes: ['ASIO', 'Windows Audio'],
  inputDeviceId: 'ASIO::Audio Interface Input 1',
  outputDeviceId: 'ASIO::Audio Interface Output 1',
  inputDeviceName: 'Audio Interface Input 1',
  outputDeviceName: 'Audio Interface Output 1',
  outputChannelOptions: [
    { id: '0', name: 'Out 1', index: 0 },
    { id: '1', name: 'Out 2', index: 1 },
    { id: '2', name: 'Out 3', index: 2 },
    { id: '3', name: 'Out 4', index: 3 },
  ],
  leftMonitorChannelId: '0',
  rightMonitorChannelId: '1',
  bufferSize: 256,
  sampleRate: 48000,
  inputDevices: [
    { id: 'ASIO::Audio Interface Input 1', name: 'Audio Interface Input 1', type: 'ASIO' },
    { id: 'ASIO::Audio Interface Input 2', name: 'Audio Interface Input 2', type: 'ASIO' },
  ],
  outputDevices: [
    { id: 'ASIO::Audio Interface Output 1', name: 'Audio Interface Output 1', type: 'ASIO' },
    { id: 'ASIO::Audio Interface Output 2', name: 'Audio Interface Output 2', type: 'ASIO' },
  ],
  bufferSizeOptions: [64, 128, 256, 512, 1024],
  sampleRateOptions: [44100, 48000, 96000],
};

const mockAppState: AppState = {
  bridgeVersion: '1.0.0-mock',
  language: 'en',
  theme: 'dark',
  statusMessage: 'Ready',
  lastError: '',
  isScanningPlugins: false,
  audio: mockAudioState,
  tuner: {
    isOpen: false,
    note: 'A',
    frequencyHz: 440.0,
    cents: 0,
    signalLevel: 0.75,
  },
  meters: {
    inputLeftDb: -12,
    inputRightDb: -14,
    outputLeftDb: -18,
    outputRightDb: -16,
  },
  availablePlugins: mockPlugins,
  chain: mockChain,
};

class MockBridgeBackend {
  private eventListeners: Map<string, Set<EventCallback>> = new Map();
  private state: AppState;
  private meterInterval: ReturnType<typeof setInterval> | null = null;
  private tunerInterval: ReturnType<typeof setInterval> | null = null;

  constructor() {
    this.state = JSON.parse(JSON.stringify(mockAppState));
  }

  addEventListener(eventId: string, fn: EventCallback): unknown {
    if (!this.eventListeners.has(eventId)) {
      this.eventListeners.set(eventId, new Set());
    }
    this.eventListeners.get(eventId)!.add(fn);
    return { eventId, fn };
  }

  removeEventListener(token: unknown): void {
    if (typeof token !== 'object' || token === null) return;
    const { eventId, fn } = token as { eventId: string; fn: EventCallback };
    const listeners = this.eventListeners.get(eventId);
    if (listeners) {
      listeners.delete(fn);
    }
  }

  emitEvent(eventId: string, payload: unknown): void {
    if (eventId === INVOKE_EVENT_ID) {
      const invokePayload = payload as {
        name: string;
        params: unknown[];
        resultId: number;
      };

      const result = this.handleCommand(invokePayload.name, invokePayload.params);
      this.emit(COMPLETE_EVENT_ID, {
        promiseId: invokePayload.resultId,
        result,
      });
      return;
    }

    this.handleCommand(eventId, payload);
  }

  private handleCommand(command: string, payload: unknown): unknown {
    switch (command) {
      case 'frontendReady':
        setTimeout(() => this.emit('bootstrapState', this.state), 50);
        this.startMeterSimulation();
        return this.state;

      case 'setLanguage':
        this.state.language = this.readStringArg(payload, 0) as AppState['language'];
        this.emit('bootstrapState', this.state);
        return true;

      case 'setAudioDeviceType':
        this.applyDeviceType(this.readStringArg(payload, 0));
        this.emit('audioStateChanged', this.state.audio);
        return true;

      case 'setTheme':
        this.state.theme = this.readStringArg(payload, 0) as AppState['theme'];
        this.emit('bootstrapState', this.state);
        return true;

      case 'setInputGain':
        this.state.audio.inputGainDb = this.readNumberArg(payload, 0);
        this.emit('audioStateChanged', this.state.audio);
        return true;

      case 'setOutputGain':
        this.state.audio.outputGainDb = this.readNumberArg(payload, 0);
        this.emit('audioStateChanged', this.state.audio);
        return true;

      case 'scanPlugins':
        this.state.isScanningPlugins = true;
        this.emit('pluginListChanged', this.state);
        setTimeout(() => {
          this.state.isScanningPlugins = false;
          this.state.availablePlugins = [...mockPlugins];
          this.emit('pluginListChanged', this.state);
        }, 1500);
        return true;

      case 'addPlugin':
        const pluginId = this.readStringArg(payload, 0);
        const plugin = mockPlugins.find(p => p.id === pluginId);
        if (plugin) {
          const newSlot: ChainSlot = {
            pluginId: plugin.id,
            name: plugin.name,
            manufacturer: plugin.manufacturer,
            category: plugin.category,
            order: this.state.chain.length,
            bypassed: false,
          };
          this.state.chain.push(newSlot);
          this.emit('pluginChainChanged', this.state);
        }
        return plugin != null;

      case 'removePlugin':
        const removeId = this.readStringArg(payload, 0);
        this.state.chain = this.state.chain.filter(c => c.pluginId !== removeId);
        this.state.chain.forEach((c, i) => c.order = i);
        this.emit('pluginChainChanged', this.state);
        return true;

      case 'togglePlugin':
        const toggleId = this.readStringArg(payload, 0);
        const slot = this.state.chain.find(c => c.pluginId === toggleId);
        if (slot) {
          slot.bypassed = !slot.bypassed;
          this.emit('pluginChainChanged', this.state);
        }
        return slot != null;

      case 'reorderPlugins':
        const chainPluginId = this.readStringArg(payload, 0);
        const newIndex = this.readNumberArg(payload, 1);
        const currentIndex = this.state.chain.findIndex(c => c.pluginId === chainPluginId);
        if (currentIndex !== -1 && newIndex >= 0 && newIndex < this.state.chain.length) {
          const [item] = this.state.chain.splice(currentIndex, 1);
          this.state.chain.splice(newIndex, 0, item);
          this.state.chain.forEach((c, i) => c.order = i);
          this.emit('pluginChainChanged', this.state);
        }
        return currentIndex !== -1;

      case 'openPluginEditor':
        console.log('[Mock] Opening plugin editor for:', this.readStringArg(payload, 0));
        return true;

      case 'openAudioSettings':
        this.emit('audioStateChanged', this.state.audio);
        return this.state.audio;

      case 'previewAudioDeviceSetup':
        return this.previewAudioState(this.readStringArg(payload, 0), this.readStringArg(payload, 1));

      case 'toggleTuner':
        this.state.tuner.isOpen = this.readBooleanArg(payload, 0);
        this.emit('tunerUpdated', this.state.tuner);
        if (this.state.tuner.isOpen) {
          this.startTunerSimulation();
        } else {
          this.stopTunerSimulation();
        }
        return true;

      case 'setAudioDeviceSetup':
        this.state.audio.inputDeviceId = this.readStringArg(payload, 0);
        this.state.audio.outputDeviceId = this.readStringArg(payload, 1);
        this.state.audio.sampleRate = this.readNumberArg(payload, 2);
        this.state.audio.bufferSize = this.readNumberArg(payload, 3);
        this.state.audio.leftMonitorChannelId = this.readStringArg(payload, 4);
        this.state.audio.rightMonitorChannelId = this.readStringArg(payload, 5);
        this.state.audio.inputDeviceName = this.state.audio.inputDevices.find((device) => device.id === this.state.audio.inputDeviceId)?.name ?? '';
        this.state.audio.outputDeviceName = this.state.audio.outputDevices.find((device) => device.id === this.state.audio.outputDeviceId)?.name ?? '';
        this.emit('audioStateChanged', this.state.audio);
        return true;
    }

    return null;
  }

  private emit(eventId: string, payload: unknown): void {
    const listeners = this.eventListeners.get(eventId);
    if (listeners) {
      listeners.forEach(fn => fn(payload));
    }
  }

  private readStringArg(payload: unknown, index: number): string {
    if (!Array.isArray(payload)) {
      return '';
    }

    const value = payload[index];
    return typeof value === 'string' ? value : '';
  }

  private readNumberArg(payload: unknown, index: number): number {
    if (!Array.isArray(payload)) {
      return 0;
    }

    const value = payload[index];
    return typeof value === 'number' ? value : 0;
  }

  private readBooleanArg(payload: unknown, index: number): boolean {
    if (!Array.isArray(payload)) {
      return false;
    }

    const value = payload[index];
    return typeof value === 'boolean' ? value : false;
  }

  private applyDeviceType(deviceType: string): void {
    this.state.audio.audioDeviceType = deviceType;
    let channelOptions: ChannelOption[];

    if (deviceType === 'Windows Audio') {
      this.state.audio.inputDevices = [
        { id: 'Windows Audio::Microphone Array', name: 'Microphone Array', type: 'Windows Audio' },
        { id: 'Windows Audio::Line In', name: 'Line In', type: 'Windows Audio' },
      ];
      this.state.audio.outputDevices = [
        { id: 'Windows Audio::Speakers', name: 'Speakers', type: 'Windows Audio' },
        { id: 'Windows Audio::Headphones', name: 'Headphones', type: 'Windows Audio' },
      ];
      this.state.audio.sampleRateOptions = [44100, 48000];
      this.state.audio.bufferSizeOptions = [128, 256, 512, 1024];
      channelOptions = [
        { id: '0', name: 'Speaker 1', index: 0 },
        { id: '1', name: 'Speaker 2', index: 1 },
      ];
    } else {
      this.state.audio.inputDevices = [
        { id: 'ASIO::Audio Interface Input 1', name: 'Audio Interface Input 1', type: 'ASIO' },
        { id: 'ASIO::Audio Interface Input 2', name: 'Audio Interface Input 2', type: 'ASIO' },
      ];
      this.state.audio.outputDevices = [
        { id: 'ASIO::Audio Interface Output 1', name: 'Audio Interface Output 1', type: 'ASIO' },
        { id: 'ASIO::Audio Interface Output 2', name: 'Audio Interface Output 2', type: 'ASIO' },
      ];
      this.state.audio.sampleRateOptions = [44100, 48000, 96000];
      this.state.audio.bufferSizeOptions = [64, 128, 256, 512, 1024];
      channelOptions = [
        { id: '0', name: 'Out 1', index: 0 },
        { id: '1', name: 'Out 2', index: 1 },
        { id: '2', name: 'Out 3', index: 2 },
        { id: '3', name: 'Out 4', index: 3 },
      ];
    }

    this.state.audio.outputChannelOptions = channelOptions;
    this.state.audio.inputDeviceId = this.state.audio.inputDevices[0]?.id ?? '';
    this.state.audio.outputDeviceId = this.state.audio.outputDevices[0]?.id ?? '';
    this.state.audio.inputDeviceName = this.state.audio.inputDevices[0]?.name ?? '';
    this.state.audio.outputDeviceName = this.state.audio.outputDevices[0]?.name ?? '';
    this.state.audio.leftMonitorChannelId = channelOptions[0]?.id ?? '0';
    this.state.audio.rightMonitorChannelId = channelOptions[1]?.id ?? channelOptions[0]?.id ?? '0';
    this.state.audio.sampleRate = this.state.audio.sampleRateOptions[0] ?? 44100;
    this.state.audio.bufferSize = this.state.audio.bufferSizeOptions[0] ?? 256;
  }

  private previewAudioState(inputDeviceId: string, outputDeviceId: string): AudioState {
    const preview = JSON.parse(JSON.stringify(this.state.audio)) as AudioState;

    preview.inputDeviceId = inputDeviceId || preview.inputDeviceId;
    preview.outputDeviceId = outputDeviceId || preview.outputDeviceId;
    preview.inputDeviceName = preview.inputDevices.find((device) => device.id === preview.inputDeviceId)?.name ?? preview.inputDeviceName;
    preview.outputDeviceName = preview.outputDevices.find((device) => device.id === preview.outputDeviceId)?.name ?? preview.outputDeviceName;

    if (preview.outputDeviceId.endsWith('Output 2')) {
      preview.outputChannelOptions = [
        { id: '2', name: 'Out 3', index: 2 },
        { id: '3', name: 'Out 4', index: 3 },
      ];
      preview.leftMonitorChannelId = '2';
      preview.rightMonitorChannelId = '3';
    } else {
      preview.outputChannelOptions = [
        { id: '0', name: 'Out 1', index: 0 },
        { id: '1', name: 'Out 2', index: 1 },
      ];
      preview.leftMonitorChannelId = '0';
      preview.rightMonitorChannelId = '1';
    }

    return preview;
  }

  private startMeterSimulation(): void {
    if (this.meterInterval) return;
    this.meterInterval = setInterval(() => {
      const inputDb = -20 + Math.random() * 15;
      const outputDb = -25 + Math.random() * 15;
      this.state.meters = {
        inputLeftDb: inputDb,
        inputRightDb: inputDb + (Math.random() * 4 - 2),
        outputLeftDb: outputDb,
        outputRightDb: outputDb + (Math.random() * 4 - 2),
      };
      this.emit('meterUpdated', this.state.meters);
    }, 50);
  }

  private startTunerSimulation(): void {
    if (this.tunerInterval) return;
    const notes = ['E', 'A', 'D', 'G', 'B', 'E'];
    const frequencies = [82.41, 110.0, 146.83, 196.0, 246.94, 329.63];
    let noteIndex = 0;
    
    this.tunerInterval = setInterval(() => {
      noteIndex = (noteIndex + 1) % notes.length;
      this.state.tuner.note = notes[noteIndex];
      this.state.tuner.frequencyHz = frequencies[noteIndex] + (Math.random() * 2 - 1);
      this.state.tuner.cents = Math.round((Math.random() * 40) - 20);
      this.state.tuner.signalLevel = 0.5 + Math.random() * 0.5;
      this.emit('tunerUpdated', this.state.tuner);
    }, 2000);
  }

  private stopTunerSimulation(): void {
    if (this.tunerInterval) {
      clearInterval(this.tunerInterval);
      this.tunerInterval = null;
    }
  }
}

export function createMockBridge(): MockBridgeBackend {
  return new MockBridgeBackend();
}

export function setupMockBridge(): void {
  if (typeof window === 'undefined') return;
  
  if (!window.__JUCE__?.backend) {
    window.__JUCE__ = {
      backend: createMockBridge() as unknown as import('../types').BridgeBackend,
    };
  }
}
