import { useTranslation } from '../../i18n';
import { chain } from '../../state';
import { bridge } from '../../bridge';
import { Button } from '../../components';
import { useSignal } from '@preact/signals';
import './ChainView.css';

interface ChainViewProps {
  onAddPlugin: () => void;
}

export function ChainView({ onAddPlugin }: ChainViewProps) {
  const { t } = useTranslation();
  const draggedItem = useSignal<string | null>(null);
  const dragOverIndex = useSignal<number | null>(null);

  const handleToggleBypass = (pluginId: string) => {
    bridge.emit('togglePlugin', { chainPluginId: pluginId });
  };

  const handleRemove = (pluginId: string) => {
    bridge.emit('removePlugin', { chainPluginId: pluginId });
  };

  const handleOpenEditor = (pluginId: string) => {
    bridge.emit('openPluginEditor', { chainPluginId: pluginId });
  };

  const handleDragStart = (pluginId: string) => {
    draggedItem.value = pluginId;
  };

  const handleDragEnd = () => {
    draggedItem.value = null;
    dragOverIndex.value = null;
  };

  const handleDragOver = (e: DragEvent, index: number) => {
    e.preventDefault();
    e.dataTransfer!.dropEffect = 'move';
    dragOverIndex.value = index;
  };

  const handleDrop = (e: DragEvent, newIndex: number) => {
    e.preventDefault();
    const pluginId = draggedItem.value;
    if (!pluginId) return;

    const currentIndex = chain.value.findIndex(s => s.pluginId === pluginId);
    if (currentIndex === -1) return;

    if (currentIndex !== newIndex) {
      bridge.emit('reorderPlugins', { chainPluginId: pluginId, newIndex });
    }

    draggedItem.value = null;
    dragOverIndex.value = null;
  };

  const chainSlots = chain.value;

  if (chainSlots.length === 0) {
    return (
      <div class="chain-view">
        <div class="chain-view__empty">
          <div class="chain-view__empty-icon">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
              <line x1="4" y1="21" x2="4" y2="14" />
              <line x1="4" y1="10" x2="4" y2="3" />
              <line x1="12" y1="21" x2="12" y2="12" />
              <line x1="12" y1="8" x2="12" y2="3" />
              <line x1="20" y1="21" x2="20" y2="16" />
              <line x1="20" y1="12" x2="20" y2="3" />
              <circle cx="4" cy="12" r="2" />
              <circle cx="12" cy="10" r="2" />
              <circle cx="20" cy="14" r="2" />
            </svg>
          </div>
          <div class="chain-view__empty-text">
            {t('plugins.emptyChain')}
          </div>
          <Button variant="primary" onClick={onAddPlugin}>
            <span class="chain-view__add-first">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <line x1="12" y1="5" x2="12" y2="19" />
                <line x1="5" y1="12" x2="19" y2="12" />
              </svg>
              {t('plugins.add')}
            </span>
          </Button>
        </div>
      </div>
    );
  }

  return (
    <div class="chain-view">
      <div class="chain-view__content">
        {chainSlots.map((slot, index) => (
          <div
            key={slot.pluginId}
            class={`chain-slot ${slot.bypassed ? 'chain-slot--bypassed' : ''} ${draggedItem.value === slot.pluginId ? 'chain-slot--dragging' : ''} ${dragOverIndex.value === index ? 'chain-slot--drop-target' : ''}`}
            style={{ animationDelay: `${index * 50}ms` }}
            draggable
            onDragStart={() => handleDragStart(slot.pluginId)}
            onDragEnd={handleDragEnd}
            onDragOver={(e) => handleDragOver(e, index)}
            onDrop={(e) => handleDrop(e, index)}
          >
            <div class="chain-slot__header">
              <div class="chain-slot__number">{index + 1}</div>
              <div class="chain-slot__info">
                <div class="chain-slot__name">{slot.name}</div>
                <div class="chain-slot__meta">
                  {slot.manufacturer}
                </div>
              </div>
              <div class={`chain-slot__status ${slot.bypassed ? 'chain-slot__status--bypassed' : 'chain-slot__status--active'}`}>
                {slot.bypassed ? 'BYPASSED' : 'ACTIVE'}
              </div>
              <div class="chain-slot__drag-handle" aria-label="Drag to reorder">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                  <circle cx="9" cy="6" r="1.5" fill="currentColor" />
                  <circle cx="15" cy="6" r="1.5" fill="currentColor" />
                  <circle cx="9" cy="12" r="1.5" fill="currentColor" />
                  <circle cx="15" cy="12" r="1.5" fill="currentColor" />
                  <circle cx="9" cy="18" r="1.5" fill="currentColor" />
                  <circle cx="15" cy="18" r="1.5" fill="currentColor" />
                </svg>
              </div>
            </div>
            
            <div class="chain-slot__actions">
              <button
                class={`chain-slot__btn ${slot.bypassed ? 'chain-slot__btn--bypassed' : ''}`}
                onClick={() => handleToggleBypass(slot.pluginId)}
                title={slot.bypassed ? t('plugins.enable') : t('plugins.bypass')}
                aria-label={slot.bypassed ? t('plugins.enable') : t('plugins.bypass')}
              >
                {slot.bypassed ? (
                  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="10" />
                  </svg>
                ) : (
                  <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
                    <circle cx="12" cy="12" r="6" />
                  </svg>
                )}
              </button>
              <button
                class="chain-slot__btn chain-slot__btn--edit"
                onClick={() => handleOpenEditor(slot.pluginId)}
                title={t('plugins.edit')}
                aria-label={t('plugins.edit')}
              >
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                  <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
                  <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
                </svg>
              </button>
              <button
                class="chain-slot__btn chain-slot__btn--danger"
                onClick={() => handleRemove(slot.pluginId)}
                title={t('plugins.remove')}
                aria-label={t('plugins.remove')}
              >
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                  <line x1="18" y1="6" x2="6" y2="18" />
                  <line x1="6" y1="6" x2="18" y2="18" />
                </svg>
              </button>
            </div>
          </div>
        ))}
        
        <button
          class="chain-add-btn chain-add-btn--end"
          onClick={onAddPlugin}
          title={t('plugins.add')}
          aria-label={t('plugins.add')}
        >
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <line x1="12" y1="5" x2="12" y2="19" />
            <line x1="5" y1="12" x2="19" y2="12" />
          </svg>
        </button>
      </div>
    </div>
  );
}