import manualWateringDurationService from "../services/manualWateringDurationService.js";

//
//  deviceCfg store          localStorage
//       │                        │
//       │ (constraints)          │ (user values)
//       ▼                        ▼
//    $watch                storage event
//       │                        │
//       └──────────┬─────────────┘
//                  ▼
//      syncFromLocalStorage()   setDuration(v, save=false)
//                  │                    │
//                  └────────┬───────────┘
//                           ▼
//                      setDuration()
//                           │
//                ┌──────────┴──────────┐
//                │                     │
//           this.duration        localStorage.set
//           (reactive UI)      (only on user action)
//

const PRINT_DEBUG = false;

export default (Alpine) => {
  Alpine.data("manualWateringDurationInput", (wayID) => ({
    min: null,
    max: null,
    step: null,
    defaultDuration: null,

    duration: null,

    unsubStorage: null,

    init() {
      // Initial read
      this.syncFromLocalStorage();

      // Sync axis 1: store → component
      this.$watch(
        () =>
          `${Alpine.store("deviceCfg").manualWateringDuration.min}|` +
          `${Alpine.store("deviceCfg").manualWateringDuration.max}|` +
          `${Alpine.store("deviceCfg").manualWateringDuration.step}` +
          `${Alpine.store("deviceCfg").manualWateringDuration.defaultDuration}`,
        (v) => {
          //console.log("watch", v);
          this.syncFromLocalStorage();
        },
      );

      // Sync axis 2: other tabs → component
      this.unsubStorage = manualWateringDurationService.subscribe(
        wayID,
        (value) => {
          if (this.duration !== value && this.ready()) {
            console.log("storage subscription triggered →", value);
            this.setDuration(value); // don't re-save, we received it
          }
        },
      );
    },

    ready() {
      return this.min !== null && this.max !== null && this.max > this.min;
    },

    syncFromLocalStorage() {
      const cfg = Alpine.store("deviceCfg").manualWateringDuration;
      this.min = cfg.min;
      this.max = cfg.max;
      this.step = cfg.step;
      this.defaultDuration = cfg.defaultDuration;

      // Re-clamp whatever duration is already stored (or fall back to default).
      const value = manualWateringDurationService.get(
        wayID,
        cfg.defaultDuration,
      );
      if (PRINT_DEBUG)
        console.log("sync from local storage (or default fallback) →", value);
      this.setDuration(value); // don't re-save, we received it
    },

    setDuration(value, save = false) {
      if (value === "") this.duration = this.defaultDuration;
      else {
        const v = Number(value);
        const clamped = isNaN(v)
          ? this.min
          : Math.max(this.min, Math.min(this.max, v));
        this.duration = Math.round(clamped);
      }

      if (PRINT_DEBUG)
        console.log(
          wayID,
          "duration →",
          this.min,
          "≤",
          this.duration,
          "≤",
          this.max,
        );

      if (save && this.ready())
        manualWateringDurationService.set(wayID, this.duration);
    },

    increase() {
      this.setDuration(this.duration + this.step, true);
    },

    decrease() {
      this.setDuration(this.duration - this.step, true);
    },

    destroy() {
      this.unsubStorage?.();
    },
  }));
};
