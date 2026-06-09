import { syncJsonLocalStorage } from "../core/syncLocalStorage.js";

export default (wayID) => ({
  // temporary typing state
  min: null,
  max: null,
  step: null,
  duration: null,

  init() {
    const { min, max, step, defaultDuration } =
      Alpine.store("esp32cfg").manualDuration;
    this.min = min;
    this.max = max;
    this.step = step;
    this.duration = this.getDurations()[wayID] ?? defaultDuration;

    syncJsonLocalStorage("wateringDurations", (durations) => {
      const newValue = durations?.[wayID];
      if (newValue) this.setDuration(newValue, false);
    });
  },

  getDurations() {
    return JSON.parse(localStorage.getItem("wateringDurations") || "{}");
  },

  setDuration(value, save = true) {
    const v = Number(value);
    const clamped = isNaN(v)
      ? this.min
      : Math.max(this.min, Math.min(this.max, v)); // clamped value
    //console.log("value =", v, "and clamped =", clamped);
    this.duration = Math.round(clamped);

    if (save) {
      const storedDurations = this.getDurations();
      storedDurations[wayID] = this.duration;
      localStorage.setItem(
        "wateringDurations",
        JSON.stringify(storedDurations),
      );
    }
  },

  increase() {
    this.setDuration(this.duration + this.step);
  },

  decrease() {
    this.setDuration(this.duration - this.step);
  },
});
