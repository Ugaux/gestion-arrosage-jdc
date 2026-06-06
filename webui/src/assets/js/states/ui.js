export default {
  sidebar: {
    isOpen: false,

    open() {
      this.isOpen = true;
    },

    close() {
      this.isOpen = false;
    },
  },

  sidebarOpen: false,
  currentPage: "home",
  overlayPage: null,

  init() {
    // Add closing of sidebar with escape key
    window.addEventListener("keydown", (e) => {
      if (e.key === "Escape") {
        this.sidebar.close();
      }
    });

    window.matchMedia("(min-width: 993px)").addEventListener("change", (e) => {
      // Solves the case where sidebar would come back open when it was first opened,
      // then window was resized to a large screen then back to a small one
      if (e.matches) {
        // Entering large screen
        if (this.sidebar.isOpen) {
          console.log("Closing small/medium screens sidebar...");
          this.sidebar.close();
        }
      }
    });

    this.sidebar.close(); //TODO: here
  },
};
