export default (Alpine) => {
  Alpine.data("manualPage", () => ({
    toggleManualWatering(wayID, durationSec) {
      Alpine.store("wsClient").sendExclusive(
        `toggleManualWatering:${wayID}`,
        {
          action: "toggleManualWatering",
          id: wayID,
          duration: durationSec,
        },
        { showToast: true },
      );
    },
  }));
};
