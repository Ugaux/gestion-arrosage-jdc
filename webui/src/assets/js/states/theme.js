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

export default {
  dark: false,
  _timeout: null,

  init() {
    const saved = localStorage.getItem("theme");
    const mediaQuery = window.matchMedia("(prefers-color-scheme: dark)");
    this.dark = saved ? saved === "dark" : mediaQuery.matches;

    document.documentElement.classList.toggle("dark-theme", this.dark);

    // Keep in sync with OS preference when user has no saved choice
    mediaQuery.addEventListener("change", (e) => {
      if (localStorage.getItem("theme") === null) {
        this.dark = e.matches;
        document.documentElement.classList.toggle("dark-theme", this.dark);
      }
    });
  },

  toggle() {
    this.dark = !this.dark;
    this.apply();
  },

  apply() {
    clearTimeout(this._timeout);

    console.log("Setting theme to", this.dark ? "Dark" : "Light", "mode");
    document.documentElement.classList.add("theme-switching");
    document.body.classList.add("no-select");
    document.documentElement.classList.toggle("dark-theme", this.dark);
    localStorage.setItem("theme", this.dark ? "dark" : "light");

    // Remove after transition completes

    this._timeout = setTimeout(() => {
      document.documentElement.classList.remove("theme-switching");
      document.body.classList.remove("no-select");
    }, parseTransitionDuration()); // match your --theme-transition duration
  },
};
