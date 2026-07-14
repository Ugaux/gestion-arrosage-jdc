export default (Alpine) => {
  Alpine.store("deviceHealth", {
    state: -1, // 0:ok/1:degraded/2:fault
    faults: [],

    runtime: 0,
    resets: {
      total: 0,
      lastReason: "Unknown", // watchdog, power-on, crash, etc.
    },

    // -> check every 60 seconds
    heap: {
      largestFreeBlock: 0, // in KB
      fragmentation: 0, // = 1.0 - (largest_free_block / total_free_heap) -> in %
      minFreeSize: 0, // in KB
    },

    lastReceivedCommand: {
      timestamp: 0, // seconds since last instruction
      name: "",
    },
  });
};
