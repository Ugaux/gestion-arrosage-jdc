/* Code inspired by https://pinemix.com/components/banner for logic and https://daisyui.com/components/alert/ for looks */

import { AppCfg } from "../core/appCfg.js";
import { formatDuration } from "../core/formatting.js";

function setBannerState(b) {
  // priority logic lives here

  const ws = Alpine.store("ws");
  if (!ws.server.reachable) {
    b.show(
      "Device unreachable, all actions disabled (" +
        formatDuration(
          Math.round((ws.now - ws.server.lastContactDate) / 1000),
        ) +
        " ago).",
      {
        type: "warning",
        dismissible: false,
      },
    );
    return;
  }

  const health = Alpine.store("health");
  if (health.state === 2) {
    b.show(
      "Total of " +
        health.faults.length +
        ": " +
        health.faults[0].text +
        " (" +
        formatDuration(
          Alpine.store("deviceInfo").localTimeSec - health.faults[0].timeStamp,
        ) +
        " ago).",
      {
        type: "error",
        link: "/clear-fault",
        linkText: "Clear",
        dismissible: false,
      },
    );
    return;
  }

  b.close();
}

export default (Alpine) => {
  Alpine.store("banner", {
    open: false,
    type: "",
    content: "",
    link: "",
    linkText: "",
    dismissible: false,

    show(content, options = {}) {
      this.content = content;
      this.type = options.type || "info"; // info, success, warning or error
      this.linkText = options.linkText || "Learn more";
      this.link = options.link || "";
      this.dismissible = options.dismissible || false;
      this.open = true;
    },

    close() {
      this.open = false;
    },
  });

  Alpine.magic("banner", () => Alpine.store("banner"));

  Alpine.directive("banner", (el) => {
    let lastType = null;
    let lastLink = null;
    let lastLinkText = null;

    Alpine.effect(() => {
      const b = Alpine.store("banner");

      setBannerState(b);

      el.classList.toggle("is-open", b.open);

      let textType = "";
      if (b.type === "error") textType = "Fault! ";
      else if (b.type === "warning") textType = "Warning: ";
      el.querySelector("[data-content]").textContent = textType + b.content;

      if (b.type !== lastType) {
        const icon = el.querySelector("[data-icon]");
        icon.setAttribute("name", "solid-" + AppCfg.icons[b.type]);
        lastType = b.type;
      }

      const type = el.querySelector("[data-inner]");
      type.className = "banner-inner " + b.type + "-state";

      const linkEl = el.querySelector("[data-link]");
      if (linkEl) {
        if (b.link) {
          linkEl.style.display = "inline-block";
          if (b.link !== lastLink) {
            linkEl.href = b.link === "#" ? "javascript:void(0)" : b.link;
            lastLink = b.link;
          }
          if (b.linkText !== lastLinkText) {
            linkEl.text = b.linkText;
            lastLinkText = b.linkText;
          }
        } else {
          linkEl.style.display = "none";
        }
      }

      const closeEl = el.querySelector("[data-close]");
      if (closeEl) {
        closeEl.style.display = b.dismissible ? "flex" : "none";
      }
    });
  });
};
