import { AppCfg } from "../core/appCfg.js";

export default (Alpine) => {
  Alpine.store("deviceInfo", {
    id: "Unknown",
    commissioningTimeSec: 0, // since epoch
    versions: {
      firmware: "Unknown",
      protocol: "Unknown",
      hardware: "Unknown",
    },

    localTimeSec: 0, // since epoch
    wifiRSSI: -999, // Received Signal Strength Indicator in dBm

    init() {
      if (AppCfg.debug) {
        this.id = "esp32-a1b2c3";
        this.commissioningTimeSec = 1768809962;
        this.versions = {
          firmware: "1.0.0",
          protocol: "v1",
          hardware: "rev_A",
        };

        this.localTimeSec = AppCfg.debugLocalTimeSec;
        setInterval(() => {
          this.localTimeSec++;
        }, 1000);
        this.wifiRSSI = -25.7;
        if (true) {
          this.wifiRSSI = 0;
          setInterval(() => {
            if (this.wifiRSSI < -140) {
              this.wifiRSSI = 0;
              return;
            }
            this.wifiRSSI = this.wifiRSSI - 10;
          }, 1000);
        }
      }
    },
  });
};
