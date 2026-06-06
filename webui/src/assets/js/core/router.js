export default {
  // ---------- ROUTING ----------
  // URL → parsed route → path validation → query validation → final route state

  invalidPageHash: null,
  currentQuery: null,

  routeRules: {
    pages: {
      home: {},
      schedule: {
        overlays: ["new", "edit"],
        requiredQuery: {
          new: ["way"], // optional rules per action
          edit: [],
        },
      },
      manual: {},
    },
    overlays: {
      settings: {},
    },
  },

  init() {
    // Load from URL on first load
    this.handleHash();
    // Listen to browser url navigation (includes back/forward + manual editing)
    window.addEventListener("hashchange", () => this.handleHash());
  },

  handleHash() {
    const hash = location.hash.replace("#", "");
    if (!hash) {
      console.log("Coming from /, adding #home to it");
      location.hash = "home";
      this.currentPage = "home";
      return;
    }

    console.log("Handling hash:", hash);
    const [page, actionWithQuery] = hash.split("/");
    const pageNames = ["home", "schedule", "manual", "settings"];
    // reset overlay by default
    this.overlayPage = null;

    if (!pageNames.includes(page)) {
      console.log("Page not found");
      this.currentPage = "invalid";
      this.invalidPageHash = hash;
    } else {
      console.log("Page found");
      switch (page) {
        case "home":
          this.currentPage = page;
          break;

        case "manual":
          this.currentPage = page;
          break;

        case "schedule":
          this.currentPage = page;
          if (actionWithQuery !== undefined) {
            const [action, query] = actionWithQuery.split("?");
            this.currentQuery = query;
            if (action === "new") {
              this.overlayPage = "new-schedule";
            } else if (action === "edit") {
              this.overlayPage = "edit-schedule";
            } else {
              this.currentPage = "invalid";
              this.currentQuery = null;
              this.invalidPageHash = hash;
            }
          }
          break;

        case "settings":
          this.overlayPage = page;
          break;
      }
      if (this.overlayPage === null) {
        document.querySelectorAll(".nav-link").forEach((link) => {
          const pageLink = link.getAttribute("href").replace("#", "");
          const isActive = page === pageLink;
          link.classList.toggle("active", isActive);
        });
      }
    }

    window.document.title = "Arrosage JdC - " + this.pageTitle();
  },

  parseRoute(hash) {
    const clean = hash.replace(/^#/, "");
    const [path, queryString] = clean.split("?");

    const parts = path.split("/");

    return {
      page: parts[0],
      action: parts[1] || null,
      id: parts[2] || null,
      query: Object.fromEntries(new URLSearchParams(queryString || "")),
    };
  },

  validatePath(route) {
    const rule = routeRules[route.page];

    if (!rule) return { valid: false, reason: "unknown_page" };

    // schedule special cases
    if (route.page === "schedule") {
      if (route.action && !rule.overlays.includes(route.action)) {
        return { valid: false, reason: "invalid_schedule_action" };
      }

      if (route.action === "edit" && !route.id) {
        return { valid: false, reason: "missing_id" };
      }
    }

    return { valid: true };
  },

  validateQuery(route) {
    const rule = routeRules[route.page];

    if (!rule) return { valid: false, reason: "unknown_page" };

    const required = rule.requiredQuery?.[route.action] || [];

    for (const key of required) {
      if (!(key in route.query)) {
        return {
          valid: false,
          reason: "missing_query_param",
          missing: key,
        };
      }
    }

    return { valid: true };
  },

  handleHash2() {
    const hash = location.hash || "#home";

    const route = this.parseRoute(hash);

    const pathCheck = this.validatePath(route);
    if (!pathCheck.valid) {
      this.route = route;
      this.currentPage = "invalid";
      this.invalidReason = pathCheck.reason;
      return;
    }

    const queryCheck = this.validateQuery(route);
    if (!queryCheck.valid) {
      this.route = route;
      this.currentPage = "invalid";
      this.invalidReason = queryCheck.reason;
      this.invalidDetails = queryCheck;
      return;
    }

    // ✅ valid route
    this.route = route;
    this.currentPage = route.page;
    this.overlayPage =
      route.page === "schedule"
        ? route.action === "new"
          ? "new-schedule"
          : route.action === "edit"
            ? "edit-schedule"
            : null
        : route.page === "settings"
          ? "settings"
          : null;

    this.currentId = route.id || null;
  },

  // ---------- APP STATE ----------
  currentPage: "",
  overlayPage: "", // null | 'settings'

  // ---------- UI HELPERS ----------
  openSettings() {
    location.hash = "settings";
  },
  closeSettings() {
    location.hash = this.currentPage;
  },
  pageTitle() {
    const capitalize = (str) => str.charAt(0).toUpperCase() + str.slice(1);

    if (this.currentPage === "schedule") {
      if (this.overlayPage === "new-schedule")
        return "Add " + capitalize(this.currentPage);
      if (this.overlayPage === "edit-schedule")
        return "Edit " + capitalize(this.currentPage);
    }

    if (this.currentPage === "invalid") {
      return "Page Not Found";
    }

    if (this.overlayPage === "settings") {
      return capitalize(this.overlayPage);
    }
    return capitalize(this.currentPage);
  },

  get pageTitle2() {
    if (this.overlayPage === "settings") return "Settings";

    switch (this.currentPage) {
      case "home":
        return "Home";
      case "schedule":
        return "Schedule";
      case "manual":
        return "Manual";
      case "history":
        return "History";
      default:
        return "App";
    }
  },
};
