export type Language = "en" | "ko" | "ja" | "zh";
export type Theme = "dark" | "light";

export interface DeviceOption {
  id: string;
  name: string;
  type: string;
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

export interface AudioState {
  inputGainDb: number;
  outputGainDb: number;
  audioDeviceType: string;
  inputDeviceId: string;
  outputDeviceId: string;
  inputDeviceName: string;
  outputDeviceName: string;
  bufferSize: number;
  sampleRate: number;
  inputDevices: DeviceOption[];
  outputDevices: DeviceOption[];
  bufferSizeOptions: number[];
  sampleRateOptions: number[];
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
  statusMessage: string;
  lastError: string;
  isScanningPlugins: boolean;
  audio: AudioState;
  tuner: TunerState;
  meters: MeterState;
  availablePlugins: PluginDescriptor[];
  chain: ChainSlot[];
}

export const defaultAppState: AppState = {
  bridgeVersion: "1.0.0",
  language: "en",
  theme: "dark",
  statusMessage: "",
  lastError: "",
  isScanningPlugins: false,
  audio: {
    inputGainDb: 0,
    outputGainDb: 0,
    audioDeviceType: "",
    inputDeviceId: "",
    outputDeviceId: "",
    inputDeviceName: "",
    outputDeviceName: "",
    bufferSize: 0,
    sampleRate: 0,
    inputDevices: [],
    outputDevices: [],
    bufferSizeOptions: [],
    sampleRateOptions: []
  },
  tuner: {
    isOpen: false,
    note: "--",
    frequencyHz: 0,
    cents: 0,
    signalLevel: 0
  },
  meters: {
    inputLeftDb: -100,
    inputRightDb: -100,
    outputLeftDb: -100,
    outputRightDb: -100
  },
  availablePlugins: [],
  chain: []
};

