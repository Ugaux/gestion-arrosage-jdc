import router from "./core/router.js";
import ws from "./core/ws.js";

import ui from "./states/ui.js";
import zones from "./states/zones.js";
import valves from "./states/valves.js";
import esp32cfg from "./states/esp32cfg.js";

import themeHandler from "./components/themeHandler.js";
import manualDurationController from "./components/manualDurationController.js";

document.addEventListener("alpine:init", () => {
  // ------------ CORE ------------
  Alpine.store("router", router);
  Alpine.store("ws", ws);

  // ----------- STATES -----------
  Alpine.store("ui", ui);
  Alpine.store("zones", zones);
  Alpine.store("valves", valves);
  Alpine.store("esp32cfg", esp32cfg);

  // --------- COMPONENTS ---------
  Alpine.data("themeHandler", themeHandler);
  Alpine.data("manualDurationController", manualDurationController);
});
