import { useEffect } from 'preact/hooks';
import { useSignal } from '@preact/signals';
import { Header } from './features/header';
import { ChainView } from './features/chain-view';
import { BottomBar } from './features/bottom-bar';
import { SettingsOverlay } from './features/settings-overlay';
import { TunerOverlay } from './features/tuner-overlay';
import { PluginPickerOverlay } from './features/plugin-picker';
import { bridge } from './bridge';
import { setupMockBridge } from './bridge/mock';
import {
  setAppState,
  updateAudioState,
  updateTunerState,
  updateMeters,
  updatePluginList,
  updateChain,
  setLanguageState,
  setThemeState,
  setScanningState,
  setError,
  theme as themeSignal,
} from './state';
import type { AppState, AudioState } from './types';
import './styles/global.css';

export function App() {
  const settingsOpen = useSignal(false);
  const tunerOpen = useSignal(false);
  const pluginPickerOpen = useSignal(false);

  useEffect(() => {
    initializeApp();
    return () => {
      cleanupApp();
    };
  }, []);

  const eventTokens = useSignal<unknown[]>([]);

  const initializeApp = () => {
    const bridgeReady = bridge.init();
    if (!bridgeReady) {
      setupMockBridge();
      bridge.init();
    }

    eventTokens.value = [
      bridge.addEventListener('bootstrapState', handleBootstrapState),
      bridge.addEventListener('audioStateChanged', handleAudioStateChanged),
      bridge.addEventListener('pluginListChanged', handlePluginListChanged),
      bridge.addEventListener('pluginChainChanged', handlePluginChainChanged),
      bridge.addEventListener('tunerUpdated', handleTunerUpdated),
      bridge.addEventListener('meterUpdated', handleMeterUpdated),
      bridge.addEventListener('errorRaised', handleErrorRaised),
    ];

    setTimeout(() => {
      bridge.emit('frontendReady');
    }, 100);
  };

  const cleanupApp = () => {
    eventTokens.value.forEach((token) => {
      if (token != null) {
        bridge.removeEventListener(token);
      }
    });
    eventTokens.value = [];
  };

  const handleBootstrapState = (payload: unknown) => {
    const state = payload as AppState;
    setAppState(state);
    applyTheme(state.theme);
  };

  const handleAudioStateChanged = (payload: unknown) => {
    const state = payload as AudioState;
    updateAudioState(state);
  };

  const handlePluginListChanged = (payload: unknown) => {
    const state = payload as AppState;
    updatePluginList(state.availablePlugins);
    setScanningState(state.isScanningPlugins);
  };

  const handlePluginChainChanged = (payload: unknown) => {
    const state = payload as AppState;
    updateChain(state.chain);
  };

  const handleTunerUpdated = (payload: unknown) => {
    const state = payload as AppState['tuner'];
    updateTunerState(state);
    tunerOpen.value = state.isOpen;
  };

  const handleMeterUpdated = (payload: unknown) => {
    const meters = payload as AppState['meters'];
    updateMeters(meters);
  };

  const handleErrorRaised = (payload: unknown) => {
    const errorMessage = payload as string;
    setError(errorMessage);
  };

  const applyTheme = (themeName: 'dark' | 'light') => {
    document.documentElement.setAttribute('data-theme', themeName);
  };

  const handleOpenSettings = () => {
    bridge.emit('openAudioSettings');
    settingsOpen.value = true;
  };

  const handleCloseSettings = () => {
    settingsOpen.value = false;
  };

  const handleOpenTuner = () => {
    bridge.emit('toggleTuner', { isOpen: true });
    tunerOpen.value = true;
  };

  const handleCloseTuner = () => {
    bridge.emit('toggleTuner', { isOpen: false });
    tunerOpen.value = false;
  };

  const handleOpenPluginPicker = () => {
    pluginPickerOpen.value = true;
  };

  const handleClosePluginPicker = () => {
    pluginPickerOpen.value = false;
  };

  const handleSelectPlugin = (pluginId: string) => {
    bridge.emit('addPlugin', { pluginDescriptorId: pluginId });
  };

  return (
    <div class="app-layout" data-theme={themeSignal.value}>
      <Header onOpenSettings={handleOpenSettings} onOpenTuner={handleOpenTuner} />
      <main class="main-content">
        <ChainView onAddPlugin={handleOpenPluginPicker} />
      </main>
      <BottomBar onOpenSettings={handleOpenSettings} />
      <SettingsOverlay isOpen={settingsOpen.value} onClose={handleCloseSettings} />
      <TunerOverlay isOpen={tunerOpen.value} onClose={handleCloseTuner} />
      <PluginPickerOverlay
        isOpen={pluginPickerOpen.value}
        onClose={handleClosePluginPicker}
        onSelect={handleSelectPlugin}
      />
    </div>
  );
}
