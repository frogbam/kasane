import { useTranslation } from '../../i18n';
import { tuner } from '../../state';
import { bridge } from '../../bridge';
import { Overlay } from '../../components';
import './TunerOverlay.css';

interface TunerOverlayProps {
  isOpen: boolean;
  onClose: () => void;
}

export function TunerOverlay({ isOpen, onClose }: TunerOverlayProps) {
  const { t } = useTranslation();

  const tunerState = tuner.value;
  const note = tunerState.note;
  const frequency = tunerState.frequencyHz;
  const cents = tunerState.cents;
  const signalLevel = tunerState.signalLevel;

  const handleClose = () => {
    bridge.emit('toggleTuner', { isOpen: false });
    onClose();
  };

  const getCentsColor = (c: number): string => {
    const absC = Math.abs(c);
    if (absC <= 5) return 'var(--success)';
    if (absC <= 15) return 'var(--warning)';
    return 'var(--error)';
  };

  const getCentsPosition = (c: number): number => {
    const clamped = Math.max(-50, Math.min(50, c));
    return 50 + (clamped / 50) * 40;
  };

  const getSignalWidth = (level: number): number => {
    return Math.max(0, Math.min(100, level * 100));
  };

  const isInTune = Math.abs(cents) <= 5;

  return (
    <Overlay isOpen={isOpen} onClose={handleClose} title={t('tuner.title')} size="sm">
      <div class="tuner">
        <div class="tuner__note-display">
          <span class="tuner__note">{note || '—'}</span>
          <span class="tuner__frequency">
            {frequency > 0 ? `${frequency.toFixed(2)} ${t('tuner.hz')}` : '—'}
          </span>
        </div>

        <div class="tuner__cents-container">
          <div class="tuner__cents-scale">
            <span class="tuner__cents-label tuner__cents-label--left">{t('tuner.flat')}</span>
            <span class="tuner__cents-label tuner__cents-label--center">{t('tuner.inTune')}</span>
            <span class="tuner__cents-label tuner__cents-label--right">{t('tuner.sharp')}</span>
          </div>
          <div class="tuner__cents-track">
            <div 
              class="tuner__cents-needle" 
              style={{ 
                left: `${getCentsPosition(cents)}%`,
                '--cents-color': getCentsColor(cents),
              }}
            />
            <div class="tuner__cents-center" />
          </div>
          <div class="tuner__cents-value" style={{ color: getCentsColor(cents) }}>
            {cents > 0 ? '+' : ''}{cents} {t('tuner.cents')}
          </div>
        </div>

        <div class="tuner__signal">
          <div class="tuner__signal-label">{t('tuner.signalLevel')}</div>
          <div class="tuner__signal-bar">
            <div 
              class="tuner__signal-fill" 
              style={{ width: `${getSignalWidth(signalLevel)}%` }}
            />
          </div>
        </div>

        <div class={`tuner__status ${isInTune ? 'tuner__status--in-tune' : ''}`}>
          {isInTune ? '●' : '○'}
        </div>
      </div>
    </Overlay>
  );
}