import './HorizontalMeter.css';

interface HorizontalMeterProps {
  leftDb: number;
  rightDb: number;
  label?: string;
}

export function HorizontalMeter({ leftDb, rightDb, label }: HorizontalMeterProps) {
  const segments = 28;
  
  const getLevelPercent = (db: number): number => {
    const normalizedDb = Math.max(-60, Math.min(12, db));
    return ((normalizedDb + 60) / 72) * 100;
  };

  const leftPercent = getLevelPercent(leftDb);
  const rightPercent = getLevelPercent(rightDb);

  const renderSegments = (percent: number) => {
    return Array.from({ length: segments }, (_, i) => {
      const threshold = -60 + (i * 60) / segments;
      const isActive = percent > (i / segments) * 100;
      
      let colorClass = 'horizontal-meter__segment--green';
      if (threshold >= 0) colorClass = 'horizontal-meter__segment--red';
      else if (threshold >= -6) colorClass = 'horizontal-meter__segment--yellow';

      return (
        <div
          key={i}
          class={`horizontal-meter__segment ${isActive ? 'horizontal-meter__segment--active' : ''} ${isActive ? colorClass : ''}`}
        />
      );
    });
  };

  const formatDb = (db: number) => {
    return db >= 0 ? `+${db.toFixed(1)}` : db.toFixed(1);
  };

  return (
    <div class="horizontal-meter">
      <div class="horizontal-meter__header">
        <span class="horizontal-meter__label">{label || 'Meter'}</span>
      </div>
      <div class="horizontal-meter__rows">
        <div class="horizontal-meter__row">
          {renderSegments(leftPercent)}
        </div>
        <div class="horizontal-meter__row">
          {renderSegments(rightPercent)}
        </div>
      </div>
    </div>
  );
}