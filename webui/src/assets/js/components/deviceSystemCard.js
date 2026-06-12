export default (wayID) => ({
  get stateLabel() {
    const stateMap = {
      0: "OK",
      1: "Degraded",
      2: "Fault",
    };
    return stateMap[Alpine.store("health").state] ?? "Unknown"; // Fallback
  },

  get stateClass() {
    const stateMap = {
      0: "ok-state",
      1: "degraded-state",
      2: "fault-state",
    };
    return stateMap[Alpine.store("health").state] ?? "unknown-state"; // Fallback
  },

  get stateIcon() {
    const stateMap = {
      0: "fa-check", // ✓
      1: "fa-triangle-exclamation", // ⚠
      2: "fa-xmark", // ✕
    };
    return (
      stateMap[Alpine.store("health").state] ?? "fa-circle-question" // Fallback
    );
  },

  get localDate() {
    return new Date(Alpine.store("device").localTimeSec * 1000).toLocaleString(
      "en-GB",
      {
        weekday: "short",
        day: "2-digit",
        month: "2-digit",
        year: "numeric",
      },
    );
  },

  get localTime() {
    return new Date(
      Alpine.store("device").localTimeSec * 1000,
    ).toLocaleTimeString();
  },

  get wifiSignal() {
    const levels = {
      excellent: "text-success",
      good: "text-info",
      medium: "text-warning",
      weak: "text-error",
      veryWeak: "text-error opacity-60",
    };

    const rssi = Alpine.store("device").wifiRSSI;
    let label = "";
    let colorClass = "";
    let opacity = "";
    if (typeof rssi !== "number" || rssi > 0 || rssi < -100) {
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
});
