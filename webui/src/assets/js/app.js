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

import registerBanner from "./components/banner.js";
import registerDeviceSystemCard from "./components/deviceSystemCard.js";
import registerManualWateringDurationInput from "./components/manualWateringDurationInput.js";
import registerSettingsPage from "./components/settingsPage.js";
import registerThemeManager from "./components/themeManager.js";
import registerToast from "./components/toast.js";
import registerTooltip from "./components/tooltip.js";
import registerWateringCard from "./components/wateringCard.js";
import registerWaterTankCard from "./components/waterTankCard.js";

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
  registerBanner(Alpine);
  registerDeviceSystemCard(Alpine);
  registerManualWateringDurationInput(Alpine);
  registerSettingsPage(Alpine);
  registerThemeManager(Alpine);
  registerToast(Alpine);
  registerTooltip(Alpine);
  registerWateringCard(Alpine);
  registerWaterTankCard(Alpine);
});
