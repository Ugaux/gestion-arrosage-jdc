export default () => ({
  // temporary typing state
  duration: null,
  min: 5,
  max: 40,

  init() {
    // initialize from store OR fallback
    this.duration = 15; // Alpine.$persist(1).as("duration_" + way.name),
  },

  setDuration(value) {
    const v = Number(value);
    const clamped = isNaN(v)
      ? this.min
      : Math.max(this.min, Math.min(this.max, v));

    console.log(clamped);
    this.duration = clamped;
  },

  increase() {
    this.setDuration(this.duration + 5);
  },

  decrease() {
    this.setDuration(this.duration - 5);
  },
});
