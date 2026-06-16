import { subscribeToLocalStorageString } from "../core/storage.js";

const FALLBACK_DURATION_MS = 5000;

function parseTransitionDuration() {
  const rawDuration = getComputedStyle(document.documentElement)
    .getPropertyValue("--theme-transition-duration")
    .trim();

  const duration = rawDuration.endsWith("ms")
    ? parseFloat(rawDuration)
    : parseFloat(rawDuration) * 1000;

  if (!rawDuration) return FALLBACK_DURATION_MS;

  const value = parseFloat(rawDuration);
  return isNaN(value)
    ? FALLBACK_DURATION_MS
    : rawDuration.endsWith("ms")
      ? value
      : value * 1000;
}

export default (Alpine) => {
  Alpine.data("themeManager", () => ({
    isDark: false,
    _timeout: null,

    unsubStorage: null,

    init() {
      const saved = localStorage.getItem("theme");
      const mediaQuery = window.matchMedia("(prefers-color-scheme: dark)");
      this.isDark = saved ? saved === "dark" : mediaQuery.matches;

      document.documentElement.classList.toggle("dark-theme", this.isDark);

      // Keep in sync with OS preference when user has no saved choice
      mediaQuery.addEventListener("change", this.syncToOsPreference);

      // Sync across tabs
      this.unsubStorage = subscribeToLocalStorageString("theme", (newValue) => {
        if (newValue) {
          this.isDark = newValue === "dark";
          this.apply(false); // apply without re-saving
        } else if (
          this.isDark !==
          window.matchMedia("(prefers-color-scheme: dark)").matches
        ) {
          this.isDark = window.matchMedia(
            "(prefers-color-scheme: dark)",
          ).matches;
          this.apply(false); // apply without re-saving
        }
      });
    },

    syncToOsPreference(event) {
      if (localStorage.getItem("theme") === null) {
        this.isDark = event.matches;
        document.documentElement.classList.toggle("dark-theme", this.isDark);
      }
    },

    toggle() {
      this.isDark = !this.isDark;
      this.apply(true);
    },

    apply(save) {
      clearTimeout(this._timeout);

      console.log("Setting theme to", this.isDark ? "Dark" : "Light", "mode");
      document.documentElement.classList.add("theme-switching");
      document.body.classList.add("no-select");
      document.documentElement.classList.toggle("dark-theme", this.isDark);
      if (save) localStorage.setItem("theme", this.isDark ? "dark" : "light");

      // Remove after transition completes
      this._timeout = setTimeout(() => {
        document.documentElement.classList.remove("theme-switching");
        document.body.classList.remove("no-select");
      }, parseTransitionDuration());
    },

    destroy() {
      this.unsubStorage?.();
      window
        .matchMedia("(prefers-color-scheme: dark)")
        .removeEventListener("change", this.syncToOsPreference);
    },
  }));
};
