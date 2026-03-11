import { useSignal } from '@preact/signals';
import { useTranslation } from '../../i18n';
import { availablePlugins, isScanningPlugins } from '../../state';
import { bridge } from '../../bridge';
import { Overlay, Button } from '../../components';
import './PluginPickerOverlay.css';

interface PluginPickerOverlayProps {
  isOpen: boolean;
  onClose: () => void;
  onSelect: (pluginId: string) => void;
}

export function PluginPickerOverlay({ isOpen, onClose, onSelect }: PluginPickerOverlayProps) {
  const { t } = useTranslation();
  const searchQuery = useSignal('');

  const plugins = availablePlugins.value;
  const filteredPlugins = plugins.filter(p => {
    const query = searchQuery.value.toLowerCase();
    return (
      p.name.toLowerCase().includes(query) ||
      p.manufacturer.toLowerCase().includes(query) ||
      p.category.toLowerCase().includes(query)
    );
  });

  const handleSelect = (pluginId: string) => {
    onSelect(pluginId);
    searchQuery.value = '';
    onClose();
  };

  const handleClose = () => {
    searchQuery.value = '';
    onClose();
  };

  const handleScanPlugins = () => {
    bridge.emit('scanPlugins');
  };

  return (
    <Overlay isOpen={isOpen} onClose={handleClose} title={t('plugins.availablePlugins')} size="md">
      <div class="plugin-picker-overlay">
        <div class="plugin-picker-overlay__header">
          <div class="plugin-picker-overlay__search">
            <input
              type="text"
              class="plugin-picker-overlay__search-input"
              placeholder="Search plugins..."
              value={searchQuery.value}
              onInput={(e) => { searchQuery.value = (e.target as HTMLInputElement).value; }}
            />
            <svg class="plugin-picker-overlay__search-icon" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <circle cx="11" cy="11" r="8" />
              <path d="M21 21l-4.35-4.35" />
            </svg>
          </div>
          <Button
            variant="secondary"
            size="sm"
            onClick={handleScanPlugins}
            loading={isScanningPlugins.value}
          >
            {isScanningPlugins.value ? t('header.scanning') : t('header.scanPlugins')}
          </Button>
        </div>

        <div class="plugin-picker-overlay__list">
          {filteredPlugins.length === 0 ? (
            <div class="plugin-picker-overlay__empty">
              {plugins.length === 0 ? t('plugins.noPlugins') : 'No matching plugins'}
            </div>
          ) : (
            filteredPlugins.map((plugin) => (
              <div
                key={plugin.id}
                class={`plugin-picker-overlay__item ${!plugin.isEnabled ? 'plugin-picker-overlay__item--disabled' : ''}`}
                onClick={() => plugin.isEnabled && handleSelect(plugin.id)}
              >
                <div class="plugin-picker-overlay__item-info">
                  <span class="plugin-picker-overlay__item-name">{plugin.name}</span>
                  <span class="plugin-picker-overlay__item-meta">
                    {plugin.manufacturer} • {plugin.category}
                  </span>
                </div>
                {plugin.isEnabled && (
                  <div class="plugin-picker-overlay__item-add">+</div>
                )}
              </div>
            ))
          )}
        </div>
      </div>
    </Overlay>
  );
}