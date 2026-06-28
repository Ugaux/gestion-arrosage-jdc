import { formatDuration } from "../core/formatting.js";

export default (Alpine) => {
  Alpine.data("settingsPage", () => ({
    get seasonalAdj() {
      const value = Alpine.store("deviceCfg").seasonalAdj * 100;
      return Math.round(value * 100) / 100;
    },

    get soilMoistureThreshold() {
      const value = Alpine.store("deviceCfg").soilMoistureThreshold * 100;
      return Math.round(value * 100) / 100;
    },

    get lastResetReason() {
      return Alpine.store("health").resets.total > 0
        ? "(last: " + Alpine.store("health").resets.lastReason + ")"
        : "";
    },

    get deviceRuntime() {
      return formatDuration(
        Alpine.store("deviceInfo").localTimeSec -
          Alpine.store("health").runtime,
      );
    },

    get lastCmdReceived() {
      if (Alpine.store("health").lastCommandReceived.name !== "")
        return {
          name: Alpine.store("health").lastCommandReceived.name,
          elapsedTime:
            formatDuration(
              Alpine.store("deviceInfo").localTimeSec -
                Alpine.store("health").lastCommandReceived.timestamp,
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
