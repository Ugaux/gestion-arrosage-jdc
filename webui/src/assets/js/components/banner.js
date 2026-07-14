/* Code inspired by https://pinemix.com/components/banner for logic and https://daisyui.com/components/alert/ for looks */

import { AppCfg } from "../core/appCfg.js";
import { formatDuration } from "../core/formatting.js";

function setBannerState(banner, Alpine) {
  // priority logic lives here

  const wsClient = Alpine.store("wsClient");
  if (!wsClient.isConnected()) {
    banner.show(
      "Device unreachable, all actions are disabled (" +
        formatDuration(Math.round(wsClient.disconnectedDuration)) +
        " ago).",
      {
        type: "warning",
        dismissible: false,
      },
    );
    return;
  }

  const health = Alpine.store("deviceHealth");
  if (health.state === 2) {
    const allFaults = health.faults;
    const numberOfFaults = allFaults.length;
    const firstFault = allFaults[0];
    banner.show(
      "Total of " +
        numberOfFaults +
        ": " +
        firstFault.text +
        " (" +
        formatDuration(
          Alpine.store("deviceInfo").localTimeSec - firstFault.timeStamp,
        ) +
        " ago).",
      {
        type: "error",
        action: () =>
          Alpine.store("wsClient").sendExclusive(
            `clearFault:${firstFault.id}`,
            {
              action: "clearFault",
              id: firstFault.id,
            },
            { showToast: true },
          ),
        actionText: "Clear",
        dismissible: false,
      },
    );
    return;
  }

  banner.close();
}

export default (Alpine) => {
  Alpine.store("banner", {
    open: false,
    type: "",
    content: "",
    action: null,
    actionText: "",
    link: "",
    linkText: "",
    dismissible: false,

    show(content, options = {}) {
      this.content = content;
      this.type = options.type || "info"; // info, success, warning or error
      this.actionText = options.actionText || "Start action";
      this.action = options.action || null;
      this.linkText = options.linkText || "Go to link";
      this.link = options.link || "";
      this.dismissible = options.dismissible || false;
      this.open = true;
    },

    close() {
      this.open = false;
      this.action = null;
    },
  });

  Alpine.magic("banner", () => Alpine.store("banner"));

  Alpine.directive("banner", (el) => {
    let lastType = null;
    let lastAction = null;
    let lastActionText = null;
    let lastLink = null;
    let lastLinkText = null;

    Alpine.effect(() => {
      const b = Alpine.store("banner");

      setBannerState(b, Alpine);

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

      const actionEl = el.querySelector("[data-action]");
      if (b.action) {
        actionEl.style.display = "inline-block";
        if (b.action !== lastAction) {
          actionEl.onclick = b.action;
          lastAction = b.action;
        }
        if (b.actionText !== lastActionText) {
          actionEl.textContent = b.actionText;
          lastActionText = b.actionText;
        }
      } else {
        actionEl.style.display = "none";
        actionEl.onclick = null;
        lastAction = null;
      }

      const linkEl = el.querySelector("[data-link]");
      if (b.link) {
        linkEl.style.display = "inline-block";
        if (b.link !== lastLink) {
          linkEl.href = b.link === "#" ? "javascript:void(0)" : b.link;
          lastLink = b.link;
        }
        if (b.linkText !== lastLinkText) {
          linkEl.textContent = b.linkText;
          lastLinkText = b.linkText;
        }
      } else linkEl.style.display = "none";

      const closeEl = el.querySelector("[data-close]");
      if (closeEl) {
        closeEl.style.display = b.dismissible ? "flex" : "none";
      }
    });
  });
};
