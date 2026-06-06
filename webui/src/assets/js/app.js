import router from "./core/router.js";
import ws from "./core/ws.js";

import ui from "./states/ui.js";
import theme from "./states/theme.js";
import zones from "./states/zones.js";
import relays from "./states/relays.js";

import durationControl from "./components/durationControl.js";

export const UI_VERSION = "1.4.2-" + Date.now(); //Web UIs on ESP32 often get aggressively cached. Version helps force reloads, /assets/app.js?v=1.7.0

document.addEventListener("alpine:init", () => {
  // ------------ CORE ------------
  Alpine.store("router", router);
  Alpine.store("ws", ws);

  // ----------- STATES -----------
  Alpine.store("ui", ui);
  Alpine.store("theme", theme);
  Alpine.store("zones", zones);
  Alpine.store("relays", relays);

  // --------- COMPONENTS ---------
  Alpine.data("durationControl", durationControl);
});
