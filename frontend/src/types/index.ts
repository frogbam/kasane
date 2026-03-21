export type Language = 'en' | 'ko' | 'ja' | 'zh';
export type Theme = 'dark' | 'light';

export interface DeviceOption {
  id: string;
  name: string;
  type: string;
}

export interface ChannelOption {
  id: string;
  name: string;
  index: number;
}

export interface AudioState {
  inputGainDb: number;
  outputGainDb: number;
  audioDeviceType: string;
  availableDeviceTypes: string[];
  inputDeviceId: string;
  outputDeviceId: string;
  inputDeviceName: string;
  outputDeviceName: string;
  inputChannelOptions: ChannelOption[];
  leftInputChannelId: string;
  rightInputChannelId: string;
  outputChannelOptions: ChannelOption[];
  leftMonitorChannelId: string;
  rightMonitorChannelId: string;
  bufferSize: number;
  sampleRate: number;
  inputDevices: DeviceOption[];
  outputDevices: DeviceOption[];
  bufferSizeOptions: number[];
  sampleRateOptions: number[];
}

export interface PluginDescriptor {
  id: string;
  name: string;
  manufacturer: string;
  category: string;
  format: string;
  isEnabled: boolean;
}

export interface ChainSlot {
  pluginId: string;
  name: string;
  manufacturer: string;
  category: string;
  order: number;
  bypassed: boolean;
}

export interface PresetSummary {
  id: string;
  name: string;
  updatedAt: string;
  pluginCount: number;
}

export interface TunerState {
  isOpen: boolean;
  note: string;
  frequencyHz: number;
  cents: number;
  signalLevel: number;
}

export interface MeterState {
  inputLeftDb: number;
  inputRightDb: number;
  outputLeftDb: number;
  outputRightDb: number;
}

export interface AppState {
  bridgeVersion: string;
  language: Language;
  theme: Theme;
  bpm: number;
  statusMessage: string;
  lastError: string;
  isScanningPlugins: boolean;
  audio: AudioState;
  tuner: TunerState;
  meters: MeterState;
  availablePlugins: PluginDescriptor[];
  chain: ChainSlot[];
  presets: PresetSummary[];
  currentPresetId: string;
  currentPresetName: string;
  hasUnsavedPresetChanges: boolean;
}

export type CommandParams = {
  frontendReady: void;
  frontendVisualReady: void;
  setLanguage: { languageCode: string };
  setTheme: { themeName: string };
  setBpm: { bpm: number };
  setInputGain: { gainDb: number };
  setOutputGain: { gainDb: number };
  scanPlugins: void;
  addPlugin: { pluginDescriptorId: string };
  removePlugin: { chainPluginId: string };
  togglePlugin: { chainPluginId: string };
  reorderPlugins: { chainPluginId: string; newIndex: number };
  openPluginEditor: { chainPluginId: string };
  loadPreset: { presetId: string };
  saveCurrentPreset: void;
  savePresetAs: { name: string };
  renamePreset: { presetId: string; name: string };
  duplicatePreset: { presetId: string; name: string };
  deletePreset: { presetId: string };
  openAudioSettings: void;
  previewAudioDeviceSetup: {
    inputDeviceId: string;
    outputDeviceId: string;
  };
  setAudioDeviceType: { deviceType: string };
  toggleTuner: { isOpen: boolean };
  setAudioDeviceSetup: {
    inputDeviceId: string;
    outputDeviceId: string;
    sampleRate: number;
    bufferSize: number;
    leftInputChannelId: string;
    rightInputChannelId: string;
    leftMonitorChannelId: string;
    rightMonitorChannelId: string;
  };
};

export type CommandName = keyof CommandParams;

export interface BridgeBackend {
  addEventListener(eventId: string, fn: (payload: unknown) => void): unknown;
  removeEventListener(token: unknown): void;
  emitEvent(eventId: string, payload: unknown): void;
}

declare global {
  interface Window {
    __JUCE__?: {
      backend?: BridgeBackend;
    };
  }
}
