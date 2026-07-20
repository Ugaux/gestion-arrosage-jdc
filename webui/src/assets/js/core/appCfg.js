export const AppCfg = {
  /* General websocket settings */
  verboseWebsocket: false,
  websocketAckTimeout: 500, // ms before a pending command times out

  /* Only fake socket settings */
  fakeScenario: "scheduledWatering", // default is 'empty'
  fakeDataLocalTimeSec: 1768909962,

  /* Only real socket settings */
  websocketURL: `ws://${location.hostname}:8000/ws`,

  debugAlpine: false,

  icons: {
    info: "circle-info",
    success: "circle-check",
    warning: "triangle-exclamation",
    error: "circle-xmark",
  },
};
