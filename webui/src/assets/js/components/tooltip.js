/* Code inspired by https://devdojo.com/pines/docs/tooltip in "Create a Tooltip Plugin" section */

export default (Alpine) => {
  Alpine.directive("tooltip", (el, { modifiers, expression }, { cleanup }) => {
    let tooltipArrow = modifiers.includes("noarrow") ? false : true;
    let tooltipPosition = "top";
    let tooltipId =
      "tooltip-" +
      Date.now().toString(36) +
      Math.random().toString(36).substring(2, 7);
    let positions = ["top", "bottom", "left", "right"];
    let elementPosition = getComputedStyle(el).position;

    for (let position of positions) {
      if (modifiers.includes(position)) {
        tooltipPosition = position;
        break;
      }
    }

    if (!["relative", "absolute", "fixed"].includes(elementPosition)) {
      el.style.position = "relative";
    }

    let tooltipHTML = `
      <div
          id="${tooltipId}"
          x-data="{
              tooltipVisible: false,
              get tooltipText() {
                return ${expression}
              },
              tooltipArrow: ${tooltipArrow},
              tooltipPosition: '${tooltipPosition}'
          }"
          x-ref="tooltip"
          x-init="setTimeout(() => tooltipVisible = true, 1)"
          x-show="tooltipVisible"
          :class="{
              'tooltip-top': tooltipPosition == 'top',
              'tooltip-left': tooltipPosition == 'left',
              'tooltip-bottom': tooltipPosition == 'bottom',
              'tooltip-right': tooltipPosition == 'right'
          }"
          class="tooltip"
          x-cloak
      >
          <div x-show="tooltipVisible" x-transition class="tooltip-content">
              <p x-text="tooltipText" class="tooltip-text"></p>

              <div
                  x-ref="tooltipArrow"
                  x-show="tooltipArrow"
                  :class="{
                      'tooltip-arrow-top': tooltipPosition == 'top',
                      'tooltip-arrow-left': tooltipPosition == 'left',
                      'tooltip-arrow-bottom': tooltipPosition == 'bottom',
                      'tooltip-arrow-right': tooltipPosition == 'right'
                  }"
                  class="tooltip-arrow"
              >
                  <div
                      :class="{
                          'tooltip-arrow-inner-top': tooltipPosition == 'top',
                          'tooltip-arrow-inner-left': tooltipPosition == 'left',
                          'tooltip-arrow-inner-bottom': tooltipPosition == 'bottom',
                          'tooltip-arrow-inner-right': tooltipPosition == 'right'
                      }"
                      class="tooltip-arrow-inner"
                  ></div>
              </div>
          </div>
      </div>`;

    el.dataset.tooltip = tooltipId;

    let hideTimer = null;

    function showTooltip() {
      if (document.getElementById(tooltipId)) return;

      el.insertAdjacentHTML("beforeend", tooltipHTML);
      Alpine.initTree(document.getElementById(tooltipId));
    }

    function hideTooltip() {
      clearTimeout(hideTimer);
      hideTimer = null;
      document.getElementById(tooltipId)?.remove();
    }

    function onPointerEnter(e) {
      if (e.pointerType === "mouse") showTooltip();
    }
    function onPointerLeave(e) {
      if (e.pointerType === "mouse") hideTooltip();
    }
    el.addEventListener("pointerenter", onPointerEnter);
    el.addEventListener("pointerleave", onPointerLeave);

    function onPointerDown(e) {
      if (e.pointerType === "touch") {
        clearTimeout(hideTimer);
        showTooltip();
        hideTimer = setTimeout(hideTooltip, 5000);
      }
    }
    function onGlobalPointerDown(e) {
      if (!document.getElementById(tooltipId)) return;

      if (el.contains(e.target)) return;

      hideTooltip();
    }
    el.addEventListener("pointerdown", onPointerDown);
    document.addEventListener("pointerdown", onGlobalPointerDown);

    cleanup(() => {
      hideTooltip();

      el.removeEventListener("pointerenter", onPointerEnter);
      el.removeEventListener("pointerleave", onPointerLeave);

      el.removeEventListener("pointerdown", onPointerDown);
      document.removeEventListener("pointerdown", onGlobalPointerDown);

      clearTimeout(hideTimer);
      hideTimer = null;
    });
  });
};
