import { formatDuration } from "../core/formatting.js";

export default (Alpine) => {
  Alpine.data("homePage", () => ({
    // ************** System Card ************** //

    get deviceStateLabel() {
      const stateMap = {
        0: "OK",
        1: "Degraded",
        2: "Fault",
      };
      return stateMap[Alpine.store("health").state] ?? "Unknown"; // Fallback
    },

    get deviceStateClass() {
      const stateMap = {
        0: "ok-state",
        1: "degraded-state",
        2: "fault-state",
      };
      return stateMap[Alpine.store("health").state] ?? "unknown-state"; // Fallback
    },

    get deviceStateIcon() {
      const stateMap = {
        0: "check", // ✓
        1: "triangle-exclamation", // ⚠
        2: "xmark", // ✕
      };
      return (
        stateMap[Alpine.store("health").state] ?? "circle-question" // Fallback
      );
    },

    get localDate() {
      return new Date(
        Alpine.store("deviceInfo").localTimeSec * 1000,
      ).toLocaleString("en-GB", {
        weekday: "short",
        day: "2-digit",
        month: "2-digit",
        year: "numeric",
      });
    },

    get localTime() {
      return new Date(
        Alpine.store("deviceInfo").localTimeSec * 1000,
      ).toLocaleTimeString("en-GB", {
        timeZone: "Europe/Paris",
        hour12: false,
      });
    },

    get wifiSignal() {
      const levels = {
        excellent: "text-success",
        good: "text-info",
        medium: "text-warning",
        weak: "text-error",
        veryWeak: "text-error opacity-60",
      };

      const rssi = Alpine.store("deviceInfo").wifiRSSI;
      let label = "";
      let colorClass = "";
      let opacity = "";
      if (typeof rssi !== "number" || rssi > 0 || rssi <= -100) {
        label = "Unknown";
        colorClass = "";
        opacity = "0.7"; // opacity = "0.7";
      } else if (rssi <= -80) {
        label = "Very weak"; // Inférieur à -80 dBm : Signal Très faible ou inutilisable, causant des déconnexions fréquentes et une latence élevée.
        colorClass = "veryweak-wifi";
        opacity = "0.7"; // opacity = "0.4";
      } else if (rssi <= -70) {
        label = "Weak"; // -70 à -80 dBm : Signal Faible, entraînant des vitesses lentes, des buffering et une instabilité potentielle.
        opacity = "0.5"; // opacity = "0.4";
      } else if (rssi <= -60) {
        label = "Medium"; // -60 à -70 dBm : Signal Moyen, suffisant pour la navigation standard et les emails, mais avec une performance dégradée.
        opacity = "0.5"; // opacity = "0.55";
      } else if (rssi <= -50) {
        label = "Good"; // -50 à -60 dBm : Signal Bon, fiable pour le streaming HD, les visioconférences et la navigation web.
        opacity = "0.5"; // opacity = "0.7";
      } else {
        label = "Excellent"; // 0 à -50 dBm : Signal Excellent, permettant le streaming 4K, les jeux en ligne et des téléchargements rapides.
        colorClass = "excellent-wifi";
        opacity = "0.7"; // opacity = "0.8";
      }

      return { label, colorClass, opacity, rssi }; // Fallback
    },

    // ************** Watering Card ************** //

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
      if (Alpine.store("watering").state == 1) icon = "circle-play";
      else if (Alpine.store("watering").state == 2) icon = "calendar";

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

    // ************** Water Tank Card ************** //

    get waterTankLevelLabel() {
      const stateMap = {
        0: "Empty",
        1: "Normal",
        2: "Full",
      };
      return stateMap[Alpine.store("sensors").waterTank.level] ?? "Unknown"; // Fallback
    },

    get waterTankStateLabel() {
      const stateMap = {
        0: "Filling",
        1: "Emptying",
      };
      return stateMap[Alpine.store("sensors").waterTank.state] ?? "Unknown"; // Fallback
    },

    get waterTankLastFillLabel() {
      return formatDuration(
        Alpine.store("deviceInfo").localTimeSec -
          Alpine.store("sensors").waterTank.lastFill,
      );
    },
  }));
};
