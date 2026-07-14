export default (Alpine) => {
  Alpine.store("watering", {
    state: -1, // 0:inactive/1:manual/2:scheduled
    currentRun: null, // is filled when state is 1 or 2
    activeValves: null,

    nextRun: null,
    lastWateredTime: 0, // since epoch

    scheduleLastModified: 0,

    byHand: {
      state: -1, // 0:inactive/1:running
      timeLeft: 0, // seconds
    },
  });
};
