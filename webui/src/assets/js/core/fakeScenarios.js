import { AppCfg } from "./appCfg";
import { logStatus, generateUniqueID } from "./utilities";
import dedent from "dedent";

const LOG_PREFIX = "[fakeScenarios]";

const SNAPSHOT_DATA = {
  device: {
    config: {
      manualWateringDuration: {
        min: 1,
        max: 40,
        step: 5,
        defaultDuration: 15,
      },

      seasonalAdj: 1,
      soilMoistureThreshold: 0.6,

      lastModified: AppCfg.fakeDataLocalTimeSec - 1.2 * 3600 - 17,
    },

    health: {
      state: 0,
      faults: [],
      runtime: AppCfg.fakeDataLocalTimeSec - 50,
      resets: {
        total: 1,
        lastReason: "brownout",
      },
      heap: {
        largestFreeBlock: 250.15,
        fragmentation: 11.3,
        minFreeSize: 123.36,
      },
      lastReceivedCommand: {
        timestamp: AppCfg.fakeDataLocalTimeSec - 23,
        name: "/start_zone_2",
      },
    },

    info: {
      id: "esp32-a1b2c3",
      commissioningTimeSec: AppCfg.fakeDataLocalTimeSec - 3600 * 24 * 5,
      versions: {
        firmware: "1.0.0",
        protocol: "v1",
        hardware: "rev_A",
      },
      localTimeSec: AppCfg.fakeDataLocalTimeSec,
      wifiRSSI: -25.7,
    },
  },

  sensors: {
    pump: {
      state: 1,
      cycles: 7,
    },

    soilMoisture: 0.3236564,

    waterFlow: 33.8512145,

    waterTank: {
      level: 1,
      state: 1,
      lastFill: AppCfg.fakeDataLocalTimeSec - (3600 * 24 * 5 + 22 * 3600),
    },
  },

  valves: [
    { index: 0, name: "Expander-GPIO:0", is_checked: false },
    { index: 1, name: "Expander-GPIO:1", is_checked: false },
    { index: 2, name: "Expander-GPIO:2", is_checked: true },
    { index: 3, name: "Expander-GPIO:3", is_checked: false },
    { index: 4, name: "Expander-GPIO:4", is_checked: false },
    { index: 5, name: "Expander-GPIO:5", is_checked: true },
    { index: 6, name: "Expander-GPIO:6", is_checked: false },
    { index: 7, name: "Expander-GPIO:7", is_checked: false },
  ],

  watering: {
    state: 2,
    currentRun: {
      zone: "Dripline",
      timeLeft: 27,
      totalZones: 3,
    },
    activeValves: [2, 6],

    nextRun: {
      zone: "House",
      startTime: AppCfg.fakeDataLocalTimeSec + 3600 * 29,
      duration: 952,
    },
    lastWateredTime: AppCfg.fakeDataLocalTimeSec - 20,

    scheduleLastModified: AppCfg.fakeDataLocalTimeSec - 34,

    byHand: {
      state: 1,
      timeLeft: 8,
    },
  },

  zones: [
    {
      id: "12364598",
      name: "Lawn",
      ways: [
        {
          id: "45621123",
          name: "Lawn.House",
          shortName: "House",
          manual: { started: false, timeLeft: 0 },
          nextSchedule: AppCfg.fakeDataLocalTimeSec - 3953,
          schedules: [
            {
              id: "99621123",
              name: "Morning",
              enabled: false,
              time: "08:00",
              duration: 12,
              freq: "even",
              skipIfSoilIsMoist: true,
            },
          ],
        },
        {
          id: "74182539",
          name: "Lawn.Studio",
          shortName: "Studio",
          manual: { started: false, timeLeft: 0 },
          nextSchedule: AppCfg.fakeDataLocalTimeSec - 13953,
          schedules: [
            {
              id: "99182539",
              name: "",
              enabled: true,
              time: "08:01",
              duration: 13,
              freq: "always",
              skipIfSoilIsMoist: false,
            },
            {
              id: "99182540",
              enabled: false,
              time: "23:00",
              duration: 16,
              freq: "odd",
              skipIfSoilIsMoist: false,
            },
          ],
        },
      ],
    },
    {
      id: "92694589",
      name: "Dripline",
      ways: [
        {
          id: "95311135",
          name: "Dripline.Flowers",
          shortName: "Flowers",
          manual: { started: true, timeLeft: 129 },
          nextSchedule: AppCfg.fakeDataLocalTimeSec - 4651953,
          schedules: [
            {
              id: "99311135",
              enabled: true,
              time: "08:07",
              duration: 17,
              freq: "always",
              skipIfSoilIsMoist: true,
            },
          ],
        },
        {
          id: "36985896",
          name: "Dripline.Vegetables",
          shortName: "Vegetables",
          manual: { started: false, timeLeft: 0 },
          nextSchedule: AppCfg.fakeDataLocalTimeSec - 313953,
          schedules: [
            {
              id: "99985896",
              name: " ",
              enabled: true,
              time: "08:17",
              duration: 23,
              freq: "custom",
              customDays: ["Mon", "Tue", "Sun"],
              skipIfSoilIsMoist: true,
            },
          ],
        },
      ],
    },
  ],
};

