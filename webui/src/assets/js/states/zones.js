export default {
  count: 0,

  increment() {
    this.count++;
  },

  items: [
    {
      id: 1,
      name: "Lawn",
      ways: [
        {
          id: 1,
          name: "Lawn.House",
          shortName: "House",
          manual: { started: false, duration: 10 },
          schedules: [
            {
              index: 1,
              enabled: false,
              time: "06:00",
              duration: "10",
              freq: "even days",
            },
          ],
        },
        {
          id: 2,
          name: "Lawn.Studio",
          shortName: "Studio",
          manual: { started: false, duration: 10 },
          schedules: [
            {
              index: 1,
              enabled: true,
              time: "06:00",
              duration: "10",
              freq: "even days",
            },
            {
              index: 2,
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
      id: 2,
      name: "Dripline",
      ways: [
        {
          id: 1,
          name: "Dripline.Flowers",
          shortName: "Flowers",
          manual: { started: true, duration: 8 },
          schedules: [
            {
              index: 1,
              enabled: true,
              time: "06:00",
              duration: "10",
              freq: "even days",
            },
          ],
        },
        {
          id: 2,
          name: "Dripline.Vegetables",
          shortName: "Vegetables",
          manual: { started: false, duration: 10 },
          schedules: [
            {
              index: 1,
              enabled: true,
              time: "06:00",
              duration: "10",
              freq: "even days",
            },
          ],
        },
      ],
    },
  ],
};
