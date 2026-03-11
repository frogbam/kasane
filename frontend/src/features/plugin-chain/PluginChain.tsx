import { useTranslation } from '../../i18n';
import { availablePlugins, chain } from '../../state';
import { bridge } from '../../bridge';
import { Button } from '../../components';
import './PluginChain.css';

export function PluginChain() {
  const { t } = useTranslation();

  const handleAddPlugin = (pluginId: string) => {
    bridge.emit('addPlugin', { pluginDescriptorId: pluginId });
  };

  const handleRemovePlugin = (pluginId: string) => {
    bridge.emit('removePlugin', { chainPluginId: pluginId });
  };

  const handleToggleBypass = (pluginId: string) => {
    bridge.emit('togglePlugin', { chainPluginId: pluginId });
  };

  const handleMoveUp = (pluginId: string, currentIndex: number) => {
    if (currentIndex > 0) {
      bridge.emit('reorderPlugins', { chainPluginId: pluginId, newIndex: currentIndex - 1 });
    }
  };

  const handleMoveDown = (pluginId: string, currentIndex: number) => {
    if (currentIndex < chain.value.length - 1) {
      bridge.emit('reorderPlugins', { chainPluginId: pluginId, newIndex: currentIndex + 1 });
    }
  };

  const handleOpenEditor = (pluginId: string) => {
    bridge.emit('openPluginEditor', { chainPluginId: pluginId });
  };

  const pluginsList = availablePlugins.value;
  const chainSlots = chain.value;

  return (
    <div class="plugin-chain">
      <div class="panel plugin-chain__available">
        <div class="panel-header">{t('plugins.availablePlugins')}</div>
        <div class="plugin-chain__list">
          {pluginsList.length === 0 ? (
            <div class="plugin-chain__empty">{t('plugins.noPlugins')}</div>
          ) : (
            pluginsList.map((plugin) => (
              <div key={plugin.id} class="plugin-item">
                <div class="plugin-item__info">
                  <span class="plugin-item__name">{plugin.name}</span>
                  <span class="plugin-item__meta">
                    {plugin.manufacturer} • {plugin.category}
                  </span>
                </div>
                <Button
                  variant="primary"
                  size="sm"
                  onClick={() => handleAddPlugin(plugin.id)}
                  disabled={!plugin.isEnabled}
                  ariaLabel={`${t('plugins.add')} ${plugin.name}`}
                >
                  +
                </Button>
              </div>
            ))
          )}
        </div>
      </div>

      <div class="panel plugin-chain__chain">
        <div class="panel-header">{t('plugins.chain')}</div>
        <div class="plugin-chain__slots">
          {chainSlots.length === 0 ? (
            <div class="plugin-chain__empty">{t('plugins.emptyChain')}</div>
          ) : (
            chainSlots.map((slot, index) => (
              <div
                key={slot.pluginId}
                class={`chain-slot ${slot.bypassed ? 'chain-slot--bypassed' : ''}`}
              >
                <div class="chain-slot__header">
                  <div class="chain-slot__order">{index + 1}</div>
                  <div class="chain-slot__info">
                    <span class="chain-slot__name">{slot.name}</span>
                    <span class="chain-slot__category">{slot.category}</span>
                  </div>
                </div>
                <div class="chain-slot__actions">
                  <button
                    class="chain-slot__btn"
                    onClick={() => handleMoveUp(slot.pluginId, index)}
                    disabled={index === 0}
                    title={t('plugins.moveUp')}
                    aria-label={t('plugins.moveUp')}
                  >
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                      <path d="M18 15l-6-6-6 6" />
                    </svg>
                  </button>
                  <button
                    class="chain-slot__btn"
                    onClick={() => handleMoveDown(slot.pluginId, index)}
                    disabled={index === chainSlots.length - 1}
                    title={t('plugins.moveDown')}
                    aria-label={t('plugins.moveDown')}
                  >
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                      <path d="M6 9l6 6 6-6" />
                    </svg>
                  </button>
                  <button
                    class={`chain-slot__btn chain-slot__btn--toggle ${slot.bypassed ? '' : 'chain-slot__btn--active'}`}
                    onClick={() => handleToggleBypass(slot.pluginId)}
                    title={slot.bypassed ? t('plugins.enable') : t('plugins.bypass')}
                    aria-label={slot.bypassed ? t('plugins.enable') : t('plugins.bypass')}
                  >
                    {slot.bypassed ? '○' : '●'}
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
                    onClick={() => handleRemovePlugin(slot.pluginId)}
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
            ))
          )}
        </div>
      </div>
    </div>
  );
}