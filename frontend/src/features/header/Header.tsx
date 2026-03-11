import { useTranslation } from '../../i18n';
import { language, theme } from '../../state';
import { bridge } from '../../bridge';
import { Button } from '../../components';
import type { Language } from '../../types';
import './Header.css';

const languageOptions = [
  { value: 'en', label: 'EN' },
  { value: 'ko', label: '한' },
  { value: 'ja', label: '日' },
  { value: 'zh', label: '中' },
];

interface HeaderProps {
  onOpenSettings: () => void;
  onOpenTuner: () => void;
}

export function Header({ onOpenSettings, onOpenTuner }: HeaderProps) {
  const { t } = useTranslation();

  const handleLanguageChange = (lang: Language) => {
    language.value = lang;
    bridge.emit('setLanguage', { languageCode: lang });
  };

  const handleThemeToggle = () => {
    const newTheme: 'dark' | 'light' = theme.value === 'dark' ? 'light' : 'dark';
    theme.value = newTheme;
    bridge.emit('setTheme', { themeName: newTheme });
  };

  return (
    <header class="header">
      <div class="header__left">
        <h1 class="header__title">{t('app.title')}</h1>
        <span class="header__version">Amp & FX Host</span>
      </div>

      <div class="header__right">
        <div class="header__lang-group">
          {languageOptions.map((opt) => (
            <button
              key={opt.value}
              class={`header__lang-btn ${language.value === opt.value ? 'header__lang-btn--active' : ''}`}
              onClick={() => handleLanguageChange(opt.value as Language)}
              aria-label={`Switch to ${opt.label}`}
              aria-pressed={language.value === opt.value}
            >
              {opt.label}
            </button>
          ))}
        </div>

        <button
          class="header__icon-btn"
          onClick={handleThemeToggle}
          aria-label={t('header.theme')}
          title={theme.value === 'dark' ? t('header.lightTheme') : t('header.darkTheme')}
        >
          {theme.value === 'dark' ? (
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <circle cx="12" cy="12" r="5" />
              <line x1="12" y1="1" x2="12" y2="3" />
              <line x1="12" y1="21" x2="12" y2="23" />
              <line x1="4.22" y1="4.22" x2="5.64" y2="5.64" />
              <line x1="18.36" y1="18.36" x2="19.78" y2="19.78" />
              <line x1="1" y1="12" x2="3" y2="12" />
              <line x1="21" y1="12" x2="23" y2="12" />
              <line x1="4.22" y1="19.78" x2="5.64" y2="18.36" />
              <line x1="18.36" y1="5.64" x2="19.78" y2="4.22" />
            </svg>
          ) : (
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" />
            </svg>
          )}
        </button>

        <Button
          variant="ghost"
          size="sm"
          onClick={onOpenTuner}
          ariaLabel={t('header.tuner')}
        >
          <span class="header__btn-content">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <path d="M12 2v20M17 7l-5-5-5 5M17 17l-5 5-5-5M2 12h20M7 7l-5 5 5 5M17 7l5 5-5 5" />
            </svg>
            <span>{t('header.tuner')}</span>
          </span>
        </Button>

        <Button
          variant="ghost"
          size="sm"
          onClick={onOpenSettings}
          ariaLabel={t('header.settings')}
        >
          <span class="header__btn-content">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <circle cx="12" cy="12" r="3" />
              <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z" />
            </svg>
            <span>{t('header.settings')}</span>
          </span>
        </Button>
      </div>
    </header>
  );
}