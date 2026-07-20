import { AppCfg } from "./appCfg";

const FALLBACK_DURATION_MS = 5000;

// in ms
export function parseCssDurationVar(varString) {
  const rawDuration = getComputedStyle(document.documentElement)
    .getPropertyValue(varString)
    .trim();

  if (!rawDuration) return FALLBACK_DURATION_MS;

  const value = parseFloat(rawDuration);
  return isNaN(value)
    ? FALLBACK_DURATION_MS
    : rawDuration.endsWith("ms")
      ? value
      : value * 1000;
}

export function logStatus(prefix, status, level = "info") {
  if (level === "info") {
    if (!AppCfg.verboseWebsocket) return;
    console.log(prefix, status);
  } else if (level === "warn") console.warn(prefix, status);
  else if (level === "error") console.error(prefix, status);
  else console.warn(prefix, "unknown log level:", level);
}

// For id -> crypto.randomUUID() only available in https or localhost http
export function generateUniqueID() {
  return `${Date.now()}-${Math.random().toString(16).slice(2)}`;
}
