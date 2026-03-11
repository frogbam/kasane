import { createContext } from 'preact';
import { useContext } from 'preact/hooks';
import { signal, computed } from '@preact/signals';
import type { Language } from '../types';
import en from './locales/en.json';
import ko from './locales/ko.json';
import ja from './locales/ja.json';
import zh from './locales/zh.json';

const translations: Record<Language, Record<string, unknown>> = {
  en,
  ko,
  ja,
  zh,
};

const currentLanguage = signal<Language>('en');

type TranslationKey = string;

function getNestedValue(obj: Record<string, unknown>, path: string): string | undefined {
  const parts = path.split('.');
  let current: unknown = obj;
  
  for (const part of parts) {
    if (typeof current !== 'object' || current === null) return undefined;
    current = (current as Record<string, unknown>)[part];
  }
  
  return typeof current === 'string' ? current : undefined;
}

export function t(key: TranslationKey, lang?: Language): string {
  const language = lang ?? currentLanguage.value;
  const translation = translations[language];
  const value = getNestedValue(translation, key);
  
  if (value !== undefined) return value;
  
  if (language !== 'en') {
    const fallback = getNestedValue(translations.en, key);
    if (fallback !== undefined) return fallback;
  }
  
  return key;
}

export function useTranslation(): {
  t: (key: TranslationKey) => string;
  language: Language;
  setLanguage: (lang: Language) => void;
} {
  return {
    t: (key: TranslationKey) => t(key),
    language: currentLanguage.value,
    setLanguage: (lang: Language) => {
      currentLanguage.value = lang;
    },
  };
}

export function getCurrentLanguage(): Language {
  return currentLanguage.value;
}

export function setLanguage(lang: Language): void {
  currentLanguage.value = lang;
}

const translationsSignal = computed(() => translations[currentLanguage.value]);

export { translationsSignal };