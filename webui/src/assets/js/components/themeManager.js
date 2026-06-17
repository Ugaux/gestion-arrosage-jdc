import { subscribeToLocalStorageString } from "../core/storage.js";
import { parseCssDurationVar } from "../core/utilities.js";

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
      }, parseCssDurationVar("--theme-transition-duration"));
    },

    destroy() {
      this.unsubStorage?.();
      window
        .matchMedia("(prefers-color-scheme: dark)")
        .removeEventListener("change", this.syncToOsPreference);
    },
  }));
};
