export default (Alpine) => {
  Alpine.store("sensors", {
    pump: {
      state: -1, // 0:OFF/1:ON
      cycles: 0,
    },

    soilMoisture: 0,

    waterFlow: 0,

    waterTank: {
      level: -1, // 0:EMPTY/1:NORMAL/2:FULL
      state: -1, // 0:FILLING/1:EMPTYING
      lastFill: 0, //  since epoch
    },
  });
};
