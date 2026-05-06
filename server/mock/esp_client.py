from random import choice, randint

from .config import ESP_BASE_URL

# import requests


def get_device_status():
    # r = requests.get(f"{ESP_BASE_URL}/api/status", timeout=1)
    # return r.json()
    return


def send_command(device: str, payload: dict):
    # r = requests.post(f"{ESP_BASE_URL}/api/{device}/command", json=payload, timeout=1)
    # return r.json()
    return


def mock_snapshot():
    return {
        "wifi": {"ssid": "ESP32Mocked", "ip": "192.168.1.42", "rssi": randint(-90, -30)},
        "valve": {"duration": 15, "openTime": 20},
        "relays": mock_relays(),
        "zones": mock_zones(),
    }


def mock_relays():
    return [
        {"n": 1, "name": "gpio-1.1", "state": choice(["ON", "OFF"])},
        {"n": 2, "name": "gpio-1.4", "state": choice(["ON", "OFF"])},
        {"n": 3, "name": "gpio-1.7", "state": choice(["ON", "OFF"])},
        {"n": 4, "name": "gpio-1.8", "state": choice(["ON", "OFF"])},
        {"n": 5, "name": "gpio-1.12", "state": choice(["ON", "OFF"])},
    ]


def mock_zones():
    return [
        {
            "name": "Lawn",
            "ways": [
                {
                    "name": "Way1",
                    "shortName": "Front lawn",
                    "manual": {
                        "started": False,
                        "duration": 10,
                    },
                    "nextLabel": "",
                    "waterings": [
                        {
                            "index": 0,
                            "enabled": True,
                            "time": "08:00",
                            "duration": 15,
                            "freq": "daily",
                        },
                        {
                            "index": 1,
                            "enabled": False,
                            "time": "18:00",
                            "duration": 20,
                            "freq": "weekly",
                        },
                    ],
                }
            ],
        }
    ]


def mock_zones2():
    return [
        {"name": "1", "ways": [mock_way("way1", "Front Yard"), mock_way("way2", "Back Yard")]},
        {"name": "2", "ways": [mock_way("way3", "Garden")]},
    ]


def mock_way(name, short_name):
    return {
        "name": name,
        "shortName": short_name,
        "manualRunning": choice([True, False]),
        "schedules": [mock_schedule(1), mock_schedule(2)],
    }


def mock_schedule(index):
    return {
        "index": index,
        "start": f"{randint(0,23):02}:{choice(['00','15','30','45'])}",
        "duration": choice([5, 10, 15, 20]),
        "frequency": choice(["Daily", "Even", "Odd", "Custom"]),
        "enabled": choice([True, False]),
    }
