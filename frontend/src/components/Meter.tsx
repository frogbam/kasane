import { useMemo } from 'preact/hooks';
import './Meter.css';

interface MeterProps {
  leftDb: number;
  rightDb: number;
  label?: string;
  channels?: 'stereo' | 'mono';
  peak?: number;
}

export function Meter({ leftDb, rightDb, label, channels = 'stereo', peak }: MeterProps) {
  const segments = useMemo(() => {
    const segmentCount = 24;
    const segmentValues: number[] = [];
    for (let i = 0; i < segmentCount; i++) {
      segmentValues.push(-60 + (i * 60) / segmentCount);
    }
    return segmentValues;
  }, []);

  const getLevelPercent = (db: number): number => {
    const normalizedDb = Math.max(-60, Math.min(12, db));
    return ((normalizedDb + 60) / 72) * 100;
  };

  const getSegmentColor = (threshold: number): string => {
    if (threshold >= 0) return 'var(--meter-red)';
    if (threshold >= -6) return 'var(--meter-yellow)';
    return 'var(--meter-green)';
  };

  const leftPercent = getLevelPercent(leftDb);
  const rightPercent = getLevelPercent(rightDb);

  return (
    <div class="meter">
      {label && <div class="meter__label">{label}</div>}
      <div class="meter__body">
        <div class="meter__channel">
          <div class="meter__scale">
            {segments.map((threshold, index) => (
              <div
                key={index}
                class={`meter__segment ${leftPercent > (index / segments.length) * 100 ? 'meter__segment--active' : ''}`}
                style={{
                  '--segment-color': getSegmentColor(threshold),
                  animationDelay: `${index * 15}ms`,
                }}
              />
            ))}
          </div>
          {channels === 'stereo' && (
            <div class="meter__scale">
              {segments.map((threshold, index) => (
                <div
                  key={index}
                  class={`meter__segment ${rightPercent > (index / segments.length) * 100 ? 'meter__segment--active' : ''}`}
                  style={{
                    '--segment-color': getSegmentColor(threshold),
                    animationDelay: `${index * 15}ms`,
                  }}
                />
              ))}
            </div>
          )}
        </div>
        <div class="meter__readings">
          <span class="meter__value">{leftDb >= 0 ? '+' : ''}{leftDb.toFixed(1)}</span>
          {channels === 'stereo' && (
            <span class="meter__value">{rightDb >= 0 ? '+' : ''}{rightDb.toFixed(1)}</span>
          )}
          <span class="meter__unit">dB</span>
        </div>
      </div>
    </div>
  );
}