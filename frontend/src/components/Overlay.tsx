import { ComponentChildren } from 'preact';
import { useEffect, useCallback } from 'preact/hooks';
import './Overlay.css';

interface OverlayProps {
  children: ComponentChildren;
  isOpen: boolean;
  onClose: () => void;
  title?: string;
  size?: 'sm' | 'md' | 'lg';
  closeOnBackdrop?: boolean;
  closeOnEscape?: boolean;
}

export function Overlay({
  children,
  isOpen,
  onClose,
  title,
  size = 'md',
  closeOnBackdrop = true,
  closeOnEscape = true,
}: OverlayProps) {
  const handleKeyDown = useCallback(
    (e: KeyboardEvent) => {
      if (e.key === 'Escape' && closeOnEscape) {
        onClose();
      }
    },
    [onClose, closeOnEscape]
  );

  useEffect(() => {
    if (isOpen) {
      document.addEventListener('keydown', handleKeyDown);
      document.body.style.overflow = 'hidden';
    }
    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      if (isOpen) {
        document.body.style.overflow = '';
      }
    };
  }, [isOpen, handleKeyDown]);

  const handleBackdropClick = useCallback(() => {
    if (closeOnBackdrop) {
      onClose();
    }
  }, [onClose, closeOnBackdrop]);

  if (!isOpen) return null;

  const contentClassNames = [
    'overlay__content',
    `overlay__content--${size}`,
  ].join(' ');

  return (
    <div class="overlay" role="dialog" aria-modal="true">
      <div class="overlay__backdrop" onClick={handleBackdropClick} aria-hidden="true" />
      <div class={contentClassNames}>
        <div class="overlay__header">
          {title && <h2 class="overlay__title">{title}</h2>}
          <button
            class="overlay__close"
            onClick={onClose}
            aria-label="Close"
            type="button"
          >
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <line x1="18" y1="6" x2="6" y2="18" />
              <line x1="6" y1="6" x2="18" y2="18" />
            </svg>
          </button>
        </div>
        <div class="overlay__body">{children}</div>
      </div>
    </div>
  );
}