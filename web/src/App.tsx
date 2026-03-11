import { useEffect, useRef } from "preact/hooks";
import Sortable from "sortablejs";
import en from "./i18n/en.json";
import ja from "./i18n/ja.json";
import ko from "./i18n/ko.json";
import zh from "./i18n/zh.json";
import { addBackendListener, initialiseBridge, invokeNative } from "./bridge";
import { appState, selectedPluginId, settingsOpen } from "./state";
import type { AppState, AudioState, ChainSlot, Language, MeterState, Theme, TunerState } from "./types";

const dictionaries = { en, ko, ja, zh };

function setAppState(nextState: AppState) {
  appState.value = nextState;
  document.documentElement.dataset.theme = nextState.theme;
  document.title = `${nextState.language === "ko" ? "Kasane" : "Kasane"} | ${nextState.statusMessage || nextState.audio.inputDeviceName || "Ready"}`;
}

function mergeAudioState(nextAudio: AudioState) {
  setAppState({ ...appState.value, audio: nextAudio });
}

function mergeTunerState(nextTuner: TunerState) {
  setAppState({ ...appState.value, tuner: nextTuner });
}

function mergeMeterState(nextMeters: MeterState) {
  appState.value = { ...appState.value, meters: nextMeters };
}

function t(key: string) {
  const language = appState.value.language;
  return dictionaries[language][key as keyof typeof en] ?? key;
}

function dbToPercent(value: number) {
  const min = -60;
  const max = 6;
  return Math.max(0, Math.min(100, ((value - min) / (max - min)) * 100));
}

function toneOffset(cents: number) {
  return Math.max(-100, Math.min(100, cents));
}

