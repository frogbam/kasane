import { useSignal } from '@preact/signals';
import { Overlay, Button } from '../../components';
import { bridge } from '../../bridge';
import { useTranslation } from '../../i18n';
import { currentPresetId, currentPresetName, hasUnsavedPresetChanges, presets } from '../../state';
import type { PresetSummary } from '../../types';
import './PresetSidebar.css';

type NameDialogState = {
  type: 'saveAs' | 'rename' | 'duplicate';
  presetId?: string;
};

type ConfirmDialogState = {
  type: 'delete' | 'confirmLoad';
  presetId: string;
  presetName: string;
};

type DialogState = NameDialogState | ConfirmDialogState;

function getDisplayPresetName(fallbackName: string): string {
  return fallbackName.trim();
}

function formatRelativeTime(dateString: string): string {
  const date = new Date(dateString);
  const now = new Date();
  const diffMs = now.getTime() - date.getTime();
  const diffMinutes = Math.floor(diffMs / (1000 * 60));
  const diffHours = Math.floor(diffMs / (1000 * 60 * 60));
  const diffDays = Math.floor(diffMs / (1000 * 60 * 60 * 24));

  if (diffMinutes < 1) return 'Just now';
  if (diffMinutes < 60) return `${diffMinutes}m`;
  if (diffHours < 24) return `${diffHours}h`;
  if (diffDays < 7) return `${diffDays}d`;
  return date.toLocaleDateString();
}

