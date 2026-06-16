import { formatDuration } from "../core/formatting.js";

export default (Alpine) => {
  Alpine.data("wateringCard", (wayID) => ({
    get wateringStatus() {
      // Status       Inactive / Scheduled -> 2:05 Dripline + X more / Manual -> 10:17 Lawn + X more
      // Next run     Next: House on May 24 at 22:00 for 10 min / No scheduled irrigation

      let state = "Unknown";
      if (Alpine.store("watering").state === 0) state = "Inactive";
      else if (
        Alpine.store("watering").state == 1 ||
        Alpine.store("watering").state == 2
      ) {
        state =
          " " +
          Alpine.store("watering").currentRun.zone +
          " " +
          formatDuration(Alpine.store("watering").currentRun.timeLeft) +
          " left";
        if (Alpine.store("watering").currentRun.totalZones > 1)
          state =
            state +
            " + " +
            Alpine.store("watering").currentRun.totalZones +
            " more";
      }

      let icon = "";
      if (Alpine.store("watering").state == 1) icon = "fa-circle-play";
      else if (Alpine.store("watering").state == 2) icon = "fa-calendar";

      let nextRun = "No scheduled irrigation";
      if (Alpine.store("watering").nextRun)
        nextRun =
          "Next: " +
          Alpine.store("watering").nextRun.zone +
          " in " +
          formatDuration(
            Alpine.store("watering").nextRun.startTime -
              Alpine.store("deviceInfo").localTimeSec,
          ) +
          " for " +
          formatDuration(Alpine.store("watering").nextRun.duration);

      let activeValves = "No active valves";
      if (Alpine.store("watering").activeValves)
        activeValves =
          "Active valves: " + Alpine.store("watering").activeValves.join(", ");

      return { state, icon, nextRun, activeValves };
    },

    get lastWatered() {
      const value = formatDuration(
        Alpine.store("deviceInfo").localTimeSec -
          Alpine.store("watering").lastWateredTime,
      );
      return "Last watered " + value + " ago";
    },

    get soilMoisture() {
      const value = Alpine.store("sensors").soilMoisture * 100;
      return Math.round(value);
    },

    get waterFlow() {
      return Math.round(Alpine.store("sensors").waterFlow);
    },
    get pumpStateLabel() {
      const stateMap = {
        0: "OFF",
        1: "ON",
      };
      return stateMap[Alpine.store("sensors").pump.state] ?? "Unknown"; // Fallback
    },

    get byHandLabel() {
      const state = Number(Alpine.store("watering").byHand.state);

      if (state === 0) return { main: "Inactive", suffix: "" };

      if (state === 1) {
        return {
          main: formatDuration(Alpine.store("watering").byHand.timeLeft),
          suffix: " left",
        };
      }

      return { main: "Unknown", suffix: "" };
    },
  }));
};
