import { useTranslation } from '../../i18n';
import { audio } from '../../state';
import { bridge } from '../../bridge';
import { Overlay, Select, Button } from '../../components';
import { useSignal } from '@preact/signals';
import './SettingsOverlay.css';

interface SettingsOverlayProps {
  isOpen: boolean;
  onClose: () => void;
}

export function SettingsOverlay({ isOpen, onClose }: SettingsOverlayProps) {
  const { t } = useTranslation();
  
  const pendingInputDevice = useSignal<string>(audio.value.inputDeviceId);
  const pendingOutputDevice = useSignal<string>(audio.value.outputDeviceId);
  const pendingSampleRate = useSignal<number>(audio.value.sampleRate);
  const pendingBufferSize = useSignal<number>(audio.value.bufferSize);

  const inputDevices = audio.value.inputDevices;
  const outputDevices = audio.value.outputDevices;
  const sampleRateOptions = audio.value.sampleRateOptions;
  const bufferSizeOptions = audio.value.bufferSizeOptions;

  const handleApply = () => {
    bridge.emit('setAudioDeviceSetup', {
      inputDeviceId: pendingInputDevice.value,
      outputDeviceId: pendingOutputDevice.value,
      sampleRate: pendingSampleRate.value,
      bufferSize: pendingBufferSize.value,
    });
    onClose();
  };

  const handleCancel = () => {
    pendingInputDevice.value = audio.value.inputDeviceId;
    pendingOutputDevice.value = audio.value.outputDeviceId;
    pendingSampleRate.value = audio.value.sampleRate;
    pendingBufferSize.value = audio.value.bufferSize;
    onClose();
  };

  const inputDeviceOptions = inputDevices.map(d => ({ value: d.id, label: d.name }));
  const outputDeviceOptions = outputDevices.map(d => ({ value: d.id, label: d.name }));
  const sampleRateOpts = sampleRateOptions.map(sr => ({ value: String(sr), label: `${sr} ${t('settings.hz')}` }));
  const bufferSizeOpts = bufferSizeOptions.map(bs => ({ value: String(bs), label: `${bs} ${t('settings.samples')}` }));

  return (
    <Overlay isOpen={isOpen} onClose={onClose} title={t('settings.title')} size="md">
      <div class="settings-overlay">
        <div class="settings-overlay__section">
          <Select
            label={t('settings.inputDevice')}
            value={pendingInputDevice.value}
            onChange={(v) => { pendingInputDevice.value = v; }}
            options={inputDeviceOptions}
          />
        </div>

        <div class="settings-overlay__section">
          <Select
            label={t('settings.outputDevice')}
            value={pendingOutputDevice.value}
            onChange={(v) => { pendingOutputDevice.value = v; }}
            options={outputDeviceOptions}
          />
        </div>

        <div class="settings-overlay__row">
          <div class="settings-overlay__field">
            <Select
              label={t('settings.sampleRate')}
              value={String(pendingSampleRate.value)}
              onChange={(v) => { pendingSampleRate.value = parseInt(v, 10); }}
              options={sampleRateOpts}
            />
          </div>

          <div class="settings-overlay__field">
            <Select
              label={t('settings.bufferSize')}
              value={String(pendingBufferSize.value)}
              onChange={(v) => { pendingBufferSize.value = parseInt(v, 10); }}
              options={bufferSizeOpts}
            />
          </div>
        </div>

        <div class="settings-overlay__actions">
          <Button variant="secondary" onClick={handleCancel}>
            {t('settings.cancel')}
          </Button>
          <Button variant="primary" onClick={handleApply}>
            {t('settings.apply')}
          </Button>
        </div>
      </div>
    </Overlay>
  );
}