export default {
  dark: false,

  init() {
    const saved = localStorage.getItem("theme");
    this.dark =
      saved === "dark" ||
      (!saved && window.matchMedia("(prefers-color-scheme: dark)").matches);

    this.apply();
  },

  toggle() {
    this.dark = !this.dark;
    this.apply();
  },

  apply() {
    console.log("Darkmode set to", this.dark);
    localStorage.setItem("theme", this.dark ? "dark" : "light");
    document.body.classList.toggle("dark-theme", this.dark);
  },
};
