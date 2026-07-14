export default (Alpine) => {
  Alpine.store("internalClock", {
    tick: Date.now(),

    init() {
      setInterval(() => {
        this.tick = Date.now();
      }, 1000);
    },
  });

  Alpine.magic("internalClock", () => Alpine.store("internalClock"));
};
