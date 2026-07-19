import { formatDuration } from "../core/formatting.js";

const DEFAULT_RAW_CFG_TEXT =
  "Click the download button to get settings.ini content...";

export default (Alpine) => {
  Alpine.data("settingsPage", () => ({
    init() {
      this.resetConfigText();
    },

    toggleValve(index) {
      Alpine.store("wsClient").sendExclusive(`toggleValve:${index}`, {
        action: "toggleValve",
        index,
      });
    },

    syncTime() {
      Alpine.store("wsClient").sendExclusive("syncTime", {
        action: "syncTime",
        timeMillisec: Date.now(),
      });
    },

    resetConfigText() {
      this.rawConfigText = DEFAULT_RAW_CFG_TEXT;
      this.cfgTextAvailable = false;
      this.configTextHasChanged = false;
    },
    async downloadConfigText() {
      try {
        const result = await Alpine.store("wsClient").sendExclusive(
          `getDeviceCfg`,
          {
            action: "getDeviceCfg",
          },
        );
        if (!result.initiated) return;
        if (!result.payload) return;

        this.rawConfigText = result.payload;
        this.cfgTextAvailable = true;
        this.configTextHasChanged = false;
      } catch (err) {
        console.error(err);
      }
    },
    async uploadConfigText() {
      try {
        const result = await Alpine.store("wsClient").sendExclusive(
          `setDeviceCfg`,
          {
            action: "setDeviceCfg",
            rawConfigText: this.rawConfigText,
          },
        );
        if (!result.initiated) return;

        Alpine.store("toast").show("Device is rebooting...", {
          type: "info",
          description: "Please wait a few seconds!",
          duration: 30000,
          tag: "device-reboot",
        });
        this.resetConfigText();
      } catch (err) {
        console.error(err);
      }
    },

    get seasonalAdj() {
      const value = Alpine.store("deviceCfg").seasonalAdj * 100;
      return Math.round(value * 100) / 100;
    },

    get soilMoistureThreshold() {
      const value = Alpine.store("deviceCfg").soilMoistureThreshold * 100;
      return Math.round(value * 100) / 100;
    },

    get lastResetReason() {
      return Alpine.store("deviceHealth").resets.total > 0
        ? "(last: " + Alpine.store("deviceHealth").resets.lastReason + ")"
        : "";
    },

    get deviceRuntime() {
      return formatDuration(
        Alpine.store("deviceInfo").localTimeSec -
          Alpine.store("deviceHealth").runtime,
      );
    },

    get lastCmdReceived() {
      if (Alpine.store("deviceHealth").lastReceivedCommand.name !== "")
        return {
          name: Alpine.store("deviceHealth").lastReceivedCommand.name,
          elapsedTime:
            formatDuration(
              Alpine.store("deviceInfo").localTimeSec -
                Alpine.store("deviceHealth").lastReceivedCommand.timestamp,
            ) + " ago",
        };

      return {
        name: "Unknown",
        elapsedTime: "",
      };
    },

    get commissioningDate() {
      return new Date(
        Alpine.store("deviceInfo").commissioningTimeSec * 1000,
      ).toLocaleString("en-GB", {
        weekday: "short",
        day: "2-digit",
        month: "2-digit",
        year: "numeric",
      });
    },

    get scheduleLastModified() {
      return (
        formatDuration(
          Alpine.store("deviceInfo").localTimeSec -
            Alpine.store("watering").scheduleLastModified,
        ) + " ago"
      );
    },

    get deviceCfgLastModified() {
      return (
        formatDuration(
          Alpine.store("deviceInfo").localTimeSec -
            Alpine.store("deviceCfg").lastModified,
        ) + " ago"
      );
    },
  }));
};
