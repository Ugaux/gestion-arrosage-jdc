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
