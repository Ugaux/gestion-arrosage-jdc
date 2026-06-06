1. 🧾 Snapshot message (GROUPED — very good use case)

Send everything that represents the current UI state:

```JSON
{
    "update": "snapshot",
    "time": "...",
    "sensors": {...},
    "watering_status": "...",
    "next_watering": "...",
    "manual": {...}
}
```

👉 This is ideal for:

- page load
- reconnect
- periodic refresh (e.g. every 30–60s)

2. ⚡ Event messages (keep separate)

Keep these as small targeted updates:

- relay toggle
- manual start/stop
- enable button
- sensor spike (optional)

Example:

```JSON
{ "update": "test_relay", "relay": 1, "state": 0 }
```
