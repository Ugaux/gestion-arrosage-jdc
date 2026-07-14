import { logStatus } from "./utilities.js";

const LOG_PREFIX = "[wsRouter]";

export function handleMessage(Alpine, client, msg) {
  switch (msg.type) {
    case "SNAPSHOT": {
      logStatus(LOG_PREFIX, "received SNAPSHOT");
      onSnapshot(Alpine, msg.payload); // Process the initial data
      client.markReady(); // Signal that the client is ready
      break;
    }

    case "EVENT": {
      logStatus(LOG_PREFIX, `received EVENT ${msg.event} in ${msg.topic}`);
      dispatchMessage(Alpine, msg);
      break;
    }

    case "ACK": {
      if (client.resolveAck(msg))
        logStatus(LOG_PREFIX, `received ACK ${msg.ok} for ${msg.id}`);
      break;
    }

    default: {
      logStatus(LOG_PREFIX, `unknown message type: ${msg.type}`, "warn");
    }
  }
}

// Full snapshot — replace everything (sent by ESP32 on every fresh connection)
function onSnapshot(Alpine, payload) {
  for (const msg of payload) {
    dispatchMessage(Alpine, msg);
  }
}

const messageDispatchTable = {
  deviceConfig: {
    updateAll: (Alpine, payload) => {
      Object.assign(Alpine.store("deviceCfg"), payload);
    },
  },

  deviceHealth: {
    updateAll: (Alpine, payload) => {
      Object.assign(Alpine.store("deviceHealth"), payload);
    },
    updateState: (Alpine, payload) => {
      Alpine.store("deviceHealth").state = payload.state;
      Alpine.store("deviceHealth").faults = payload.faults;
    },
    updateRuntime: (Alpine, payload) => {
      Alpine.store("deviceHealth").runtime = payload.runtime;
      Alpine.store("deviceHealth").resets = payload.resets;
    },
    updateHeap: (Alpine, payload) => {
      Alpine.store("deviceHealth").heap = payload;
    },
    updateLastRcvdCmd: (Alpine, payload) => {
      Alpine.store("deviceHealth").lastReceivedCommand = payload;
    },
  },

  deviceInfo: {
    updateAll: (Alpine, payload) => {
      Object.assign(Alpine.store("deviceInfo"), payload);
    },
    updateLocalTime: (Alpine, payload) => {
      Alpine.store("deviceInfo").localTimeSec = payload;
    },
    updateWifiRSSI: (Alpine, payload) => {
      Alpine.store("deviceInfo").wifiRSSI = payload;
    },
  },

  sensors: {
    updateAll: (Alpine, payload) => {
      Object.assign(Alpine.store("sensors"), payload);
    },
    updatePump: (Alpine, payload) => {
      Alpine.store("sensors").pump = payload;
    },
    updateSoilMoisture: (Alpine, payload) => {
      Alpine.store("sensors").soilMoisture = payload;
    },
    updateWaterFlow: (Alpine, payload) => {
      Alpine.store("sensors").waterFlow = payload;
    },
    updateWaterTank: (Alpine, payload) => {
      Alpine.store("sensors").waterTank = payload;
    },
  },

  valves: {
    updateAll: (Alpine, payload) => {
      Alpine.store("valves").items = payload;
    },
  },

  watering: {
    updateAll: (Alpine, payload) => {
      Object.assign(Alpine.store("watering"), payload);
    },
    updateCurrentRun: (Alpine, payload) => {
      Alpine.store("watering").state = payload.state;
      Alpine.store("watering").currentRun = payload.currentRun;
      Alpine.store("watering").activeValves = payload.activeValves;
    },
    updateNextRun: (Alpine, payload) => {
      Alpine.store("watering").nextRun = payload;
    },
    updateLastWateringTime: (Alpine, payload) => {
      Alpine.store("watering").lastWateredTime = payload;
    },
    updateScheduleLastModified: (Alpine, payload) => {
      Alpine.store("watering").scheduleLastModified = payload;
    },
    updateByHand: (Alpine, payload) => {
      Alpine.store("watering").byHand = payload;
    },
  },

  zones: {
    // only when zone created/deleted, way created/deleted, reconnect/resync after connection loss
    updateAll: (Alpine, payload) => {
      Alpine.store("zones").setItems(payload);
    },
    updateZoneName: (Alpine, payload) => {
      const zone = Alpine.store("zones").getZone(payload.id);
      if (zone) zone.name = payload.name;
    },
    updateWayName: (Alpine, payload) => {
      const way = Alpine.store("zones").getWay(payload.id);
      if (way) {
        way.name = payload.name;
        way.shortName = payload.shortName;
      }
    },
    updateWayManual: (Alpine, payload) => {
      const way = Alpine.store("zones").getWay(payload.id);
      if (way) way.manual = payload.manual;
    },
    updateWayNextSchedule: (Alpine, payload) => {
      const way = Alpine.store("zones").getWay(payload.id);
      if (way) way.nextSchedule = payload.nextSchedule;
    },
    updateWaySchedules: (Alpine, payload) => {
      const way = Alpine.store("zones").getWay(payload.id);
      if (way) way.schedules = payload.schedules;
    },
  },
};

function dispatchMessage(Alpine, msg) {
  const topic = messageDispatchTable[msg.topic];

  if (!topic) {
    logStatus(LOG_PREFIX, `unknown topic: ${msg.topic}`, "warn");
    return;
  }

  const handler = topic[msg.event];

  if (!handler) {
    logStatus(
      LOG_PREFIX,
      `unknown event "${msg.event}" for topic "${msg.topic}"`,
      "warn",
    );
    return;
  }

  handler(Alpine, msg.payload);
}
