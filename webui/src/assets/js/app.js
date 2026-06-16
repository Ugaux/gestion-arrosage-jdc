import { formatDuration } from "./core/formatting.js";
window.formatDuration = formatDuration;

import registerRouter from "./core/router.js";
import registerWebsocket from "./core/ws.js";

import registerDeviceCfg from "./stores/deviceCfgStore.js";
import registerDeviceInfo from "./stores/deviceInfoStore.js";
import registerHealth from "./stores/healthStore.js";
import registerSensors from "./stores/sensorStore.js";
import registerUI from "./stores/uiStore.js";
import registerValves from "./stores/valveStore.js";
import registerWatering from "./stores/wateringStore.js";
import registerZones from "./stores/zoneStore.js";

import themeHandler from "./components/themeHandler.js";
import manualDurationController from "./components/manualDurationController.js";
import tooltip from "./components/tooltip.js";
import registerBanner from "./components/banner.js";

document.addEventListener("alpine:init", () => {
  // ------------ CORE ------------
  registerRouter(Alpine);
  registerWebsocket(Alpine);

  // ----------- STORES -----------
  registerDeviceCfg(Alpine);
  registerDeviceInfo(Alpine);
  registerHealth(Alpine);
  registerSensors(Alpine);
  registerUI(Alpine);
  registerValves(Alpine);
  registerWatering(Alpine);
  registerZones(Alpine);

  // --------- COMPONENTS ---------
  Alpine.data("themeHandler", themeHandler);
  Alpine.data("manualDurationController", manualDurationController);
  Alpine.directive("tooltip", tooltip);
  registerBanner(Alpine);
});
