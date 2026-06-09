export default {
  items: [],

  setItems(zones) {
    this.items = zones;
    this.reconcileManualDurations();
  },

  init() {
    this.setItems([
      {
        id: 12364598,
        name: "Lawn",
        ways: [
          {
            id: 45621123,
            name: "Lawn.House",
            shortName: "House",
            manual: { started: false, timeLeft: 0 },
            schedules: [
              {
                id: 45621124,
                enabled: false,
                time: "06:00",
                duration: "10",
                freq: "even days",
              },
            ],
          },
          {
            id: 74182539,
            name: "Lawn.Studio",
            shortName: "Studio",
            manual: { started: false, timeLeft: 0 },
            schedules: [
              {
                id: 74182540,
                enabled: true,
                time: "06:00",
                duration: "10",
                freq: "even days",
              },
              {
                id: 74182541,
                enabled: false,
                time: "23:00",
                duration: "13",
                freq: "odd days",
              },
            ],
          },
        ],
      },
      {
        id: 92694589,
        name: "Dripline",
        ways: [
          {
            id: 95311135,
            name: "Dripline.Flowers",
            shortName: "Flowers",
            manual: { started: true, timeLeft: 129 },
            schedules: [
              {
                id: 95311136,
                enabled: true,
                time: "06:00",
                duration: "10",
                freq: "even days",
              },
            ],
          },
          {
            id: 36985896,
            name: "Dripline.Vegetables",
            shortName: "Vegetables",
            manual: { started: false, timeLeft: 0 },
            schedules: [
              {
                id: 36985897,
                enabled: true,
                time: "06:00",
                duration: "10",
                freq: "even days",
              },
            ],
          },
        ],
      },
    ]);
  },

  reconcileManualDurations() {
    const activeIds = new Set(
      this.items.flatMap((z) => z.ways.map((w) => String(w.id))),
    );
    //console.log("active durations:", activeIds);

    const stored = JSON.parse(
      localStorage.getItem("wateringDurations") || "{}",
    );
    //console.log("stored durations:", stored);

    const cleaned = Object.fromEntries(
      Object.entries(stored).filter(([id]) => activeIds.has(id)),
    ); // reconciliate stored entries with active zones
    //console.log("cleaned durations:", cleaned);

    localStorage.setItem("wateringDurations", JSON.stringify(cleaned));
  },
};
