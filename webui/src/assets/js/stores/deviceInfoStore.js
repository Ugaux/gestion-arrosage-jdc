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
  });
};
