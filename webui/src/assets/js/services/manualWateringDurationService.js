import {
  readJsonLocalStorage,
  subscribeToLocalStorageJson,
} from "../core/storage.js";

const KEY = "manualWateringDurations";
const PRINT_DEBUG = false;

export default {
  get(zoneId, defaultValue) {
    return readJsonLocalStorage(KEY)[zoneId] ?? defaultValue;
  },

  set(zoneId, duration) {
    const durations = readJsonLocalStorage(KEY);
    durations[zoneId] = duration;
    localStorage.setItem(KEY, JSON.stringify(durations));
  },

  subscribe(zoneId, callback) {
    return subscribeToLocalStorageJson(KEY, (durations) => {
      const value = durations?.[zoneId];
      if (value !== undefined) callback(value);
    });
  },

  syncWithZones(zones) {
    const activeIds = new Set(
      zones.flatMap((z) => z.ways.map((w) => String(w.id))),
    );
    if (PRINT_DEBUG) console.log("active durations:", activeIds);

    const stored = readJsonLocalStorage(KEY);
    if (PRINT_DEBUG) console.log("stored durations:", stored);

    // reconciliate stored entries with active zones
    const cleaned = Object.fromEntries(
      Object.entries(stored).filter(([id]) => activeIds.has(id)),
    );
    if (PRINT_DEBUG) console.log("cleaned durations:", cleaned);

    if (Object.keys(cleaned).length > 0)
      localStorage.setItem(KEY, JSON.stringify(cleaned));
    else localStorage.removeItem(KEY);
  },
};
