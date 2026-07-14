import { formatDuration } from "../core/formatting.js";

export default (Alpine) => {
  Alpine.data("settingsPage", () => ({
    toggleValve(index) {
      Alpine.store("wsClient").sendExclusive(
        `toggleValve:${index}`,
        {
          action: "toggleValve",
          index,
        },
        { showToast: true },
      );
    },

    syncTime() {
      Alpine.store("wsClient").sendExclusive(
        "syncTime",
        {
          action: "syncTime",
          timeSec: Math.floor(Date.now() / 1000),
        },
        { showToast: true },
      );
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
