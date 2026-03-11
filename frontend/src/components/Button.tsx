import { ComponentChildren } from 'preact';
import { useTranslation } from '../../i18n';
import './Button.css';

interface ButtonProps {
  children: ComponentChildren;
  onClick?: () => void;
  disabled?: boolean;
  variant?: 'primary' | 'secondary' | 'ghost' | 'danger';
  size?: 'sm' | 'md' | 'lg';
  fullWidth?: boolean;
  loading?: boolean;
  ariaLabel?: string;
  title?: string;
}

export function Button({
  children,
  onClick,
  disabled = false,
  variant = 'primary',
  size = 'md',
  fullWidth = false,
  loading = false,
  ariaLabel,
  title,
}: ButtonProps) {
  const classNames = [
    'btn',
    `btn--${variant}`,
    `btn--${size}`,
    fullWidth && 'btn--full-width',
    loading && 'btn--loading',
  ]
    .filter(Boolean)
    .join(' ');

  return (
    <button
      class={classNames}
      onClick={onClick}
      disabled={disabled || loading}
      aria-label={ariaLabel}
      title={title}
    >
      {loading ? (
        <span class="btn__spinner" aria-hidden="true" />
      ) : null}
      <span class={loading ? 'btn__text--hidden' : 'btn__text'}>{children}</span>
    </button>
  );
}