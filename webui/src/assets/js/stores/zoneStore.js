import manualWateringDurationService from "../services/manualWateringDurationService.js";

export default (Alpine) => {
  Alpine.store("zones", {
    items: [],

    setItems(zones) {
      this.items = zones;
      manualWateringDurationService.syncWithZones(zones);
    },

    getZone(zoneId) {
      return this.items.find((z) => z.id === zoneId);
    },

    getParentZone(wayId) {
      return this.items.find((zone) =>
        zone.ways.some((way) => way.id === wayId),
      );
    },

    getWay(wayId) {
      const way = this.items
        .flatMap((item) => item.ways)
        .find((way) => way.id === wayId);
      return way;
    },

    getSchedule(way, scheduleId) {
      const schedule = way.schedules.find((s) => s.id === scheduleId);
      return schedule;
    },
  });
};
