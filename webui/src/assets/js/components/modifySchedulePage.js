const PRINT_DEBUG = false;

const DEFAULT_TIME = "07:00";
const DEFAULT_DURATION = 15;
const DEFAULT_FREQ = "always";
const DEFAULT_CUSTOM_DAYS = ["Mon", "Tue", "Wed", "Thu", "Fri"];
const DEFAULT_SKIP_IS_MOIST_SOIL = true;

export default (Alpine) => {
  Alpine.data("modifySchedulePage", () => ({
    wayId: null,
    scheduleId: null,
    action: null,

    reset() {
      // Reset page state
      this.time = DEFAULT_TIME;
      this.duration = DEFAULT_DURATION;
      this.selectedFreq = DEFAULT_FREQ;
      this.customDays = DEFAULT_CUSTOM_DAYS;
      this.skipIfSoilIsMoist = DEFAULT_SKIP_IS_MOIST_SOIL;
    },

    populate(params) {
      if (!params || Object.keys(params).length === 0) {
        this.action = null;
        this.wayId = null;
        this.scheduleId = null;
        return;
      }

      this.action = params.action;
      this.wayId = params.wayId;
      if (params.action === "add") {
        // Reset to default when adding a new schedule
        this.reset();
        return;
      }

      this.scheduleId = params.scheduleId;

      const way = Alpine.store("zones").getWay(params.wayId);
      if (!way) return;

      const schedule = Alpine.store("zones").getSchedule(
        way,
        params.scheduleId,
      );
      if (!schedule) return;

      this.time = schedule.time;
      this.duration = schedule.duration;
      this.selectedFreq = schedule.freq;
      this.customDays = schedule.customDays
        ? [...schedule.customDays]
        : DEFAULT_CUSTOM_DAYS;
      this.skipIfSoilIsMoist = schedule.skipIfSoilIsMoist;
    },

    init() {
      this.$watch(
        () => Alpine.store("navigation").params,
        (params) => {
          this.populate(params);
        },
      );

      this._onKeyDown = (e) => {
        if (e.key !== "Enter") return;

        // Ignore when this overlay isn't active
        const params = Alpine.store("navigation").params;
        if (!params || params.wayId !== this.wayId) return;

        const isEditing = e.target.matches(
          "input, textarea, select, [contenteditable]",
        );
        if (isEditing) return;

        e.preventDefault();
        this.update();
      };

      window.addEventListener("keydown", this._onKeyDown);
    },

    destroy() {
      window.removeEventListener("keydown", this._onKeyDown);
    },

    get isWayExisting() {
      const way = Alpine.store("zones").getWay(this.wayId);
      if (!way) return false;

      return true;
    },

    get canScheduleBeEdited() {
      const way = Alpine.store("zones").getWay(this.wayId);
      if (!way) return false;
      if (this.action === "add") return true;

      const schedule = Alpine.store("zones").getSchedule(way, this.scheduleId);
      if (!schedule) return false;
      if (this.action === "edit") return true;

      return false;
    },

    time: DEFAULT_TIME,
    formatTimeOnInput() {
      let digits = this.time.replace(/\D/g, "").slice(0, 4);

      if (digits.length <= 2) {
        let hours = digits;
        if (hours !== "") {
          hours = String(Math.min(parseInt(hours, 10), 23)).padStart(
            hours.length,
            "0",
          );
        }
        this.time = hours;
      } else {
        let hours = digits.slice(0, 2);
        let minutes = digits.slice(2);
        hours = String(Math.min(parseInt(hours, 10), 23)).padStart(2, "0");
        if (minutes !== "") {
          minutes = String(Math.min(parseInt(minutes, 10), 59)).padStart(
            minutes.length,
            "0",
          );
        }
        this.time = `${hours}:${minutes}`;
      }
    },
    formatTimeOnblur() {
      if (!this.time.trim()) {
        this.time = DEFAULT_TIME;
        return;
      }

      let [h = "", m = ""] = this.time.split(":");
      this.time = h.padStart(2, "0") + ":" + m.padStart(2, "0");
    },
    convertTimeToSec() {
      let [h = "", m = ""] = this.time.split(":");
      return parseInt(h, 10) * 3600 + parseInt(m, 10) * 60;
    },

    min: 1,
    max: 60,
    step: 5,
    duration: DEFAULT_DURATION,
    setDuration(value) {
      if (value === "") this.duration = this.defaultDuration;
      else {
        const v = Number(value);
        const clamped = isNaN(v)
          ? this.min
          : Math.max(this.min, Math.min(this.max, v));
        this.duration = Math.round(clamped);
      }

      if (PRINT_DEBUG)
        console.log("duration →", this.min, "≤", this.duration, "≤", this.max);
    },
    increaseDuration() {
      this.setDuration(this.duration + this.step, true);
    },
    decreaseDuration() {
      this.setDuration(this.duration - this.step, true);
    },

    frequencies: ["always", "even", "odd", "custom"],
    selectedFreq: DEFAULT_FREQ,
    selectFrequency(freq) {
      this.selectedFreq = freq;
    },
    formatFrequency(freq) {
      if (freq === "always") return "Always";
      if (freq === "odd") return "Odd";
      if (freq === "even") return "Even";
      if (freq === "custom") return "Custom";
      return "Unknown";
    },

    weekdays: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
    customDays: DEFAULT_CUSTOM_DAYS,
    toggleDay(day) {
      const isSelected = this.customDays.includes(day);

      // Prevent removing the last selected day
      if (isSelected && this.customDays.length === 1) {
        Alpine.store("toast").show("Forbidden action", {
          type: "info",
          description: "At least one day must be selected!",
        });
        return;
      }
      // Prevent selecting all days
      if (!isSelected && this.customDays.length === 6) {
        Alpine.store("toast").show("Forbidden action", {
          type: "info",
          description: "Select 'Always' for all days!",
        });
        return;
      }

      if (isSelected) {
        this.customDays = this.customDays.filter((d) => d !== day);
      } else {
        this.customDays.push(day);
      }
    },

    skipIfSoilIsMoist: DEFAULT_SKIP_IS_MOIST_SOIL,

    get canDelete() {
      if (!this.wayId) return;
      if (!Alpine.store("zones").getWay(this.wayId)) return;

      return (
        this.action === "edit" &&
        Alpine.store("zones").getWay(this.wayId).schedules.length > 1
      );
    },

    async update(remove = false) {
      const key = this.scheduleId ? `saveSchedule:${this.scheduleId}` : "";
      const payload = {
        action: remove
          ? "removeSchedule"
          : this.scheduleId
            ? "saveSchedule"
            : "addSchedule",
        wayId: this.wayId,
        scheduleId: this.scheduleId,
        scheduleData: remove
          ? {}
          : {
              time: this.time,
              duration: this.duration,
              freq: this.selectedFreq,
              customDays:
                this.selectedFreq === "custom" ? this.customDays : null,
              skipIfSoilIsMoist: this.skipIfSoilIsMoist,
            },
      };
      try {
        const result = await Alpine.store("wsClient").sendExclusive(
          key,
          payload,
        );
        if (!result.initiated) return;

        Alpine.store("navigation").closeOverlay();
      } catch (err) {
        console.error(err);
      }
    },
  }));
};
