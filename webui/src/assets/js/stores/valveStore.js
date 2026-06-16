import { AppCfg } from "../core/appCfg.js";

export default (Alpine) => {
  Alpine.store("valves", {
    items: [],

    init() {
      if (AppCfg.debug) {
        this.items = [
          { index: 1, name: "Expander-GPIO:0", is_checked: false },
          { index: 2, name: "Expander-GPIO:1", is_checked: false },
          { index: 3, name: "Expander-GPIO:2", is_checked: true },
          { index: 4, name: "Expander-GPIO:3", is_checked: false },
          { index: 5, name: "Expander-GPIO:4", is_checked: false },
          { index: 6, name: "Expander-GPIO:5", is_checked: true },
          { index: 7, name: "Expander-GPIO:6", is_checked: false },
          { index: 8, name: "Expander-GPIO:7", is_checked: false },
        ];
      }
    },
  });
};
