const DEBUG = false;

function registerRoutes(routes, Alpine) {
  for (const route of routes) {
    Alpine.$router.add(route.path, {
      name: route.name,
      // templates: null, // fixes bug in PineconeRouter 7.5.0/7.5.1 where "pinecone:end" event never triggers
      handlers: [
        (ctx) => {
          Alpine.store("navigation").navigate({
            ...route,
            params: route.params?.(ctx),
          });
        },
      ],
    });
  }
}

export default (Alpine) => {
  Alpine.store("navigation", {
    path: null,
    type: null,
    name: null,
    parentPath: null,
    title: null,
    params: {},

    lastNavPath: null,

    routes: [
      {
        path: "/",
        type: "page",
        name: "home",
        title: "Home",
        isNav: true,
        icon: "regular-house",
      },

      {
        path: "/schedule",
        type: "page",
        name: "schedule",
        title: "Schedule",
        isNav: true,
        icon: "regular-calendar",
      },

      {
        path: "/manual",
        type: "page",
        name: "manual",
        title: "Manual",
        isNav: true,
        icon: "regular-circle-play",
      },

      {
        path: "/history",
        type: "page",
        name: "history",
        title: "History",
        isNav: true,
        hidden: true,
        icon: "regular-clock",
      },

      {
        path: "/schedule/add",
        type: "overlay",
        name: "add-schedule",
        parentPath: "/schedule",
        title: "Add Schedule",
      },

      {
        path: "/schedule/edit/:id",
        type: "overlay",
        name: "edit-schedule",
        parentPath: "/schedule",
        title: "Edit Schedule",
        params: (ctx) => ({
          id: ctx.params.id,
        }),
      },

      {
        path: "/settings",
        type: "overlay",
        name: "settings",
        // no parentPath as it is a global overlay
        title: "Settings",
      },

      {
        path: "notfound",
        type: "page",
        name: "notfound",
        title: "Page Not Found",
        params: (ctx) => ({
          notFoundPath: ctx.path,
        }),
      },
    ],

    init() {
      registerRoutes(this.routes, Alpine);

      document.addEventListener("pinecone:end", () => {
        document.title = "Arrosage JdC - " + this.title;
      });
    },

    navigate(route) {
      this.path = route.path ?? null;
      this.type = route.type ?? null;
      this.name = route.name ?? null;
      this.parentPath = route.parentPath ?? null;
      this.title = route.title ?? null;
      this.params = route.params ?? {};

      if (route.isNav) this.lastNavPath = this.path;

      // Ensure navigation ends in content-focused state
      // (includes navigation to the current route and browser back/forward).
      // On mobile this closes the sidebar; on desktop it has no visual effect.
      Alpine.store("ui").sidebar.close();
    },

    closeOverlay() {
      if (this.type !== "overlay") return;

      // Overlay has a specific parent
      if (this.parentPath) {
        if (DEBUG)
          console.log(`Overlay has a specific parent '${this.parentPath}'`);
        Alpine.$router.navigate(this.parentPath);
        return;
      }

      // Previous navigation exists
      if (this.lastNavPath) {
        if (DEBUG)
          console.log(
            `Previous navigation exists, using last path '${this.lastNavPath}'`,
          );
        Alpine.$router.navigate(this.lastNavPath);
        return;
      }

      // No previous navigation (first page load)
      if (DEBUG) console.log("No previous navigation: '/'");
      Alpine.$router.navigate("/");
    },
  });

  Alpine.magic("navigation", () => Alpine.store("navigation"));
};
