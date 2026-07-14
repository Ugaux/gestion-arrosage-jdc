export default (Alpine) => {
  Alpine.store("ui", {
    sidebar: {
      isOpen: false,

      open() {
        this.isOpen = true;
      },

      close() {
        this.isOpen = false;
      },
    },

    isDesktop: null,

    init() {
      // Add closing of sidebar with escape key
      window.addEventListener("keydown", (e) => {
        if (e.key === "Escape") {
          this.sidebar.close();
        }
      });

      const mediaQuery = window.matchMedia("(min-width: 993px)");
      document.documentElement.classList.toggle(
        "is-desktop",
        mediaQuery.matches,
      );
      this.isDesktop = mediaQuery.matches;

      mediaQuery.addEventListener("change", (e) => {
        document.documentElement.classList.toggle("is-desktop", e.matches);
        this.isDesktop = e.matches;
        if (e.matches) {
          // Entering large screen
          if (this.sidebar.isOpen) {
            // Solves the case where sidebar would come back open when it was first opened,
            // then window was resized to a large screen then back to a small one
            console.log("Closing small/medium screens sidebar...");
            this.sidebar.close();
          }

          // Scrolling to top for overlay pages on resize
          document.querySelectorAll("#overlays>section").forEach((el) => {
            if (el.offsetParent !== null) {
              el.scrollTop = 0;
            }
          });
        }
      });
    },
  });

  Alpine.magic("ui", () => Alpine.store("ui"));
};
