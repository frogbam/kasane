import { useTranslation } from '../../i18n';
import { audio, meters } from '../../state';
import { bridge } from '../../bridge';
import { Meter, Slider } from '../../components';
import { useSignal } from '@preact/signals';
import './AudioPanel.css';

export function AudioPanel() {
  const { t } = useTranslation();

  const handleInputGainChange = (value: number) => {
    bridge.emit('setInputGain', { gainDb: value });
  };

  const handleOutputGainChange = (value: number) => {
    bridge.emit('setOutputGain', { gainDb: value });
  };

  const inputLeftDb = meters.value.inputLeftDb;
  const inputRightDb = meters.value.inputRightDb;
  const outputLeftDb = meters.value.outputLeftDb;
  const outputRightDb = meters.value.outputRightDb;
  const inputGainDb = audio.value.inputGainDb;
  const outputGainDb = audio.value.outputGainDb;

  return (
    <div class="audio-panel panel">
      <div class="panel-header">{t('audio.inputDevice')}</div>
      <div class="audio-panel__device-name">{audio.value.inputDeviceName || '—'}</div>

      <div class="audio-panel__section">
        <div class="panel-header">{t('audio.inputMeter')}</div>
        <div class="audio-panel__meters">
          <Meter
            leftDb={inputLeftDb}
            rightDb={inputRightDb}
            channels="stereo"
          />
        </div>
      </div>

      <div class="audio-panel__section">
        <Slider
          value={inputGainDb}
          onChange={handleInputGainChange}
          min={-60}
          max={12}
          step={0.5}
          label={t('audio.inputGain')}
          unit={t('audio.decibel')}
        />
      </div>

      <div class="audio-panel__divider" />

      <div class="panel-header">{t('audio.outputDevice')}</div>
      <div class="audio-panel__device-name">{audio.value.outputDeviceName || '—'}</div>

      <div class="audio-panel__section">
        <div class="panel-header">{t('audio.outputMeter')}</div>
        <div class="audio-panel__meters">
          <Meter
            leftDb={outputLeftDb}
            rightDb={outputRightDb}
            channels="stereo"
          />
        </div>
      </div>

      <div class="audio-panel__section">
        <Slider
          value={outputGainDb}
          onChange={handleOutputGainChange}
          min={-60}
          max={12}
          step={0.5}
          label={t('audio.outputGain')}
          unit={t('audio.decibel')}
        />
      </div>
    </div>
  );
}