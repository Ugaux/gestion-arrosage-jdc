import { AppCfg } from "../core/appCfg.js";

export default (Alpine) => {
  Alpine.store("health", {
    state: -1, // 0:ok/1:degraded/2:fault/-1:Unknown
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
    lastCommandReceived: {
      timestamp: 0, // seconds since last instruction
      name: "",
    },

    init() {
      if (AppCfg.debug) {
        this.state = 2;
        this.faults = [
          {
            index: 0,
            id: 46589655,
            text: "no water is flowing to fill water tank",
            timeStamp: 1768908922,
          },
          {
            index: 1,
            id: 23548569,
            text: "no flow while watering, check pump and/or valves",
            timeStamp: 1768909922,
          },
        ];
        if (true) {
          setTimeout(() => {
            const a = this.faults.shift();
          }, 4000);
          setTimeout(() => {
            this.state = 0;
          }, 8000);
        }
        if (false)
          setInterval(() => {
            this.state = this.state === 0 ? 2 : 0;
          }, 4000);
        if (false)
          setInterval(() => {
            this.state = Math.round(Math.random() * 3);
          }, 4000);

        this.runtime = AppCfg.debugLocalTimeSec - 50;
        this.resets = {
          total: 0,
          lastReason: "",
        };
        setTimeout(() => {
          this.resets = {
            total: 1,
            lastReason: "brownout",
          };
          this.heap = {
            largestFreeBlock: 250.15, // in KB
            fragmentation: 11.3, // = 1.0 - (largest_free_block / total_free_heap) -> in %
            minFreeSize: 123.36, // in KB
          };
          this.lastCommandReceived = {
            timestamp: AppCfg.debugLocalTimeSec - 23, // seconds since last instruction
            name: "/start_zone_2",
          };
        }, 3000);
      }
    },
  });
};
