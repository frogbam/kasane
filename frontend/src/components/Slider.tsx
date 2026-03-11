import { useCallback } from 'preact/hooks';
import './Slider.css';

interface SliderProps {
  value: number;
  onChange: (value: number) => void;
  min?: number;
  max?: number;
  step?: number;
  label?: string;
  unit?: string;
  disabled?: boolean;
  showValue?: boolean;
  vertical?: boolean;
  ariaLabel?: string;
  resetValue?: number;
}

export function Slider({
  value,
  onChange,
  min = -60,
  max = 12,
  step = 0.1,
  label,
  unit = '',
  disabled = false,
  showValue = true,
  vertical = false,
  ariaLabel,
  resetValue = 0,
}: SliderProps) {
  const handleChange = useCallback(
    (e: Event) => {
      const target = e.target as HTMLInputElement;
      onChange(parseFloat(target.value));
    },
    [onChange]
  );

  const handleDoubleClick = useCallback(() => {
    onChange(resetValue);
  }, [onChange, resetValue]);

  const formattedValue = value >= 0 ? `+${value.toFixed(1)}` : value.toFixed(1);
  const percentage = ((value - min) / (max - min)) * 100;

  const classNames = [
    'slider',
    vertical && 'slider--vertical',
    disabled && 'slider--disabled',
  ]
    .filter(Boolean)
    .join(' ');

  return (
    <div class={classNames}>
      {label && (
        <label class="slider__label">
          <span class="slider__label-text">{label}</span>
          {showValue && (
            <span class="slider__value">
              {formattedValue}
              {unit}
            </span>
          )}
        </label>
      )}
      <div class="slider__track-wrapper">
        <div class="slider__track">
          <div class="slider__fill" style={{ width: `${percentage}%` }} />
        </div>
        <input
          type="range"
          class="slider__input"
          min={min}
          max={max}
          step={step}
          value={value}
          onChange={handleChange}
          onInput={handleChange}
          onDblClick={handleDoubleClick}
          disabled={disabled}
          aria-label={ariaLabel || label}
        />
      </div>
    </div>
  );
}