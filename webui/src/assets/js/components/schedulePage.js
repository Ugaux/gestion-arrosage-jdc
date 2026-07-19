import { formatDuration } from "../core/formatting.js";

export default (Alpine) => {
  Alpine.data("schedulePage", () => ({
    formatFrequency(freq, customDays) {
      if (freq === "always") return "Always";
      if (freq === "odd") return "Odd days";
      if (freq === "even") return "Even days";
      if (freq === "custom") return customDays.join("/");
      return "Unknown";
    },

    formatNextRun(nextRun) {
      return formatDuration(Alpine.store("deviceInfo").localTimeSec - nextRun);
    },

    sortedSchedules(schedules) {
      return [...schedules].sort((a, b) => {
        return a.time.localeCompare(b.time);
      });
    },

    toggleScheduleEnable(wayId, scheduleId) {
      Alpine.store("wsClient").sendExclusive(`toggleScheduleEnable:${wayId}`, {
        action: "toggleScheduleEnable",
        wayId,
        scheduleId,
      });
    },
  }));
};
