import { AppCfg } from "../core/appCfg.js";

export default (Alpine) => {
  Alpine.store("sensors", {
    pump: {
      state: -1, // 0:OFF/1:ON/-1:unknown
      cycles: 0,
    },

    soilMoisture: 0,

    waterFlow: 0,

    waterTank: {
      level: -1, // 0:EMPTY/1:NORMAL/2:FULL/-1:Unknown
      state: -1, // 0:FILLING/1:EMPTYING/-1:Unknown
      lastFill: 0, //  since epoch
    },

    init() {
      if (AppCfg.debug) {
        this.pump = {
          state: 0, // OFF/ON
          cycles: 7,
        };
        setInterval(() => {
          this.pump.state = this.pump.state === 0 ? 1 : 0;
        }, 6000);
        setInterval(() => {
          this.pump.cycles++;
        }, 12000);

        this.soilMoisture = 0.3236564;
        setInterval(() => {
          this.soilMoisture = Math.random() / 10 + 0.7;
        }, 500);

        this.waterFlow = 56.8512145;
        setInterval(() => {
          this.waterFlow = Math.random() * 80;
        }, 500);

        this.waterTank = {
          level: 1,
          state: 1,
          lastFill: AppCfg.debugLocalTimeSec - (3600 * 24 * 5 + 22 * 3600),
        };
        setInterval(() => {
          this.waterTank.level = this.waterTank.level === 1 ? 2 : 1;
          this.waterTank.state = this.waterTank.state === 1 ? 0 : 1;
        }, 8000);
      }
    },
  });
};
