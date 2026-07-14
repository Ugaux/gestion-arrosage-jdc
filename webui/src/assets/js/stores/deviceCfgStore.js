export default (Alpine) => {
  Alpine.store("deviceCfg", {
    manualWateringDuration: {
      min: 0, //min
      max: 0, //min
      step: 0, //min
      defaultDuration: 0, //min
    },

    seasonalAdj: 0, // 0-1

    soilMoistureThreshold: 0, // 0-1

    lastModified: 0,
  });
};
