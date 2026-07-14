function registerRoutes(routes, Alpine) {
  for (const route of routes) {
    Alpine.$router.add(route.path, {
      name: route.name,
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

function buildPath(path, params = {}) {
  return path.replace(/:([a-zA-Z0-9]+)/g, (_, key) => {
    if (!(key in params)) {
      throw new TypeError(`Missing param: ${key}`);
    }
    return params[key];
  });
}

export default (Alpine) => {
  Alpine.store("navigation", {
    path: null,
    type: null,
    name: null,
    title: null,
    params: {},

    lastNavigatableName: null,
    parentRoute: null,

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
        path: "/ways/schedules",
        type: "page",
        name: "schedule",
        title: "Schedule",
        isNav: true,
        icon: "regular-calendar",
      },

      {
        path: "/ways/manual",
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
        path: "/ways/:wayId/schedules/add",
        type: "overlay",
        name: "add-schedule",
        parentName: "schedule",
        title: "Add Schedule",
        params: (ctx) => ({
          action: "add",
          wayId: ctx.params.wayId,
        }),
      },

      {
        path: "/ways/:wayId/schedules/:scheduleId/edit",
        type: "overlay",
        name: "edit-schedule",
        parentName: "schedule",
        title: "Edit Schedule",
        params: (ctx) => ({
          action: "edit",
          wayId: ctx.params.wayId,
          scheduleId: ctx.params.scheduleId,
        }),
      },

      {
        path: "/settings",
        type: "overlay",
        name: "settings",
        parentName: "*",
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
        let modifyScheduleName = "";
        if (this.params.wayId) {
          modifyScheduleName += this.params.scheduleId
            ? " " + this.getScheduleName(true) + " of "
            : " to ";
          modifyScheduleName +=
            this.getWayName() + " Way in " + this.getZoneName() + " Zone";
        }
        document.title = this.title + modifyScheduleName + " — Arrosage JdC";
      });

      window.addEventListener("keydown", (e) => {
        if (e.key === "Escape") {
          this.closeOverlay();
        }
      });
    },

    navigate(route) {
      this.path = route.path ?? null;
      this.type = route.type ?? null;
      this.name = route.name ?? null;
      this.title = route.title ?? null;
      this.params = route.params ?? {};

      if (route.isNav) this.lastNavigatableName = this.name;
      this.parentRoute = this._resolveParentRoute(route.parentName);

      // Ensure navigation ends in content-focused state
      // (includes navigation to the current route and browser back/forward).
      // On mobile this closes the sidebar; on desktop it has no visual effect.
      Alpine.store("ui").sidebar.close();
    },

    closeOverlay() {
      if (this.type !== "overlay") return;

      Alpine.$router.navigate(this.parentRoute?.path || "/");
    },

    push(name, params) {
      const route = this.routes.find((route) => route.name === name);
      if (!route) throw new Error(`Unknown route: ${name}`);

      return buildPath(route.path, params);
    },

    _resolveParentRoute(parentName) {
      if (parentName === "*") {
        const lastName =
          this.lastNavigatableName ??
          this.routes.find((route) => route.path === "/").name;

        return this.routes.find((route) => route.name === lastName);
      }

      return this.routes.find((route) => route.name === parentName);
    },

    getTitle() {
      if (!Alpine.store("ui").isDesktop || this.type === "page")
        return this.title;

      if (this.type === "overlay") return this.parentRoute.title;
    },

    show(type, name) {
      // Show page/overlay from url
      if (type === this.type && name === this.name) return true;

      if (!Alpine.store("ui").isDesktop) return;
      if (type === "page" && this.type === "overlay") {
        // If on large screens and an overlay is displayed, also show its parent page
        if (name === this.parentRoute.name) {
          return true;
        }
      }

      return false;
    },

    showToolbarActions() {
      return Alpine.store("ui").isDesktop || this.type === "page";
    },

    getZoneName() {
      if (!this.params.wayId) return;

      return Alpine.store("zones").getParentZone(this.params.wayId)?.name;
    },

    getWayName() {
      if (!this.params.wayId) return;

      return Alpine.store("zones").getWay(this.params.wayId)?.shortName;
    },

    getScheduleName(useApostrophe = false) {
      if (!this.params.scheduleId) return;

      const way = Alpine.store("zones").getWay(this.params.wayId);
      if (!way) return;

      let foundSchedule = null;
      let foundIndex = -1;
      way?.schedules?.some((schedule, index) => {
        if (schedule.id === this.params.scheduleId) {
          foundSchedule = schedule;
          foundIndex = index;
          return true;
        }
        return false;
      });

      if (!foundSchedule?.name || foundSchedule?.name.trim() === "")
        return foundIndex + 1;

      return useApostrophe
        ? `'${foundSchedule.name.trim()}'`
        : foundSchedule.name.trim();
    },
  });

  Alpine.magic("navigation", () => Alpine.store("navigation"));
};
