import { AppCfg } from "../core/appCfg.js";

export default (Alpine) => {
  Alpine.store("deviceCfg", {
    manualWateringDuration: {
      min: 0, //min
      max: 0, //min
      step: 0, //min
      defaultDuration: 0, //min
    },

    seasonalAdj: 0, // 0-1

    soilMoistureThreshold: 0, // 0-1

    lastModified: 0,

    init() {
      if (AppCfg.debug) {
        this.manualWateringDuration = {
          min: 1,
          max: 40,
          step: 5,
          defaultDuration: 15,
        };

        this.seasonalAdj = 1;
        this.soilMoistureThreshold = 0.6;

        this.lastModified = AppCfg.debugLocalTimeSec - 41;

        if (false) {
          setTimeout(() => {
            console.log("update form deviceCfg store");
            this.manualWateringDuration = {
              min: 2,
              max: 80,
              step: 10,
              defaultDuration: 30,
            };

            this.seasonalAdj = 0.9;
            this.soilMoistureThreshold = 0.7;
          }, 2000);
        }
        if (false) {
          setTimeout(() => {
            console.log("update form deviceCfg store");
            this.manualWateringDuration = {
              min: 1,
              max: 50,
              step: 4,
              defaultDuration: 27,
            };
          }, 7000);
        }
        if (false) {
          setTimeout(() => {
            console.log("update form deviceCfg store");
            this.manualWateringDuration = {
              min: 1,
              max: 40,
              step: 4,
              defaultDuration: 30,
            };
          }, 14000);
        }
      }
    },
  });
};
