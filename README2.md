repo/
├── .vscode/
│ ├── tasks.json
│ └── launch.json
│
├── firmware/
│ ├── .pio/
│ ├── platformio.ini
│ ├── src/
│ ├── data/
│ └── scripts/
│
├── webui/
│ ├── src/
│ └── dist/
│
├── server/
│
└── README.md

check si upload va aussi faire un build avec ma task custom "upload" -> en fct ajouter task build ou non dans full deploy
(pio run -e ESP32-release --target upload)

test debuggage avec point d'arrêt dans script python du bridge

#

clear data of modals when closing them or for stuff that does not need to stay in the dom

#

Subtle hint to user that click has been registered (no complicated animation required)

```html
<button
  @click="toggleValve"
  :class="{
    'w3-green': state.valveOpen,
    'w3-red': !state.valveOpen,
    'pending': state.valvePending
  }"
></button>
```

```css
.pending {
  opacity: 0.7;
}
```

#

State.valvepending pour éviter spams ou concurrency dans fetch API après appuie bouton/toggle + state machine pour gérer la déconnexion de lesp ou server python

```js
state = {
wsConnected: false,
apiReachable: false,
LastHeartbeat:...
}
```

```js
connected = websocketHealthy && recentHeartbeat && noRecentApiFailures;
```

CONNECTED_IDLE
CONNECTED_PENDING
DISCONNECTED_UNKNOWN
RECONNECTING
RESYNCING

Cmd de l'API :

```json
{
  "commandId": 123,
  "action": "open_valve"
}
```

Réponse par websocket :

```json
{
  "type": "command_ack",
  "commandId": 123,
  "success": true
}
```

#

Toggle valve()

```js
async function toggleValve() {
  if (state.valvePending) return;

  state.valvePending = true;

  try {
    await fetch("/api/valve/toggle", {
      method: "POST",
    });
  } finally {
    state.valvePending = false;
  }
}
```
