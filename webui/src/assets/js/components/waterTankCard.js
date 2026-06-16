import { formatDuration } from "../core/formatting.js";

export default (Alpine) => {
  Alpine.data("waterTankCard", (wayID) => ({
    get levelLabel() {
      const stateMap = {
        0: "Empty",
        1: "Normal",
        2: "Full",
      };
      return stateMap[Alpine.store("sensors").waterTank.level] ?? "Unknown"; // Fallback
    },

    get stateLabel() {
      const stateMap = {
        0: "Filling",
        1: "Emptying",
      };
      return stateMap[Alpine.store("sensors").waterTank.state] ?? "Unknown"; // Fallback
    },

    get lastFillLabel() {
      return formatDuration(
        Alpine.store("deviceInfo").localTimeSec -
          Alpine.store("sensors").waterTank.lastFill,
      );
    },
  }));
};
