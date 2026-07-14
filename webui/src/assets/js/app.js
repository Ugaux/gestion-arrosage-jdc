import { AppCfg } from "./core/appCfg.js";
import { formatDuration } from "./core/formatting.js";
import registerNavigation from "./core/navigation.js";
import registerWebsocketClient from "./core/wsClient.js";
import registerInternalClock from "./core/internalClock.js";

import registerDeviceCfg from "./stores/deviceCfgStore.js";
import registerDeviceInfo from "./stores/deviceInfoStore.js";
import registerHealth from "./stores/deviceHealthStore.js";
import registerSensors from "./stores/sensorStore.js";
import registerUI from "./stores/uiStore.js";
import registerValves from "./stores/valveStore.js";
import registerWatering from "./stores/wateringStore.js";
import registerZones from "./stores/zoneStore.js";

import defineXIcon from "./components/x-icon.js";
import registerBanner from "./components/banner.js";
import registerHomePage from "./components/homePage.js";
import registerManualWateringDurationInput from "./components/manualWateringDurationInput.js";
import registerModifySchedulePage from "./components/modifySchedulePage.js";
import registerManualPage from "./components/manualPage.js";
import registerSchedulePage from "./components/schedulePage.js";
import registerSettingsPage from "./components/settingsPage.js";
import {
  setTheme,
  getInitialDarkMode,
  registerThemeManager,
} from "./components/themeManager.js";
import registerToast from "./components/toast.js";
import registerTooltip from "./components/tooltip.js";

import Tash from "./plugins/alpinejs-tash@1.2.1.esm.min.js";
import Mask from "./plugins/alpinejs-mask@0.1.0.esm.js";
import PineconeRouter from "./plugins/pinecone-router@7.5.2.esm.min.js";
import Alpine from "./plugins/alpinejs@3.15.12.esm.js";

setTheme(getInitialDarkMode());

// --------- PLUGINS ---------
Alpine.plugin(Tash);
Alpine.plugin(Mask);
Alpine.plugin(PineconeRouter);

// ------------ CORE ------------
window.formatDuration = formatDuration;
registerNavigation(Alpine);
registerWebsocketClient(Alpine);
registerInternalClock(Alpine);

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
defineXIcon();
registerBanner(Alpine);
registerHomePage(Alpine);
registerManualWateringDurationInput(Alpine);
registerModifySchedulePage(Alpine);
registerManualPage(Alpine);
registerSchedulePage(Alpine);
registerSettingsPage(Alpine);
registerThemeManager(Alpine);
registerToast(Alpine);
registerTooltip(Alpine);

await Alpine.store("wsClient").ready;

if (AppCfg.debugAlpine) window.Alpine = Alpine;
Alpine.start();