export function App() {
  const chainRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    initialiseBridge();

    const disposers = [
      addBackendListener<AppState>("bootstrapState", setAppState),
      addBackendListener<AppState>("pluginListChanged", setAppState),
      addBackendListener<AppState>("pluginChainChanged", setAppState),
      addBackendListener<AudioState>("audioStateChanged", mergeAudioState),
      addBackendListener<TunerState>("tunerUpdated", mergeTunerState),
      addBackendListener<MeterState>("meterUpdated", mergeMeterState),
      addBackendListener<string>("errorRaised", (message) => {
        appState.value = { ...appState.value, lastError: message };
      })
    ];

    invokeNative<AppState>("frontendReady").then(setAppState);

    return () => {
      disposers.forEach((token) => {
        if (token) {
          window.__JUCE__?.backend?.removeEventListener(token);
        }
      });
    };
  }, []);

  useEffect(() => {
    if (!chainRef.current) {
      return;
    }

    const sortable = Sortable.create(chainRef.current, {
      animation: 160,
      ghostClass: "plugin-card--ghost",
      draggable: ".plugin-card",
      onEnd: (event) => {
        const movedId = event.item.getAttribute("data-plugin-id");
        if (!movedId || event.newIndex == null) {
          return;
        }

        void invokeNative<boolean>("reorderPlugins", movedId, event.newIndex);
      }
    });

    return () => sortable.destroy();
  }, [appState.value.chain.length]);

  const state = appState.value;

  const applyAudioSettings = async () => {
    await invokeNative<boolean>(
      "setAudioDeviceSetup",
      state.audio.inputDeviceId,
      state.audio.outputDeviceId,
      state.audio.sampleRate,
      state.audio.bufferSize
    );
    settingsOpen.value = false;
  };

  const onThemeToggle = async () => {
    const nextTheme: Theme = state.theme === "dark" ? "light" : "dark";
    await invokeNative<boolean>("setTheme", nextTheme);
  };

  const onLanguageChange = async (event: Event) => {
    const nextLanguage = (event.currentTarget as HTMLSelectElement).value as Language;
    await invokeNative<boolean>("setLanguage", nextLanguage);
  };

  return (
    <main class="app-shell">
      <header class="topbar">
        <div>
          <p class="eyebrow">{t("app.title")}</p>
          <h1>{t("app.subtitle")}</h1>
        </div>
        <div class="toolbar">
          <select class="toolbar-select" value={state.language} onChange={onLanguageChange}>
            <option value="en">EN</option>
            <option value="ko">KO</option>
            <option value="ja">JA</option>
            <option value="zh">ZH</option>
          </select>
          <button class="toolbar-button" type="button" onClick={onThemeToggle}>
            {state.theme === "dark" ? t("header.theme.dark") : t("header.theme.light")}
          </button>
          <button
            class="toolbar-button"
            type="button"
            onClick={async () => {
              await invokeNative<boolean>("toggleTuner", !state.tuner.isOpen);
            }}
          >
            {t("header.tuner")}
          </button>
          <button
            class="toolbar-button"
            type="button"
            onClick={async () => {
              settingsOpen.value = true;
              await invokeNative<AudioState>("openAudioSettings");
            }}
          >
            {t("header.settings")}
          </button>
          <button
            class="toolbar-button toolbar-button--accent"
            type="button"
            onClick={async () => {
              appState.value = { ...appState.value, isScanningPlugins: true };
              await invokeNative<boolean>("scanPlugins");
            }}
          >
            {state.isScanningPlugins ? t("status.scanning") : t("header.scan")}
          </button>
        </div>
      </header>

      <section class="status-strip">
        <span>{state.statusMessage || `${state.audio.audioDeviceType || "Audio"} ready`}</span>
        {state.lastError ? <strong>{state.lastError}</strong> : null}
      </section>

      <section class="grid">
        <article class="panel">
          <div class="panel-head">
            <h2>{t("audio.title")}</h2>
            <span>{state.audio.audioDeviceType || "No device"}</span>
          </div>

          <div class="meter-block">
            <div class="meter-row">
              <label>{t("audio.input")}</label>
              <span>{state.audio.inputDeviceName || "-"}</span>
            </div>
            <div class="dual-meter">
              <div class="meter-track"><div class="meter-fill" style={{ width: `${dbToPercent(state.meters.inputLeftDb)}%` }} /></div>
              <div class="meter-track"><div class="meter-fill" style={{ width: `${dbToPercent(state.meters.inputRightDb)}%` }} /></div>
            </div>
          </div>

          <div class="slider-block">
            <div class="slider-head">
              <span>{t("audio.inputGain")}</span>
              <strong>{state.audio.inputGainDb.toFixed(1)} dB</strong>
            </div>
            <input
              type="range"
              min={-60}
              max={20}
              step={0.5}
              value={state.audio.inputGainDb}
              onInput={(event) => void invokeNative<boolean>("setInputGain", Number((event.currentTarget as HTMLInputElement).value))}
            />
          </div>

          <div class="meter-block">
            <div class="meter-row">
              <label>{t("audio.output")}</label>
              <span>{state.audio.outputDeviceName || "-"}</span>
            </div>
            <div class="dual-meter">
              <div class="meter-track"><div class="meter-fill meter-fill--blue" style={{ width: `${dbToPercent(state.meters.outputLeftDb)}%` }} /></div>
              <div class="meter-track"><div class="meter-fill meter-fill--blue" style={{ width: `${dbToPercent(state.meters.outputRightDb)}%` }} /></div>
            </div>
          </div>

          <div class="slider-block">
            <div class="slider-head">
              <span>{t("audio.outputGain")}</span>
              <strong>{state.audio.outputGainDb.toFixed(1)} dB</strong>
            </div>
            <input
              type="range"
              min={-60}
              max={20}
              step={0.5}
              value={state.audio.outputGainDb}
              onInput={(event) => void invokeNative<boolean>("setOutputGain", Number((event.currentTarget as HTMLInputElement).value))}
            />
          </div>
        </article>

        <article class="panel panel--wide">
          <div class="panel-head">
            <h2>{t("plugins.title")}</h2>
            <div class="plugin-add">
              <select
                class="toolbar-select"
                value={selectedPluginId.value}
                onChange={(event) => {
                  selectedPluginId.value = (event.currentTarget as HTMLSelectElement).value;
                }}
              >
                <option value="">Select VST3</option>
                {state.availablePlugins.map((plugin) => (
                  <option key={plugin.id} value={plugin.id}>
                    {plugin.name} · {plugin.manufacturer}
                  </option>
                ))}
              </select>
              <button
                class="toolbar-button toolbar-button--accent"
                type="button"
                disabled={!selectedPluginId.value}
                onClick={async () => {
                  await invokeNative<boolean>("addPlugin", selectedPluginId.value);
                  selectedPluginId.value = "";
                }}
              >
                {t("plugins.add")}
              </button>
            </div>
          </div>

          {state.chain.length === 0 ? (
            <div class="empty-state">{t("plugins.empty")}</div>
          ) : (
            <div class="chain-track" ref={chainRef}>
              {state.chain.map((plugin: ChainSlot) => (
                <article class="plugin-card" data-plugin-id={plugin.pluginId} key={plugin.pluginId}>
                  <div class="plugin-card__head">
                    <div>
                      <strong>{plugin.name}</strong>
                      <span>{plugin.manufacturer}</span>
                    </div>
                    <button
                      class={`status-dot ${plugin.bypassed ? "status-dot--off" : ""}`}
                      type="button"
                      onClick={() => void invokeNative<boolean>("togglePlugin", plugin.pluginId)}
                      aria-label={plugin.bypassed ? t("plugins.bypassed") : "Enabled"}
                    />
                  </div>
                  <p>{plugin.category || "Effect"}</p>
                  <div class="plugin-card__actions">
                    <button class="ghost-button" type="button" onClick={() => void invokeNative<boolean>("openPluginEditor", plugin.pluginId)}>
                      {t("plugins.editor")}
                    </button>
                    <button class="ghost-button" type="button" onClick={() => void invokeNative<boolean>("removePlugin", plugin.pluginId)}>
                      {t("plugins.remove")}
                    </button>
                  </div>
                </article>
              ))}
            </div>
          )}
        </article>
      </section>

      {state.tuner.isOpen ? (
        <section class="overlay">
          <div class="tuner-card">
            <div class="panel-head">
              <h2>{t("tuner.title")}</h2>
              <button class="ghost-button" type="button" onClick={() => void invokeNative<boolean>("toggleTuner", false)}>
                Close
              </button>
            </div>
            <div class="tuner-note">{state.tuner.note}</div>
            <div class="tuner-frequency">{state.tuner.frequencyHz ? `${state.tuner.frequencyHz.toFixed(2)} Hz` : "--"}</div>
            <div class="tuner-gauge">
              <div class="tuner-gauge__center" />
              <div class="tuner-gauge__needle" style={{ transform: `translateX(${toneOffset(state.tuner.cents)}%)` }} />
            </div>
            <div class="tuner-meta">
              <span>{state.tuner.cents} cents</span>
              <span>{t("tuner.signal")}: {(state.tuner.signalLevel * 100).toFixed(0)}%</span>
            </div>
          </div>
        </section>
      ) : null}

      {settingsOpen.value ? (
        <section class="overlay">
          <div class="settings-card">
            <div class="panel-head">
              <h2>{t("settings.title")}</h2>
              <button class="ghost-button" type="button" onClick={() => (settingsOpen.value = false)}>
                {t("settings.close")}
              </button>
            </div>
            <label class="settings-field">
              <span>{t("settings.inputDevice")}</span>
              <select
                value={state.audio.inputDeviceId}
                onChange={(event) => {
                  const value = (event.currentTarget as HTMLSelectElement).value;
                  mergeAudioState({ ...state.audio, inputDeviceId: value });
                }}
              >
                {state.audio.inputDevices.map((device) => (
                  <option key={device.id} value={device.id}>{device.name}</option>
                ))}
              </select>
            </label>
            <label class="settings-field">
              <span>{t("settings.outputDevice")}</span>
              <select
                value={state.audio.outputDeviceId}
                onChange={(event) => {
                  const value = (event.currentTarget as HTMLSelectElement).value;
                  mergeAudioState({ ...state.audio, outputDeviceId: value });
                }}
              >
                {state.audio.outputDevices.map((device) => (
                  <option key={device.id} value={device.id}>{device.name}</option>
                ))}
              </select>
            </label>
            <label class="settings-field">
              <span>{t("settings.sampleRate")}</span>
              <select
                value={String(state.audio.sampleRate)}
                onChange={(event) => {
                  mergeAudioState({ ...state.audio, sampleRate: Number((event.currentTarget as HTMLSelectElement).value) });
                }}
              >
                {state.audio.sampleRateOptions.map((value) => (
                  <option key={value} value={value}>{value} Hz</option>
                ))}
              </select>
            </label>
            <label class="settings-field">
              <span>{t("settings.bufferSize")}</span>
              <select
                value={String(state.audio.bufferSize)}
                onChange={(event) => {
                  mergeAudioState({ ...state.audio, bufferSize: Number((event.currentTarget as HTMLSelectElement).value) });
                }}
              >
                {state.audio.bufferSizeOptions.map((value) => (
                  <option key={value} value={value}>{value}</option>
                ))}
              </select>
            </label>
            <button class="toolbar-button toolbar-button--accent" type="button" onClick={applyAudioSettings}>
              {t("settings.apply")}
            </button>
          </div>
        </section>
      ) : null}
    </main>
  );
}

