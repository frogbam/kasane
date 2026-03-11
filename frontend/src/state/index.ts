import { signal, computed } from '@preact/signals';
import type { AppState, AudioState, TunerState, MeterState, PluginDescriptor, ChainSlot, Language, Theme } from '../types';

const initialState: AppState = {
  bridgeVersion: '',
  language: 'en',
  theme: 'dark',
  bpm: 120,
  statusMessage: '',
  lastError: '',
  isScanningPlugins: false,
  audio: {
    inputGainDb: 0,
    outputGainDb: 0,
    audioDeviceType: '',
    availableDeviceTypes: [],
    inputDeviceId: '',
    outputDeviceId: '',
    inputDeviceName: '',
    outputDeviceName: '',
    outputChannelOptions: [],
    leftMonitorChannelId: '0',
    rightMonitorChannelId: '1',
    bufferSize: 256,
    sampleRate: 48000,
    inputDevices: [],
    outputDevices: [],
    bufferSizeOptions: [64, 128, 256, 512, 1024],
    sampleRateOptions: [44100, 48000, 96000],
  },
  tuner: {
    isOpen: false,
    note: '',
    frequencyHz: 0,
    cents: 0,
    signalLevel: 0,
  },
  meters: {
    inputLeftDb: -60,
    inputRightDb: -60,
    outputLeftDb: -60,
    outputRightDb: -60,
  },
  availablePlugins: [],
  chain: [],
};

export const bridgeVersion = signal<string>(initialState.bridgeVersion);
export const language = signal<Language>(initialState.language);
export const theme = signal<Theme>(initialState.theme);
export const bpm = signal<number>(initialState.bpm);
export const statusMessage = signal<string>(initialState.statusMessage);
export const lastError = signal<string>(initialState.lastError);
export const isScanningPlugins = signal<boolean>(initialState.isScanningPlugins);

export const audio = signal<AudioState>(initialState.audio);
export const tuner = signal<TunerState>(initialState.tuner);
export const meters = signal<MeterState>(initialState.meters);
export const availablePlugins = signal<PluginDescriptor[]>(initialState.availablePlugins);
export const chain = signal<ChainSlot[]>(initialState.chain);

export const inputGainDb = computed(() => audio.value.inputGainDb);
export const outputGainDb = computed(() => audio.value.outputGainDb);
export const inputDeviceName = computed(() => audio.value.inputDeviceName);
export const outputDeviceName = computed(() => audio.value.outputDeviceName);

export const tunerNote = computed(() => tuner.value.note);
export const tunerFrequency = computed(() => tuner.value.frequencyHz);
export const tunerCents = computed(() => tuner.value.cents);
export const tunerSignalLevel = computed(() => tuner.value.signalLevel);
export const isTunerOpen = computed(() => tuner.value.isOpen);

export const inputMeters = computed(() => ({
  left: meters.value.inputLeftDb,
  right: meters.value.inputRightDb,
}));

export const outputMeters = computed(() => ({
  left: meters.value.outputLeftDb,
  right: meters.value.outputRightDb,
}));

export function setAppState(newState: AppState): void {
  bridgeVersion.value = newState.bridgeVersion;
  language.value = newState.language;
  theme.value = newState.theme;
  bpm.value = newState.bpm;
  statusMessage.value = newState.statusMessage;
  lastError.value = newState.lastError;
  isScanningPlugins.value = newState.isScanningPlugins;
  audio.value = { ...newState.audio };
  tuner.value = { ...newState.tuner };
  meters.value = { ...newState.meters };
  availablePlugins.value = [...newState.availablePlugins];
  chain.value = [...newState.chain];
}

export function updateAudioState(newAudio: AudioState): void {
  audio.value = { ...newAudio };
}

export function updateTunerState(newTuner: TunerState): void {
  tuner.value = { ...newTuner };
}

export function updateMeters(newMeters: MeterState): void {
  meters.value = { ...newMeters };
}

export function updatePluginList(plugins: PluginDescriptor[]): void {
  availablePlugins.value = [...plugins];
}

export function updateChain(newChain: ChainSlot[]): void {
  chain.value = [...newChain];
}

export function setLanguageState(newLanguage: Language): void {
  language.value = newLanguage;
}

export function setThemeState(newTheme: Theme): void {
  theme.value = newTheme;
}

export function setScanningState(scanning: boolean): void {
  isScanningPlugins.value = scanning;
}

export function setError(error: string): void {
  lastError.value = error;
}

export function clearError(): void {
  lastError.value = '';
}