export function PresetSidebar() {
  const { t } = useTranslation();
  const isCollapsed = useSignal(false);
  const dialogState = useSignal<DialogState | null>(null);
  const draftName = useSignal('');

  const openSaveAsDialog = () => {
    draftName.value = getDisplayPresetName(currentPresetName.value || '');
    dialogState.value = { type: 'saveAs' };
  };

  const openRenameDialog = (preset: PresetSummary) => {
    draftName.value = preset.name;
    dialogState.value = { type: 'rename', presetId: preset.id };
  };

  const openDuplicateDialog = (preset: PresetSummary) => {
    draftName.value = `${preset.name} Copy`;
    dialogState.value = { type: 'duplicate', presetId: preset.id };
  };

  const openDeleteDialog = (preset: PresetSummary) => {
    dialogState.value = { type: 'delete', presetId: preset.id, presetName: preset.name };
  };

  const openLoadConfirmDialog = (preset: PresetSummary) => {
    dialogState.value = { type: 'confirmLoad', presetId: preset.id, presetName: preset.name };
  };

  const closeDialog = () => {
    dialogState.value = null;
    draftName.value = '';
  };

  const handleLoadPreset = (preset: PresetSummary) => {
    if (currentPresetId.value === preset.id && !hasUnsavedPresetChanges.value) {
      return;
    }

    if (hasUnsavedPresetChanges.value) {
      openLoadConfirmDialog(preset);
      return;
    }

    bridge.emit('loadPreset', { presetId: preset.id });
  };

  const handleDialogConfirm = () => {
    const dialog = dialogState.value;
    if (dialog == null) {
      return;
    }

    switch (dialog.type) {
      case 'saveAs': {
        const name = draftName.value.trim();
        if (name.length === 0) return;
        bridge.emit('savePresetAs', { name });
        break;
      }
      case 'rename': {
        const name = draftName.value.trim();
        if (name.length === 0 || !dialog.presetId) return;
        bridge.emit('renamePreset', { presetId: dialog.presetId, name });
        break;
      }
      case 'duplicate': {
        const name = draftName.value.trim();
        if (name.length === 0 || !dialog.presetId) return;
        bridge.emit('duplicatePreset', { presetId: dialog.presetId, name });
        break;
      }
      case 'delete':
        bridge.emit('deletePreset', { presetId: dialog.presetId });
        break;
      case 'confirmLoad':
        bridge.emit('loadPreset', { presetId: dialog.presetId });
        break;
    }

    closeDialog();
  };

  const sortedPresets = [...presets.value].sort((left, right) => Date.parse(right.updatedAt) - Date.parse(left.updatedAt));
  const hasCurrentPreset = currentPresetId.value.trim().length > 0;
  const currentPresetLabel = currentPresetName.value.trim().length > 0
    ? currentPresetName.value
    : hasUnsavedPresetChanges.value
      ? t('presets.unsavedTone')
      : t('presets.noPresetLoaded');

  const dialog = dialogState.value;
  const dialogTitle = dialog?.type === 'rename'
    ? t('presets.rename')
    : dialog?.type === 'duplicate'
      ? t('presets.duplicate')
      : dialog?.type === 'delete'
        ? t('presets.delete')
        : dialog?.type === 'confirmLoad'
          ? t('presets.load')
          : t('presets.saveAs');

  return (
    <>
      <aside class={`preset-sidebar ${isCollapsed.value ? 'preset-sidebar--collapsed' : ''}`}>
        <div class="preset-sidebar__header">
          <button
            class="preset-sidebar__toggle"
            type="button"
            onClick={() => {
              isCollapsed.value = !isCollapsed.value;
            }}
            aria-label={isCollapsed.value ? t('presets.expand') : t('presets.collapse')}
            title={isCollapsed.value ? t('presets.expand') : t('presets.collapse')}
          >
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
              {isCollapsed.value ? (
                <path d="M9 18l6-6-6-6" />
              ) : (
                <path d="M15 18l-6-6 6-6" />
              )}
            </svg>
          </button>
          {!isCollapsed.value ? <h2 class="preset-sidebar__title">{t('presets.title')}</h2> : null}
        </div>

        <div class={`preset-sidebar__current ${isCollapsed.value ? 'preset-sidebar__current--collapsed' : ''}`}>
          {isCollapsed.value ? (
            <div
              class={`preset-sidebar__collapsed-indicator ${hasUnsavedPresetChanges.value ? 'preset-sidebar__collapsed-indicator--dirty' : ''}`}
              title={currentPresetLabel}
            />
          ) : (
            <>
              <div class="preset-sidebar__current-row">
                <div class="preset-sidebar__current-info">
                  <div class="preset-sidebar__current-name" title={currentPresetLabel}>
                    {currentPresetLabel}
                  </div>
                  <div class="preset-sidebar__current-status">
                    <span class={`preset-sidebar__status-dot ${hasUnsavedPresetChanges.value ? 'preset-sidebar__status-dot--dirty' : ''}`} />
                    <span class={`preset-sidebar__status-text ${hasUnsavedPresetChanges.value ? 'preset-sidebar__status-text--dirty' : ''}`}>
                      {hasUnsavedPresetChanges.value ? t('presets.unsaved') : t('presets.saved')}
                    </span>
                  </div>
                </div>
                <div class="preset-sidebar__current-actions">
                  <button
                    type="button"
                    class="preset-sidebar__action-btn preset-sidebar__action-btn--save"
                    onClick={() => bridge.emit('saveCurrentPreset')}
                    disabled={!hasCurrentPreset || !hasUnsavedPresetChanges.value}
                    aria-label={t('common.save')}
                    title={t('common.save')}
                  >
                    <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                      <path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z" />
                      <polyline points="17 21 17 13 7 13 7 21" />
                      <polyline points="7 3 7 8 15 8" />
                    </svg>
                  </button>
                  <button
                    type="button"
                    class="preset-sidebar__action-btn"
                    onClick={openSaveAsDialog}
                    aria-label={t('presets.saveAs')}
                    title={t('presets.saveAs')}
                  >
                    <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                      <path d="M12 5v14M5 12h14" />
                    </svg>
                  </button>
                </div>
              </div>
            </>
          )}
        </div>

        {!isCollapsed.value ? (
          <>
            <div class="preset-sidebar__section-label">{t('presets.library')}</div>
            <div class="preset-sidebar__list">
              {sortedPresets.length === 0 ? (
                <div class="preset-sidebar__empty">
                  <div class="preset-sidebar__empty-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                      <path d="M9 3H5a2 2 0 0 0-2 2v4m6-6h10a2 2 0 0 1 2 2v4M9 3v18m0 0h10a2 2 0 0 0 2-2V9M9 21H5a2 2 0 0 1-2-2V9m0 0h18" />
                    </svg>
                  </div>
                  {t('presets.empty')}
                </div>
              ) : (
                sortedPresets.map((preset) => (
                  <div
                    key={preset.id}
                    class={`preset-sidebar__item ${currentPresetId.value === preset.id ? 'preset-sidebar__item--active' : ''}`}
                    onClick={() => handleLoadPreset(preset)}
                    role="button"
                    tabIndex={0}
                    onKeyDown={(e) => {
                      if (e.key === 'Enter' || e.key === ' ') {
                        e.preventDefault();
                        handleLoadPreset(preset);
                      }
                    }}
                  >
                    <div class="preset-sidebar__item-main">
                      <div class="preset-sidebar__item-name">{preset.name}</div>
                      <div class="preset-sidebar__item-meta">{formatRelativeTime(preset.updatedAt)}</div>
                    </div>
                    <div class="preset-sidebar__item-actions">
                      <button
                        type="button"
                        class="preset-sidebar__item-btn"
                        aria-label={t('presets.rename')}
                        title={t('presets.rename')}
                        onClick={(event) => {
                          event.stopPropagation();
                          openRenameDialog(preset);
                        }}
                      >
                        <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                          <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
                          <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
                        </svg>
                      </button>
                      <button
                        type="button"
                        class="preset-sidebar__item-btn"
                        aria-label={t('presets.duplicate')}
                        title={t('presets.duplicate')}
                        onClick={(event) => {
                          event.stopPropagation();
                          openDuplicateDialog(preset);
                        }}
                      >
                        <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                          <rect x="9" y="9" width="13" height="13" rx="2" />
                          <path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1" />
                        </svg>
                      </button>
                      <button
                        type="button"
                        class="preset-sidebar__item-btn preset-sidebar__item-btn--danger"
                        aria-label={t('presets.delete')}
                        title={t('presets.delete')}
                        onClick={(event) => {
                          event.stopPropagation();
                          openDeleteDialog(preset);
                        }}
                      >
                        <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                          <polyline points="3 6 5 6 21 6" />
                          <path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6" />
                          <path d="M10 11v6M14 11v6" />
                          <path d="M9 6V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v2" />
                        </svg>
                      </button>
                    </div>
                  </div>
                ))
              )}
            </div>
          </>
        ) : null}
      </aside>

      <Overlay
        isOpen={dialog != null}
        onClose={closeDialog}
        title={dialogTitle}
        size="sm"
      >
        {dialog?.type === 'saveAs' || dialog?.type === 'rename' || dialog?.type === 'duplicate' ? (
          <div class="preset-sidebar__dialog">
            <p class="preset-sidebar__dialog-text">
              {dialog.type === 'saveAs'
                ? t('presets.saveAsDescription')
                : dialog.type === 'rename'
                  ? t('presets.renameDescription')
                  : t('presets.duplicateDescription')}
            </p>
            <input
              class="preset-sidebar__dialog-input"
              type="text"
              value={draftName.value}
              onInput={(event) => {
                draftName.value = (event.currentTarget as HTMLInputElement).value;
              }}
              onKeyDown={(event) => {
                if (event.key === 'Enter' && draftName.value.trim().length > 0) {
                  handleDialogConfirm();
                }
              }}
              placeholder={t('presets.namePlaceholder')}
              autoFocus
            />
            <div class="preset-sidebar__dialog-actions">
              <Button variant="ghost" onClick={closeDialog}>{t('settings.cancel')}</Button>
              <Button variant="primary" onClick={handleDialogConfirm} disabled={draftName.value.trim().length === 0}>
                {dialog.type === 'rename' ? t('presets.rename') : dialog.type === 'duplicate' ? t('presets.duplicate') : t('common.save')}
              </Button>
            </div>
          </div>
        ) : dialog?.type === 'delete' ? (
          <div class="preset-sidebar__dialog">
            <p class="preset-sidebar__dialog-text">{t('presets.deleteConfirm').replace('{name}', dialog.presetName)}</p>
            <div class="preset-sidebar__dialog-actions">
              <Button variant="ghost" onClick={closeDialog}>{t('settings.cancel')}</Button>
              <Button variant="danger" onClick={handleDialogConfirm}>{t('presets.delete')}</Button>
            </div>
          </div>
        ) : dialog?.type === 'confirmLoad' ? (
          <div class="preset-sidebar__dialog">
            <p class="preset-sidebar__dialog-text">{t('presets.loadConfirm').replace('{name}', dialog.presetName)}</p>
            <div class="preset-sidebar__dialog-actions">
              <Button variant="ghost" onClick={closeDialog}>{t('settings.cancel')}</Button>
              <Button variant="primary" onClick={handleDialogConfirm}>{t('presets.load')}</Button>
            </div>
          </div>
        ) : null}
      </Overlay>
    </>
  );
}