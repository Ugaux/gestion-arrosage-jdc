/* Code inspired by https://dev.to/zaxwebs/toast-notifications-with-alpine-js-tailwind-css-lpc for logic and https://daisyui.com/components/toast/ with https://daisyui.com/components/alert/ for looks */

import { AppCfg } from "../core/appCfg.js";
import { parseCssDurationVar, generateUniqueID } from "../core/utilities.js";

const MAX_TOASTS = 4;
const POSITION = "bottom-right";
// Choices:
// "top-left"
// "top-center"
// "top-right"
// "bottom-left"
// "bottom-center"
// "bottom-right"
const DEFAULT_DURATION = 4000;

export default (Alpine) => {
  Alpine.store("toast", createToastStore());

  Alpine.data("toastContainer", () => createToastContainer(Alpine));

  Alpine.magic("toast", () => Alpine.store("toast"));

  Alpine.directive("toast", createToastDirective);
};

function createToastStore() {
  return {
    items: [],

    animationDuration: parseCssDurationVar("--toast-animation-duration"),

    show(content, options = {}) {
      const toast = {
        content,
        type: options.type || "info", // info, success, warning or error
        description: options.description ?? "",
        duration: options.duration ?? DEFAULT_DURATION,

        id: generateUniqueID(),
        leaving: false,
      };

      this.items.push(toast);

      if (this.items.length > MAX_TOASTS) {
        const firstNonLeavingItem = this.items.find((item) => !item.leaving);
        if (!firstNonLeavingItem) return;
        this.close(firstNonLeavingItem.id);
      }

      toast.timer = setTimeout(() => {
        this.close(toast.id);
      }, toast.duration);

      return toast.id;
    },

    close(id) {
      const toast = this.items.find((t) => t.id === id);
      if (!toast || toast.leaving) return;

      clearTimeout(toast.timer);
      toast.timer = null;

      toast.leaving = true;
      // ensure DOM update happens before removal
      requestAnimationFrame(() => {
        setTimeout(
          () => (this.items = this.items.filter((t) => t.id !== id)),
          this.animationDuration + 30,
        );
      });
    },

    clear() {
      for (const item of this.items) if (!item.leaving) this.close(item.id);
    },
  };
}

function createToastContainer(Alpine) {
  return {
    positionMap: {
      "top-left": "toast-top-left",
      "top-center": "toast-top-center",
      "top-right": "toast-top-right",
      "bottom-left": "toast-bottom-left",
      "bottom-center": "toast-bottom-center",
      "bottom-right": "toast-bottom-right",
    },

    get items() {
      return Alpine.store("toast").items;
    },

    close(id) {
      Alpine.store("toast").close(id);
    },

    containerClass() {
      return this.positionMap[POSITION];
    },

    toastLeaveClass() {
      return POSITION.endsWith("left")
        ? "toast-leave-left"
        : "toast-leave-right";
    },

    toastTypeClass(type) {
      return type + "-state";
    },

    positionClass() {
      return POSITION.startsWith("bottom") ? "flex-col-reverse" : "flex-col";
    },

    iconClass(type) {
      return AppCfg.icons[type];
    },
  };
}

function createToastDirective(el, { expression }, { evaluate }) {
  const toast = evaluate(expression);

  const toastEnterClass = POSITION.startsWith("bottom")
    ? "toast-enter-bottom"
    : "toast-enter-top";
  el.classList.add(toastEnterClass);

  void el.offsetWidth;

  requestAnimationFrame(() => {
    el.classList.remove(toastEnterClass);
  });

  /* 
  // OLD way of removing element from DOM, based on opacity transition ending
  // Less reliable than a timeout in close, base on CSS transition duration
  const handleTransition = (event) => {
    if (event.propertyName !== "opacity") return;
    if (toast.leaving) {
      Alpine.store("toast").remove(toast.id);
    }
  };
  el.addEventListener("transitionend", handleTransition);
  */
}
