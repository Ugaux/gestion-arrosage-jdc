import { AppCfg } from "./appCfg.js";
import { handleMessage } from "./wsRouter.js";
import { logStatus, generateUniqueID } from "./utilities.js";

const LOG_PREFIX = "[wsClient]";

const FIRST_RECONNECT_DELAY = 2000;
const WATCHDOG_TIMEOUT = 35000;
const TOAST_DISPLAY_DURATION = 6000;

async function createSocket() {
  if (USE_FAKE_SOCKET) {
    const { FakeSocket } = await import("./fakeSocket.js");
    logStatus(
      LOG_PREFIX,
      "Using the fake socket (intended for dev-only)",
      "warn",
    );
    return new FakeSocket(AppCfg.fakeScenario);
  }

  return new WebSocket(AppCfg.websocketURL);
}

class CommError extends Error {
  constructor(message, name = "") {
    super(message);
    this.name = name || "Communication Error";
  }
}

export default (Alpine) => {
  Alpine.store("wsClient", {
    socket: null,
    ready: null,

    status: "disconnected", // 'connected' | 'disconnected' | 'reconnecting' | 'error'
    disconnectDuration: 0,
    disconnectTimer: null,
    lastMessageAt: null,
    reconnectTimer: null,
    retryCount: 0,

    pendingMessages: new Map(), // id → { resolve, reject, timer }

    async _connect() {
      if (
        this.socket &&
        (this.socket.readyState === WebSocket.OPEN ||
          this.socket.readyState === WebSocket.CONNECTING)
      ) {
        return;
      }

      try {
        this.status = "connecting";
        logStatus(LOG_PREFIX, this.status, "info");
        const socket = await createSocket();
        this.socket = socket;

        let watchdog = null;
        const resetWatchdog = () => {
          if (USE_FAKE_SOCKET) return;
          // If we hear nothing from the ESP32 for 35 s, force a reconnect
          clearTimeout(watchdog);
          watchdog = setTimeout(() => {
            if (socket.readyState === WebSocket.OPEN) {
              logStatus(
                LOG_PREFIX,
                `no news from server for ${WATCHDOG_TIMEOUT / 1000}s, closing...`,
                "warn",
              );
              socket.close(); // IMPORTANT: closes correct socket instance
            }
          }, WATCHDOG_TIMEOUT);
        };

        this.socket.onopen = () => {
          resetWatchdog();
          clearTimeout(this.reconnectTimer);
          this.reconnectTimer = null;
          this.retryCount = 0; // reset after success
          clearInterval(this.disconnectedTimer);
          this.disconnectedTimer = null;
          this.disconnectedDuration = 0;

          this.status = "connected";
          logStatus(LOG_PREFIX, this.status);
          Alpine.store("toast").dismissByTag("device-reboot");
        };

        this.socket.onclose = (event) => {
          clearTimeout(watchdog);

          for (const p of this.pendingMessages.values()) {
            clearTimeout(p.timer);
            p.reject(
              new CommError("Connection closed before message acknowledge"),
            );
          }
          this.pendingMessages.clear();

          this.status = "disconnected";
          let reason = "";
          if (!event.wasClean) {
            switch (event.code) {
              case 1006: {
                reason = "server unreachable";
                break;
              }
              case 1008: {
                reason = "access denied";
                break;
              }
              default: {
                reason = `unexpected error ${event.code}`;
              }
            }
            reason = ` (${reason})`;
          }
          logStatus(LOG_PREFIX, `${this.status}${reason}`, "info");
          clearInterval(this.disconnectedTimer);
          this.disconnectedTimer = setInterval(() => {
            this.disconnectedDuration++;
          }, 1000);

          const delay = Math.min(
            FIRST_RECONNECT_DELAY * 2 ** this.retryCount,
            30000,
          );
          logStatus(
            LOG_PREFIX,
            `next reconnection attempt in ${delay / 1000}s`,
          );

          clearTimeout(this.reconnectTimer);
          this.reconnectTimer = setTimeout(() => {
            logStatus(
              LOG_PREFIX,
              "trying again after reconnect delay timed-out",
            );
            this._connect();
          }, delay);
          this.retryCount++;
        };

        this.socket.onerror = (event) => {
          this.status = "error";
          logStatus(
            LOG_PREFIX,
            `${this.status} ${event.code}, reason is '${event.reason}'`,
            "error",
          );

          this.socket.close();
        };

        this.socket.onmessage = (event) => {
          resetWatchdog();

          this.lastMessageAt = Date.now();
          let msg = null;
          try {
            msg = JSON.parse(event.data);
          } catch {
            return;
          } // silently drop malformed
          if (!msg?.type) return;
          handleMessage(Alpine, this, msg);
        };
      } catch (err) {
        this.status = "error";
        logStatus(LOG_PREFIX, `Failed to create socket: ${err}`, "error");
      }
    },

    init() {
      // Initial connection
      this.ready = new Promise((resolve) => {
        this._resolveReady = resolve;
      });
      this._connect();

      // Forces a reconnect when user comes back to the browser tab on mobile after having left it off
      document.addEventListener("visibilitychange", () => {
        if (!document.hidden) {
          logStatus(LOG_PREFIX, "force reconnect on user back to browser tab");
          this._connect();
        }
      });
    },

    isConnected() {
      return this.status === "connected";
    },

    markReady() {
      if (this._resolveReady) {
        this._resolveReady();
        this._resolveReady = null; // prevent resolving twice
      }
    },

    send(payload = {}) {
      // Returns a Promise that resolves/rejects when the ESP32 acks the command
      if (this.socket?.readyState !== WebSocket.OPEN)
        return Promise.reject(
          new CommError("Tried to send message but socket is not connected"),
        );

      const id = generateUniqueID();
      if (this.pendingMessages.has(id)) {
        return Promise.reject(
          new CommError("Message already exists (duplicate id)"),
        );
      }
      const msg = JSON.stringify({ id, payload });
      this.socket.send(msg);

      return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
          this.pendingMessages.delete(id);
          reject(new CommError(`Command "${payload.action}" timed-out`));
        }, AppCfg.websocketAckTimeout);
        this.pendingMessages.set(id, { resolve, reject, timer });
      });
    },

    pendingActions: new Map(),

    async sendExclusive(key, payload, options = { showToast: true }) {
      // If this action is already running, wait for the existing one.
      if (this.pendingActions.has(key)) {
        const ackPayload = await this.pendingActions.get(key);
        return {
          initiated: false, // I was not the initiator
          payload: ackPayload,
        };
      }

      const promise = (async () => {
        try {
          return await this.send(payload);
        } catch (err) {
          if (options.showToast)
            Alpine.store("toast").show(err.name ?? "Error", {
              type: "error",
              duration: TOAST_DISPLAY_DURATION,
              description: err.message ?? "",
            });
          throw err;
        } finally {
          this.pendingActions.delete(key);
        }
      })();

      this.pendingActions.set(key, promise);

      const ackPayload = await promise;
      return {
        initiated: true, // I initiated the request
        payload: ackPayload,
      };
    },

    resolveAck(msg) {
      const p = this.pendingMessages.get(msg.id);
      if (!p) return false;

      clearTimeout(p.timer);
      this.pendingMessages.delete(msg.id);
      if (msg.ok)
        p.resolve(msg.payload); // return the ACK payload
      else
        p.reject(
          new CommError(
            msg.error.message ?? "Command failed (unknown reason)",
            msg.error.name,
          ),
        );

      return true;
    },
  });

  Alpine.magic("wsClient", () => Alpine.store("wsClient"));
};
