import { useTranslation } from '../../i18n';
import { audio, meters } from '../../state';
import { bridge } from '../../bridge';
import { HorizontalMeter, Slider } from '../../components';
import './BottomBar.css';

interface BottomBarProps {
  onOpenSettings?: () => void;
}

export function BottomBar({ onOpenSettings }: BottomBarProps) {
  const { t } = useTranslation();

  return (
    <div class="bottom-bar">
      <div class="bottom-bar__section bottom-bar__section--input">
        <div class="bottom-bar__meter-container">
          <HorizontalMeter
            leftDb={meters.value.inputLeftDb}
            rightDb={meters.value.inputRightDb}
            label={t('audio.inputMeter')}
          />
        </div>
        <div class="bottom-bar__gain-container">
          <div class="bottom-bar__gain-label">
            {t('audio.inputGain')}: <span class="bottom-bar__gain-value">
              {audio.value.inputGainDb >= 0 ? '+' : ''}{audio.value.inputGainDb.toFixed(1)} dB
            </span>
          </div>
          <Slider
            value={audio.value.inputGainDb}
            onChange={(v) => bridge.emit('setInputGain', { gainDb: v })}
            min={-20}
            max={20}
            step={0.1}
            showValue={false}
            resetValue={0}
          />
        </div>
      </div>

      <div class="bottom-bar__section bottom-bar__section--output">
        <div class="bottom-bar__gain-container">
          <div class="bottom-bar__gain-label">
            {t('audio.outputGain')}: <span class="bottom-bar__gain-value">
              {audio.value.outputGainDb >= 0 ? '+' : ''}{audio.value.outputGainDb.toFixed(1)} dB
            </span>
          </div>
          <Slider
            value={audio.value.outputGainDb}
            onChange={(v) => bridge.emit('setOutputGain', { gainDb: v })}
            min={-20}
            max={20}
            step={0.1}
            showValue={false}
            resetValue={0}
          />
        </div>
        <div class="bottom-bar__meter-container">
          <HorizontalMeter
            leftDb={meters.value.outputLeftDb}
            rightDb={meters.value.outputRightDb}
            label={t('audio.outputMeter')}
          />
        </div>
      </div>
    </div>
  );
}