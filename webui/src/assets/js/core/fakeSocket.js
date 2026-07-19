import { logStatus } from "./utilities.js";
import { scenarios, createSnapshot } from "./fakeScenarios.js";

const LOG_PREFIX = "[fakeSocket]";

const SNAPSHOT_DELAY = 195; // ms
const SIMULATION_STEP = 100; // ms
const SIMULATION_CMD_EXEC_DELAY = 112; // ms

export class FakeSocket {
  readyState = WebSocket.CONNECTING;

  onopen = null;
  onclose = null;
  onmessage = null;
  onerror = null;

  constructor(scenarioName = "empty") {
    this.loadScenario(scenarioName, SNAPSHOT_DELAY);

    window.FakeSocket = this; // for browser devtools
  }

  _update() {
    this.elapsedTime += SIMULATION_STEP;

    // interval updates
    this.tickFn?.(this);

    // event updates
    for (const e of this.events) {
      if (!this.executedEvents.has(e) && this.elapsedTime >= e.at) {
        e.run(this);
        this.executedEvents.add(e);
      }
    }
  }

  emitFromServer(msg) {
    this.onmessage?.({
      data: JSON.stringify(msg),
    });
  }

  every(key, interval, maxRandomInterval = null) {
    this._timers ??= {};
    this._timersRandomInterval ??= {};

    this._timers[key] = (this._timers[key] ?? 0) + SIMULATION_STEP;
    if (maxRandomInterval)
      this._timersRandomInterval[key] =
        this._timersRandomInterval[key] ??
        Math.max(interval, Math.random() * maxRandomInterval);

    const usedInterval = maxRandomInterval
      ? this._timersRandomInterval[key]
      : interval;
    if (this._timers[key] >= usedInterval) {
      this._timers[key] -= usedInterval;
      if (maxRandomInterval) {
        this._timersRandomInterval[key] = Math.max(
          interval,
          Math.random() * maxRandomInterval,
        );
      }
      return true;
    }

    return false;
  }

  close() {
    if (
      this.readyState === WebSocket.CLOSING ||
      this.readyState === WebSocket.CLOSED
    ) {
      return "Socket close skipped (already closed)";
    }

    this.readyState = WebSocket.CLOSING;
    this.haltSimulation();
    this.readyState = WebSocket.CLOSED;

    this.onclose?.({ wasClean: true });
    return "Socket closed";
  }

  loadScenario(scenarioName, delay = 0) {
    this.haltSimulation();

    logStatus(LOG_PREFIX, `loading scenario '${scenarioName}'`);
    if (!(scenarioName in scenarios)) {
      logStatus(
        LOG_PREFIX,
        `scenario '${scenarioName}' does not exist, defaulting to 'idle'`,
        "warn",
      );
    }
    const scenario = scenarios[scenarioName] ?? scenarios.idle;

    this.elapsedTime = 0;
    this.data = structuredClone(scenario.data ?? {});
    this.events = scenario.events ?? [];
    this.executedEvents = new Set();
    this.tickFn = scenario.tick ?? null;
    this.onCommand = scenario.onCommand ?? null;

    setTimeout(() => {
      this.readyState = WebSocket.OPEN;
      this.onopen?.();

      this.emitFromServer(createSnapshot(this.data));
      this.startSimulation();
    }, delay);

    return `Scenario '${scenarioName}' loaded`;
  }

  startSimulation() {
    this.haltSimulation();

    this._updateTimer = setInterval(() => {
      this._update();
    }, SIMULATION_STEP);

    return "Simulation started";
  }

  haltSimulation() {
    clearInterval(this._updateTimer);
    this._updateTimer = null;

    return "Simulation halted";
  }

  send(raw) {
    const msg = JSON.parse(raw);

    logStatus(LOG_PREFIX, `sent ${msg.payload.action} ${msg.id}`);

    // simulate CMD
    setTimeout(() => {
      this.onCommand?.(this, msg, SIMULATION_CMD_EXEC_DELAY);
    }, SIMULATION_CMD_EXEC_DELAY);
  }
}
