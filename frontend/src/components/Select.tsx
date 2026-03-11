import { ComponentChildren } from 'preact';
import './Select.css';

interface SelectOption {
  value: string;
  label: string;
  disabled?: boolean;
}

interface SelectProps {
  value: string;
  onChange: (value: string) => void;
  options: SelectOption[];
  label?: string;
  disabled?: boolean;
  placeholder?: string;
  ariaLabel?: string;
}

export function Select({
  value,
  onChange,
  options,
  label,
  disabled = false,
  placeholder,
  ariaLabel,
}: SelectProps) {
  const handleChange = (e: Event) => {
    const target = e.target as HTMLSelectElement;
    onChange(target.value);
  };

  const classNames = ['select', disabled && 'select--disabled']
    .filter(Boolean)
    .join(' ');

  return (
    <div class={classNames}>
      {label && <label class="select__label">{label}</label>}
      <div class="select__wrapper">
        <select
          class="select__input"
          value={value}
          onChange={handleChange}
          disabled={disabled}
          aria-label={ariaLabel || label}
        >
          {placeholder && (
            <option value="" disabled>
              {placeholder}
            </option>
          )}
          {options.map((option) => (
            <option
              key={option.value}
              value={option.value}
              disabled={option.disabled}
            >
              {option.label}
            </option>
          ))}
        </select>
      </div>
    </div>
  );
}