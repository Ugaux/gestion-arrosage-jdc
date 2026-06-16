import { AppCfg } from "../core/appCfg.js";

export default (Alpine) => {
  Alpine.store("watering", {
    state: -1, // 0:inactive/1:manual/2:scheduled/-1:unknown
    currentRun: null, // is filled when state is 1 or 2

    nextRun: null,
    activeValves: null,
    lastWateredTime: 0, // since epoch

    scheduleLastModified: 0,

    byHand: {
      state: -1, // 0:inactive/1:running/-1:unknown
      timeLeft: 0, // seconds
    },

    init() {
      if (AppCfg.debug) {
        this.state = 1;
        this.currentRun = {
          zone: "Dripline",
          timeLeft: 9,
          totalZones: 3,
        };
        setInterval(() => {
          if (!this.currentRun) return;
          if (this.currentRun.timeLeft <= 0) {
            this.state = 0;
            this.currentRun = null;
            return;
          }
          this.currentRun.timeLeft--;
        }, 1000);
        this.nextRun = {
          zone: "House",
          startTime: AppCfg.debugLocalTimeSec + 3600 * 29,
          duration: 952,
        };
        this.activeValves = [2, 6];
        this.lastWateredTime = AppCfg.debugLocalTimeSec - 20;

        this.scheduleLastModified = AppCfg.debugLocalTimeSec - 34;

        this.byHand = {
          state: 1, // Inactive/Running
          timeLeft: 8, // seconds
        };
        setInterval(() => {
          if (this.byHand.timeLeft <= 0) {
            this.byHand.state = 0;
            return;
          }
          this.byHand.timeLeft--;
        }, 1000);
      }
    },
  });
};