export function createSnapshot(data) {
  return {
    type: "SNAPSHOT",
    payload:
      data && Object.keys(data).length > 0
        ? [
            // device config
            {
              topic: "deviceConfig",
              event: "updateAll",
              payload: data?.device?.config,
            },

            // device health
            {
              topic: "deviceHealth",
              event: "updateAll",
              payload: data?.device?.health,
            },

            // device info
            {
              topic: "deviceInfo",
              event: "updateAll",
              payload: data?.device?.info,
            },

            // sensors
            {
              topic: "sensors",
              event: "updateAll",
              payload: data?.sensors,
            },

            // valves
            {
              topic: "valves",
              event: "updateAll",
              payload: data?.valves,
            },

            // watering
            {
              topic: "watering",
              event: "updateAll",
              payload: data?.watering,
            },

            // zones
            {
              topic: "zones",
              event: "updateAll",
              payload: data?.zones,
            },
          ]
        : [],
  };
}

export const scenarios = {
  empty: {}, // only works when loading a page with it (does not replace all values with default ones)

  idle: {
    data: SNAPSHOT_DATA,

    events: [
      {
        at: 1000,
        run(socket) {
          socket._emitFromServer({
            type: "EVENT",
            topic: "zones",
            event: "updateZoneName",
            payload: { id: 92694589, name: "NewDripline" },
          });
        },
      },
      {
        at: 3000,
        run(socket) {
          socket._emitFromServer({
            type: "EVENT",
            topic: "zones",
            event: "updateZoneName",
            payload: { id: 12364598, name: "NewLawn" },
          });
        },
      },
    ],

    tick(socket) {
      if (socket._every("wifiRSSI", 1000, 5000)) {
        if (socket.data.device.info.wifiRSSI < -120)
          socket.data.device.info.wifiRSSI = 0;
        else
          socket.data.device.info.wifiRSSI =
            socket.data.device.info.wifiRSSI - Math.random() - 10;
        socket._emitFromServer({
          type: "EVENT",
          topic: "deviceInfo",
          event: "updateWifiRSSI",
          payload: socket.data.device.info.wifiRSSI,
        });
      }
      if (socket._every("pump", 36000)) {
        socket.data.sensors.pump.state =
          socket.data.sensors.pump.state === 0 ? 1 : 0;
        if (socket.data.sensors.pump.state === 0)
          socket.data.sensors.pump.cycles++;
        socket._emitFromServer({
          type: "EVENT",
          topic: "sensors",
          event: "updatePump",
          payload: socket.data.sensors.pump,
        });
      }
      if (socket._every("soilMoisture", 500, 15000)) {
        socket._emitFromServer({
          type: "EVENT",
          topic: "sensors",
          event: "updateSoilMoisture",
          payload: Math.random() / 10 + 0.33,
        });
      }
      if (socket._every("waterFlow", 1000, 10000)) {
        socket._emitFromServer({
          type: "EVENT",
          topic: "sensors",
          event: "updateWaterFlow",
          payload: 12 + Math.random() * 20,
        });
      }
      if (socket._every("waterTank", 9000)) {
        socket.data.sensors.waterTank.state =
          socket.data.sensors.waterTank.state === 0 ? 1 : 0;
        socket.data.sensors.waterTank.level =
          socket.data.sensors.waterTank.state === 1 ? 2 : 1;
        socket._emitFromServer({
          type: "EVENT",
          topic: "sensors",
          event: "updateWaterTank",
          payload: socket.data.sensors.waterTank,
        });
      }
      if (socket._every("wateringAuto", 1000)) {
        if (!this.currentRun) return;
        if (this.currentRun.timeLeft <= 0) {
          this.state = 0;
          this.currentRun = null;
          return;
        }
        this.currentRun.timeLeft--;

        socket.data.watering;
        socket._emitFromServer({
          type: "EVENT",
          topic: "watering",
          event: "updateAll",
          payload: socket.data.watering,
        });
      }
      if (socket._every("wateringByHand", 1000)) {
        if (socket.data.watering.byHand.state === 1) {
          if (socket.data.watering.byHand.timeLeft > 0)
            socket.data.watering.byHand.timeLeft--;
          if (socket.data.watering.byHand.timeLeft <= 0)
            socket.data.watering.byHand.state = 0;
          socket._emitFromServer({
            type: "EVENT",
            topic: "watering",
            event: "updateByHand",
            payload: socket.data.watering.byHand,
          });
        }
      }

      if (socket._every("heartbeat", 1000)) {
        socket.data.device.info.localTimeSec++;
        socket._emitFromServer({
          type: "EVENT",
          topic: "deviceInfo",
          event: "updateLocalTime",
          payload: socket.data.device.info.localTimeSec,
        });
      }
    },

    onCommand(socket, msg, cmdExecDelay) {
      let ackDelay = Math.round(
        Math.max(
          10,
          Math.random() * AppCfg.websocketAckTimeout - cmdExecDelay - 50,
        ),
      );
      logStatus(LOG_PREFIX, `replying from ${msg.id} in ${ackDelay}ms`);

      switch (msg.payload.action) {
        case "setValve": {
          const success = Math.round(Math.random());
          if (success) {
            setTimeout(() => {
              console.log(msg.payload.id);
              //socket.data.device.health.faults.shift();
              socket.data.device.health.faults =
                socket.data.device.health.faults.filter(
                  (f) => f.id !== msg.payload.id,
                );
              console.log(socket.data.device.health.faults);
              if (socket.data.device.health.faults.length === 0)
                socket.data.device.health.state = 0;
              socket._emitFromServer({
                type: "EVENT",
                topic: "deviceHealth",
                event: "updateState",
                payload: {
                  state: socket.data.device.health.state,
                  faults: socket.data.device.health.faults,
                },
              });
              socket.onmessage?.({
                data: JSON.stringify({
                  type: "ACK",
                  id: msg.id,
                  ok: success,
                  error: success
                    ? {}
                    : {
                        name: "Failed to set valve",
                        message: "Valve is not connected",
                      },
                }),
              });
            }, ackDelay);
          }
          break;
        }
      }
    },
  },

  systemFaults: {
    data: SNAPSHOT_DATA,

    events: [
      {
        at: 500,
        run(socket) {
          socket._emitFromServer({
            type: "EVENT",
            topic: "deviceHealth",
            event: "updateState",
            payload: {
              state: 1,
              faults: [],
            },
          });
        },
      },
      {
        at: 1000,
        run(socket) {
          socket._emitFromServer({
            type: "EVENT",
            topic: "deviceHealth",
            event: "updateState",
            payload: {
              state: -1,
              faults: [],
            },
          });
        },
      },
      {
        at: 2000,
        run(socket) {
          const state = 2;
          const faults = [
            {
              index: 0,
              id: 46589655,
              text: "no water is flowing to fill water tank",
              timeStamp: 1768908922,
            },
          ];
          socket.data.device.health.state = 2;
          socket.data.device.health.faults = structuredClone(faults);
          socket._emitFromServer({
            type: "EVENT",
            topic: "deviceHealth",
            event: "updateState",
            payload: {
              state,
              faults,
            },
          });
        },
      },
      {
        at: 3000,
        run(socket) {
          const state = 2;
          const faults = [
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
          socket.data.device.health.state = 2;
          socket.data.device.health.faults = structuredClone(faults);
          socket._emitFromServer({
            type: "EVENT",
            topic: "deviceHealth",
            event: "updateState",
            payload: {
              state,
              faults,
            },
          });
        },
      },
      {
        at: 20000,
        run(socket) {
          socket.close();
        },
      },
    ],

    onCommand(socket, msg, cmdExecDelay) {
      let ackDelay = Math.round(
        Math.max(
          10,
          Math.random() * AppCfg.websocketAckTimeout - cmdExecDelay - 50,
        ),
      );
      logStatus(LOG_PREFIX, `replying from ${msg.id} in ${ackDelay}ms`);

      switch (msg.payload.action) {
        case "clearFault": {
          const success = Math.round(Math.random());
          if (success) {
            setTimeout(() => {
              //socket.data.device.health.faults.shift();
              socket.data.device.health.faults =
                socket.data.device.health.faults.filter(
                  (f) => f.id !== msg.payload.id,
                );
              console.log(socket.data.device.health.faults);
              if (socket.data.device.health.faults.length === 0)
                socket.data.device.health.state = 0;
              socket._emitFromServer({
                type: "EVENT",
                topic: "deviceHealth",
                event: "updateState",
                payload: {
                  state: socket.data.device.health.state,
                  faults: socket.data.device.health.faults,
                },
              });
              socket.onmessage?.({
                data: JSON.stringify({
                  type: "ACK",
                  id: msg.id,
                  ok: success,
                  error: success
                    ? {}
                    : {
                        name: "Failed to clear fault",
                        message: "ESP32 bad memory access",
                      },
                }),
              });
            }, ackDelay);
          }
          break;
        }
      }
    },
  },

  configUpdate: {
    data: SNAPSHOT_DATA,

    events: [
      {
        at: 2000,
        run(socket) {
          socket.data.device.config = {
            manualWateringDuration: {
              min: 2,
              max: 80,
              step: 10,
              defaultDuration: 30,
            },
            seasonalAdj: 0.9,
            soilMoistureThreshold: 0.7,
            lastModified: AppCfg.fakeDataLocalTimeSec - 41,
          };
          socket._emitFromServer({
            type: "EVENT",
            topic: "deviceConfig",
            event: "updateAll",
            payload: socket.data.device.config,
          });
        },
      },
      {
        at: 7000,
        run(socket) {
          socket.data.device.config.manualWateringDuration = {
            min: 1,
            max: 50,
            step: 4,
            defaultDuration: 27,
          };
          socket.data.device.config.lastModified =
            AppCfg.fakeDataLocalTimeSec - 41 + 7;
          socket._emitFromServer({
            type: "EVENT",
            topic: "deviceConfig",
            event: "updateAll",
            payload: socket.data.device.config,
          });
        },
      },
      {
        at: 14000,
        run(socket) {
          socket.data.device.config.manualWateringDuration = {
            min: 1,
            max: 40,
            step: 4,
            defaultDuration: 30,
          };
          socket.data.device.config.lastModified =
            AppCfg.fakeDataLocalTimeSec - 41 + 7 + 7;
          socket._emitFromServer({
            type: "EVENT",
            topic: "deviceConfig",
            event: "updateAll",
            payload: socket.data.device.config,
          });
        },
      },
    ],
  },

  scheduledWatering: {
    data: SNAPSHOT_DATA,

    events: [
      {
        at: 500,
        run(socket) {
          socket.data.watering.state = 1;
          socket.data.watering.currentRun = {
            zone: "Dripline",
            timeLeft: 21,
            totalZones: 2,
            activeValves: [2, 3],
          };
          socket._emitFromServer({
            type: "EVENT",
            topic: "watering",
            event: "updateCurrentRun",
            payload: {
              state: socket.data.watering.state,
              currentRun: socket.data.watering.currentRun,
            },
          });
        },
      },
      {
        at: 15000,
        run(socket) {
          ((socket.data.watering.nextRun = {
            zone: "Lawn",
            startTime: AppCfg.fakeDataLocalTimeSec + 3600 * 23,
            duration: 513,
          }),
            socket._emitFromServer({
              type: "EVENT",
              topic: "watering",
              event: "updateNextRun",
              payload: socket.data.watering.nextRun,
            }));
        },
      },
    ],

    tick(socket) {
      if (socket._every("heartbeat", 1000)) {
        socket.data.device.info.localTimeSec++;
        socket._emitFromServer({
          type: "EVENT",
          topic: "deviceInfo",
          event: "updateLocalTime",
          payload: socket.data.device.info.localTimeSec,
        });

        const currentRun = socket.data.watering.currentRun;
        if (currentRun.timeLeft > 0) {
          currentRun.timeLeft--;
          socket._emitFromServer({
            type: "EVENT",
            topic: "watering",
            event: "updateCurrentRun",
            payload: {
              state: 1,
              currentRun,
              activeValves: [2, 3],
            },
          });
          socket._emitFromServer({
            type: "EVENT",
            topic: "watering",
            event: "updateLastWateringTime",
            payload: socket.data.device.info.localTimeSec,
          });
        }
        if (currentRun.timeLeft <= 0) {
          socket._emitFromServer({
            type: "EVENT",
            topic: "watering",
            event: "updateCurrentRun",
            payload: {
              state: 0,
              currentRun,
            },
          });
        }
      }
    },

    onCommand(socket, msg, cmdExecDelay) {
      let ackDelay = Math.round(
        Math.max(
          10,
          Math.random() * AppCfg.websocketAckTimeout - cmdExecDelay - 50,
        ),
      );
      logStatus(LOG_PREFIX, `replying from ${msg.id} in ${ackDelay}ms`);

      switch (msg.payload.action) {
        case "toggleValve": {
          const v = socket.data.valves[msg.payload.index];
          let success = false;
          if (v) {
            success = true;
            v.is_checked = !v.is_checked;

            setTimeout(() => {
              socket._emitFromServer({
                type: "EVENT",
                topic: "valves",
                event: "updateAll",
                payload: socket.data.valves,
              });
              socket.onmessage?.({
                data: JSON.stringify({
                  type: "ACK",
                  id: msg.id,
                  ok: success,
                  error: success
                    ? {}
                    : {
                        name: "Failed to toggle valve",
                        message: "Index out of range",
                      },
                }),
              });
            }, 2);
          }
          break;
        }
        case "toggleManualWatering": {
          const m = socket.data.zones
            .flatMap((item) => item.ways)
            .find((way) => way.id === msg.payload.id).manual;
          m.started = !m.started;
          m.timeLeft = msg.payload.duration;

          setTimeout(() => {
            socket._emitFromServer({
              type: "EVENT",
              topic: "zones",
              event: "updateWayManual",
              payload: { id: msg.payload.id, manual: m },
            });
            socket.onmessage?.({
              data: JSON.stringify({
                type: "ACK",
                id: msg.id,
                ok: true,
                error: {},
              }),
            });
          }, ackDelay);
          break;
        }
        case "toggleScheduleEnable": {
          const schedules = socket.data.zones
            .flatMap((item) => item.ways)
            .find((way) => way.id === msg.payload.wayId).schedules;
          const schedule = schedules.find(
            (s) => s.id === msg.payload.scheduleId,
          );
          schedule.enabled = !schedule.enabled;

          setTimeout(() => {
            socket._emitFromServer({
              type: "EVENT",
              topic: "zones",
              event: "updateWaySchedules",
              payload: { id: msg.payload.wayId, schedules },
            });
            socket.onmessage?.({
              data: JSON.stringify({
                type: "ACK",
                id: msg.id,
                ok: true,
                error: {},
              }),
            });
          }, ackDelay);
          break;
        }
        case "addSchedule": {
          const zone = socket.data.zones
            .flatMap((item) => item.ways)
            .find((way) => way.id === msg.payload.wayId);
          zone.schedules.push({
            id: generateUniqueID(),
            name: "  ",
            enabled: true,
            time: msg.payload.scheduleData.time,
            duration: msg.payload.scheduleData.duration,
            freq: msg.payload.scheduleData.freq,
            customDays: msg.payload.scheduleData.customDays,
            skipIfSoilIsMoist: msg.payload.scheduleData.skipIfSoilIsMoist,
          });

          setTimeout(() => {
            socket._emitFromServer({
              type: "EVENT",
              topic: "zones",
              event: "updateWaySchedules",
              payload: { id: msg.payload.wayId, schedules: zone.schedules },
            });
            socket.onmessage?.({
              data: JSON.stringify({
                type: "ACK",
                id: msg.id,
                ok: true,
                error: {},
              }),
            });
          }, ackDelay);
          break;
        }
        case "removeSchedule": {
          const zone = socket.data.zones
            .flatMap((item) => item.ways)
            .find((way) => way.id === msg.payload.wayId);
          zone.schedules = zone.schedules.filter(
            (s) => s.id !== msg.payload.scheduleId,
          );

          setTimeout(() => {
            socket._emitFromServer({
              type: "EVENT",
              topic: "zones",
              event: "updateWaySchedules",
              payload: { id: msg.payload.wayId, schedules: zone.schedules },
            });
            socket.onmessage?.({
              data: JSON.stringify({
                type: "ACK",
                id: msg.id,
                ok: true,
                error: {},
              }),
            });
          }, ackDelay);
          break;
        }
        case "saveSchedule": {
          const zone = socket.data.zones
            .flatMap((item) => item.ways)
            .find((way) => way.id === msg.payload.wayId);
          const schedule = zone.schedules.find(
            (s) => s.id === msg.payload.scheduleId,
          );
          schedule.time = msg.payload.scheduleData.time;
          schedule.duration = msg.payload.scheduleData.duration;
          schedule.freq = msg.payload.scheduleData.freq;
          schedule.customDays = msg.payload.scheduleData.customDays;
          schedule.skipIfSoilIsMoist =
            msg.payload.scheduleData.skipIfSoilIsMoist;

          setTimeout(() => {
            socket._emitFromServer({
              type: "EVENT",
              topic: "zones",
              event: "updateWaySchedules",
              payload: { id: msg.payload.wayId, schedules: zone.schedules },
            });
            socket.onmessage?.({
              data: JSON.stringify({
                type: "ACK",
                id: msg.id,
                ok: true,
                error: {},
              }),
            });
          }, ackDelay);
          break;
        }
        case "syncTime": {
          socket.data.device.info.localTimeSec =
            msg.payload.timeMillisec / 1000;

          setTimeout(() => {
            socket.onmessage?.({
              data: JSON.stringify({
                type: "ACK",
                id: msg.id,
                ok: true,
                error: {},
              }),
            });
          }, ackDelay);
          break;
        }
        case "getDeviceCfg": {
          const initDeviceTxtCfg = dedent`
            [WIFI]
            access-point=JardinDuCiel-Arrosage:: ; SSID:Password

            [relays]
            modules=gpio-1
            gpio-1=GPIO-H(4), GPIO-H(14), GPIO-H(15), GPIO-H(16), GPIO-H(17), GPIO-H(18)

            [valve]
            main=gpio-1.0

            [zones]
            zones=Serre, Noémie, Aromatiques

            [Serre]
            ways=Tout(gpio-1.1)

            [Noémie]
            ways=Oscillant(gpio-1.2), Goutteurs(gpio-1.3)

            [Aromatiques]
            ways=Tout(gpio-1.4)

            [manual]
            duration=10

            [moisture]
            sensor=32
            max=60

            [flow]
            sensor=39
            max=10
          `;

          setTimeout(() => {
            socket.onmessage?.({
              data: JSON.stringify({
                type: "ACK",
                id: msg.id,
                ok: true,
                error: {},
                payload: initDeviceTxtCfg,
              }),
            });
          }, ackDelay);
          break;
        }
        case "setDeviceCfg": {
          /*console.log("** Sent config **\n\n", msg.payload.rawConfigText);*/

          setTimeout(() => {
            socket.onmessage?.({
              data: JSON.stringify({
                type: "ACK",
                id: msg.id,
                ok: true,
                error: {},
              }),
            });
            socket.close();
          }, ackDelay);
          break;
        }
      }
    },
  },
};
