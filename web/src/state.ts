import { signal } from "@preact/signals";
import { defaultAppState } from "./types";

export const appState = signal(defaultAppState);
export const settingsOpen = signal(false);
export const selectedPluginId = signal("");

