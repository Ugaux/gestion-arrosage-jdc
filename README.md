# 🌱 Garden Irrigation Controller

## Project Overview

This project is an **ESP32-based irrigation controller** designed to manage the watering system of our associative garden.

The system is designed to operate **independently of the Internet and cloud services**, providing reliable local control with minimal latency and no recurring software or service costs.

## Features

### Automatic Water Tank Filling

- Automatic filling of the **1000-liter water tank**.
- Pump control via **433 MHz radio**.
- Autonomous self-contained operation without requiring an Internet connection.

### Automatic Irrigation

The irrigation system provides fully automatic control of the garden watering process:

- A clean and minimalist **web interface** allows users to configure irrigation schedules, monitor the entire system, and access additional controls and information.
- The system can control **as many irrigation valves as physically supported by the hardware**. It is currently configured to control **8 × 24 V AC valves**, with each valve corresponding to a specific watering zone.
- **Soil moisture detection** helps determine whether watering is actually necessary.
- **Pump control via 433 MHz radio**.
- **Local and autonomous operation**, without relying on a remote server, Internet connection, or cloud service.

### Time Backup

- An external **DS3231 RTC (Real-Time Clock)** keeps track of the time.
- The irrigation schedule remains accurate even after a **power outage**.
- The controller can resume normal operation without requiring an Internet connection or time synchronization with an external server.

### Key Advantages

- **Works without Internet** — the system remains fully functional even when the - Internet connection is unavailable.
- **Low latency** — commands are processed locally without waiting for a remote - server.
- **No cloud dependency** — the controller does not require a cloud platform or - external backend.
- **Better privacy** — system data and controls remain local.
- **Continued operation during Internet outages** — irrigation schedules continue - to run normally.
- **Long-term maintainability** — fewer external dependencies can make the system - easier to maintain and support over time.
- **No dedicated mobile app** — no application needs to be installed from the App - Store or Google Play.
- **No user account** — no registration or login is required.
- **No subscription** — there are no recurring cloud or software fees.
- **Browser-based access** — simply open the controller's local web interface in a - browser.
- **Easy mobile access** — the web interface can be saved as a browser shortcut or installed as a **Progressive Web App (PWA)** for an app-like experience.

In short:

> No app. No account. No subscription. No cloud. Just open the controller in a browser and use it.

## ⚠️ Important Pump Safety Note

**A centrifugal pump should not be operated for extended periods at zero or near-zero flow.**

In typical residential installations, a pressure switch may stop the pump when the system reaches high pressure, which generally corresponds to a very low or zero flow condition.

The irrigation controller should therefore be designed to **avoid running the pump continuously against a closed system** and should include appropriate protection or operating logic to prevent prolonged zero-flow operation.

# 📖 Documentation

## Available VSCode tasks

BRIDGE

```
bridge: dev live refresh
bridge: dev (mock)
bridge: dev (esp32)
🌍 bridge: live refresh + dev (mock)
🌍 bridge: live refresh + dev (esp32)
🌍 bridge: prod
```

ESP32: FILESYSTEM SIDE

```
🔨 esp32-fs: deploy webui app
⬆️ esp32-fs: upload fs image (USB)
⬆️ esp32-fs: upload fs image (OTA)
🚀 esp32-fs: deploy fs (USB)
🚀 esp32-fs: deploy fs (OTA)
```

## Fast web dev

Quick setup for developing and testing the web interface locally.

### Prerequisites

Create the virtual environment and install all python dependencies:

```bash
uv sync --project ./bridge
bun install --cwd ./webui
bun run --cwd ./webui build-app
```

`bun` toolkit is required, install it from [here](https://bun.com/docs/installation).

### 1. Start the server

```bash
cd bridge/scripts
cls && uv run ./start_bridge.py [--mock_esp_api] [--dev_mode]
```

#### Options

- `--mock_esp_api`  
  Use a fake API for frontend-only development. requests are sent to the ESP32 API using the URL defined in `/server/mock/config.py`.
- `--dev_mode`  
  Enables development features (see below).

---

### 2. Development mode

When using `--dev_mode`, start the file watcher beforehand in a separate terminal with:

```bash
cd bridge
cls && uv run ./live_refresh.py
```

This enables:

- Automatic browser opening
- Automatic browser refresh on file changes
- FastAPI auto-reload (backend)
- LiveReload injection (frontend)

---

### 3. Development workflow

Edit the following files:

```
./webui/src/*
./bridge/start_bridge.py
```

#### Adding new webfonts

Put the .ttf files that need to be converted in the `webui/fonts` folder and execute in terminal:

```bash
bun run --cwd ./webui convert-fonts
```

Then manually move the desired files into the `webui/src/assets/fonts` folder.

It is also necessary to execute the `copy-assets` bun script with:

```bash
bun run --cwd ./webui copy-assets
```

#### Adding new images

When adding new images into the `webui/src/assets/img` folder, it is also necessary to run `copy-assets` bun script (follow explanation in [Adding new webfonts](#adding-new-webfonts))

#### Tips

- Keep the Python web architecture consistent with the ESP32 implementation (`WebServer.h/cpp`)
- Links:
  - Access website locally at http://127.0.0.1:8000
  - Use the API docs for debugging at http://127.0.0.1:8000/docs

---

### 4. Export for production

Once everything works, try a build (minify+gzip):

```bash
bun run --cwd ./webui build
```

Note that it is not required to do before uploading to ESP32 filesystem image, since it is done automatically.

---

### 5. Deploy to ESP32

- Build the filesystem image
- Upload it to ESP32 using PlatformIO

## Versioning

vMAJOR.MINOR.PATCH

- MAJOR → breaking changes (API changes, payload changes)
- MINOR → new features (new sensors, UI additions)
- PATCH → bug fixes

Example for my system

- WEB UI: 1.4.0
- ESP32 API/WS: v1
- ESP32 FW: 1.3.2
- ESP32 HW: rev_C

Change hardware revision when:

- PCB changes
- sensors/peripherals change
- electrical capabilities change
- pin mappings change

Change firmware version when:

- any software changes

Change api/ws version when:

- request/response compatibility changes
- routes change
- frontend behavior must adapt

If it is same contract, with only more fields:

- field missing -> not supported on this device
- null -> sensor exists but no data
- number -> valid reading

## Features souhaitées d'un contrôleur intelligent

- Sécurité :
  - détection du débit (comme ça message erreur au cas où pompe arrosage ne s'allume pas, tuyau est pincé/débranché, vanne est bloqué, plusieurs vannes sont ouvertes, ...)
  - freeze prevention (températures proches de 0°C, donc à partir de températures < 3°C )
  - water hammer reduction (by opening next valve 10s prior to its scheduled time, and each valve a few seconds after the other)

- Plannificateur optimal :
  - utilise les secondes au lieu des minutes
  - cycle and soak
  - priority execution (with "time sensitive" checkbox, using it will prioritize it over others)
  - skip by rain/humidity sensor
  - skip by chance of rain (from internet local weather)
  - scheduling on sunrise/sunset
  - seasonal adjustment

- Praticité :
  - enable/disable zone
  - skip/delay for x days manually
  - synchro de l’heure avec horloge internet (inclus le décalage automatique de l’horaire)
  - graphiques (total water usage de type histogramme, et pour chaque capteurs, ...)
  - historique des actions (log)
  - arrosage à la main avec tuyau souple et pistolet (utilise une vanne 12v en plus)
  - intégration dans Home Assistant par msgs MQTT (pour voir état des capteurs dont niveau cuve et avoir des notifs en cas d’erreur, d'inactivité, ...)

## Recommandation alimentation

Recommended setup (solid and common approach), use:

- External 12 V DC adapter (brick style) outside the box
- A buck converter (12 V → 5 V) for the Pi and optionally another regulated rail if needed inside the box

Why 12 V instead of 5 V directly:

- Handles cable losses much better over ~1 m
- Allows valves, relays, or pumps to be powered directly if needed
- More robust overall system design

Why external power is the better choice:

- A watering system will live in a humid or even wet environment. Keeping 230 V AC out of the enclosure massively reduces risk.
- Thermal stability for the Pi. The Pi already runs warm, adding an internal AC-DC supply (like a Mean Well module) increases internal temperature and can lead to throttling or instability.
- Noise isolation. Pumps/valves switching can introduce electrical noise. Externalizing the PSU helps keep the sensitive logic side cleaner.
- Simpler enclosure design. No need to worry about: mains isolation distances, grounding, fire safety, ventilation for a PSU

## Partie eau

Pressostat/manomètre au plus proche du ballon (sinon pression plus instable à cause des pertes de charge liés au coude du flexible tressé)

Montage clapet et filtre côté refoulement

Distance pour capteur débit (3-10x amont 2-4x aval)

Eviter coudes

Avoir flexible en sortie de pompe

Si besoin de plus de débit, aller sur un embout rapide grand débit (pour Noémie par exemple)

## PCB

The 12->5V buck-converter used does not like having a reverse voltage at all (5V from ESP to it OUT+/OUT- pins). Using a USB cable without 5V for communicating with ESP32 while having 12V from external power supply. Even without 12V, never power ESP32 via USB directly if connected to PCB!

Use ferrite bead smd, to suppress high-frequency electromagnetic interference (EMI)

### Note about analog sensors

¨
Attenuation for adc is tunable in code with analogSetPinAttenuation().
Can go to as high as 11dB for readings up to 2.9-3.0V.
0.05V to 0.1V is practically unreliable.

| Attenuation Setting | Parameter Name     | Approx. Measurable Range (ESP32) | Best Use Case                                             |
| ------------------- | ------------------ | -------------------------------- | --------------------------------------------------------- |
| 0 dB                | `ADC_ATTEN_DB_0`   | 100 mV – 950 mV                  | High-precision low-voltage sensors. Highest resolution.   |
| 2.5 dB              | `ADC_ATTEN_DB_2_5` | 100 mV – 1.25 V                  | Slightly extended range for low-voltage signals.          |
| 6 dB                | `ADC_ATTEN_DB_6`   | 150 mV – 1.75 V                  | Mid-range sensors (e.g., some battery monitors).          |
| 11 dB               | `ADC_ATTEN_DB_11`  | 150 mV – 2.45 V (up to ~3.1 V\*) | General-purpose 3.3 V logic sensors. Most common setting. |

\*While 11 dB theoretically allows reading up to ~3.9V, accuracy drops significantly above 2.45V due to non-linearity

ADC need calibration for readings to be meaningful and reliable.

#### Note about capacitive soil moisture sensor

What to check for a good capacitive soil sensor:
https://www.youtube.com/watch?v=IGP38bz-K48

- Presence of 3.3v regulator instead of only resistor
  => make it stable if source is not
- Presence of TCL555C or TCL555I instead of NE555 (works only at 4.5-16V, even if some batches actually work at 3.3V)
  => allow powering at 5V AND 3.3V
- Correct positionning of via hole, to have the 1M ohm resistor connected to ground
  => allow output to vary much faster

Measurements done with a sensor that does not have 3.3v regulator, but everything else is ok:
5V + no load: 1.8V wet / 3.7V dry
5V with voltage divider of 51k/100k: 1.1v wet / 2.1v dry (aout is now 3.1V instead of supposedly 3.7V due to high impedance)
3.15V + no load: 0.8V wet / 2.2V dry

#### Actual sensor layout

Ponts diviseurs de tension:

- 51k/100k pour low/high (5V à vide et 4.5V avec pont et 3V vers gpio)
- 33k/100k pour filling (4.3V à vide et 4V avec pont connecté et 3v vers gpio)

=> agissent comme pull-down si capteurs non branchés (donc pas de pins floating)

Pont de 20k/47k irait pour capteur flow (5V à vide et 4.3V avec pont et 3v vers gpio) mais finalement pas de pont -> 3.3V direct + utilise une pull-up de 47k pour éviter que le pin soit flottant si capteur non branché

Pas de pont non plus pour capteur soil moisture -> 3.3V direct + utilise une pull-down de 300k pour éviter que le pin soit flottant si capteur non branché

Capteur courant OK (1.65V biased, ±1V around bias) -> utilise une résistance de 47k en parallèle du capteur pour éviter que le pin soit flottant si capteur non branché

#### New sensor layout

Avoir un câble shieldé et un condo proche du GPIO pour limiter les interférences.

Utiliser un comparateur logique au lieu de pont pour tous les capteurs digitaux si futur version avec pcb (3.3 V Schmitt-trigger buffer with a suitable input divider/clamp). Ex:

```
YF-B10
   │
   │ 5m cable
   ▼
[TVS/protection] ── [RC filter] ── [Schmitt/comparator] ── ESP32
```

Il existe d'autres versions pour les capteurs XKC-Y25-V et le capteur XKC-Y26-V de la cuve. A la place du "-V", c'est soit "-NPN" ou "-PNP":

- NPN (sinking / open-collector):
  liquid detected = output is pulled to ground (0V)
  no liquid detected = output is floating (open circuit)
- PNP (sourcing / open-collector):
  liquid detected = output is pulled up to positive supply voltage (Vin)
  no liquid detected = output is floating (open circuit)

Malheureusement, la logique de détection qui rend robuste le système n'est pas possible avec une autre version que la "-V" pour la capteur de haut-niveau de la cuve:

- Cuve pas pleine -> 5V
- Cuve pleine OU capteur déconnecté OU capteur cassé\* -> 0V

\*Uniquement si le failure mode maintient effectivement la sortie à l'état bas

| Sensor         | Future output         | ESP32 interface            |
| -------------- | --------------------- | -------------------------- |
| `LowLevel`     | "-PNP", source to Vin | GPIO with pull-down        |
| `HighLevel`    | Same "-V" version     | GPIO with level shifter    |
| `Filling`      | "-PNP", source to Vin | GPIO with pull-down        |
| `SoilMoisture` | Analog, high-Z        | **Buffer + scaling**       |
| `Flow`         | 5 V push-pull         | **Level shifting/scaling** |
| `PumpCurrent`  | 1.65 V ±1 V           | Direct ADC input           |

Toujours s'assurer que lorsqu'un capteur n'est pas connecté ou pas actif, d'avoir un état fixe avec une résistance pull-up ou pull-down (directement sur output du capteur ou après une étape de scaling par un op-amp par exemple).

## Solution avec contrôleur existant

Besoin des 3 systèmes suivants :

- Si puit trop loin des zones à arroser, ajouter un système automatique DIY de remplissage de la cuve avec détection arrivée d'eau et niveau + désactivation master valve ou pompe arrosage si erreur (cuve vide ou problème de capteur par exemple)\
  &#8594; Permet aussi de recevoir une erreur de débit faible de la part du contrôleur\
  &#8594; Avec intégration dans Home Assistant par msgs MQTT (pour voir niveau cuve et avoir notif en cas d’erreur)
- Contrôleur intelligent utilisant les secondes
- Ajout arrosage avec tuyau manuel si besoin avec système indépendant du contrôleur intelligent, qui comporte une vanne supplémentaire (à placer en amont du capteur de débit utilisé par le contrôleur, pour ne pas déclencher de fuite par exemple) et un bouton qui déclenche relai en parallèle pour allumer la pompe et qui déclenche un compte à rebours de 30min avec possibilité de réappuyer pour le remettre à 30min. Le bouton se trouve au niveau du branchement du tuyau avec un affichage à segments qui indique le temps restant (étiquette avec texte qui dit « purger le tuyau après utilisation » ou « ranger le tuyau après utilisation »)

## Possible improvements

### Very useful

#### Reset button

Resets everything to default. Could be used if someone entered an access-point password he cannot remember, while having useAPMode set to true, for example.

#### Internet connectiviy

Pour publier des données par Internet, cf. [achat carte sim](https://www.thingsmobile.com/business/shop) et [tuto](https://randomnerdtutorials.com/esp32-sim800l-publish-data-to-cloud/)

3 façons de faire :

- juste wifi esp32,
- `wifi esp32 + alertes par sms` <- préference pour celle-ci
- wifi esp32 uniquement pour debug serial par ex et serveur web qui communique avec esp via carte sim pour toutes les fonctionnalités (permet de recevoir des notifs par SMS ou eMAIL si défaut rencontré ou même de pouvoir accéder à l'interface de partout avec un serveur web externe)

cf.notes dans README.md de la partie webui à `Convenience`

#### Water tank level (replacement)

Upgrade the simplest water tank EMPTY/FULL levels with an ultrasonic distance sensor

#### LAN (addition by SPI interface + 2 worth considering pins)

⚠️ Almost all SPI pins (18, 19 and 23) are already in use by some elements such as 7-segment display.

Another external box for RJ45 LAN connectivity with lightning surge isolator by SPI and IF no LED is already present on the femelle port on the module, use the last available pin for a connection status green LED. Connection status LED states :

- off (LAN not available, rare but failure state)
- blinking (connected/communicating)
- solid (active and ready)

Note for RJ45 LAN module (like ENC28J60 module), there are two additional pins that would be worth connecting:

- INT (reduces polling and CPU usage, allow efficient networking)
- RESET (lets the ESP32 recover the Ethernet controller if it locks up or during startup sequencing)

#### Valve solenoid current sensor (addition by additional I2C expander)

Ajout d'un capteur de courant pr connâitre l'état de santé des solénoïdes des vannes

#### Hand watering buzzer (addition by additional I2C expander)

Add a buzzer for knowing time left for hand watering

Think in terms of “where am I in the cycle?”, example:
30:00 left → three short beeps
20:00 left → two short beeps
10:00 left → one short beep
Last 30 seconds → slow periodic beep (every ~2 sec) + LED blinking on sync
Last 5 seconds → fast beep + LED blinking in sync
0:00 → long beep (stop)

Nice to have (peut-être pas utile car trop agressif)

#### Weather station (addition by additional I2C expander)

Another external box for weather station (wind speed/direction, air temperature/humidity, rain, ...) with expander by I2C. Add these sensors:

- air temp/humidity
- pression atmo
- anémomètre sur girouette (pour aussi avoir direction du vent)
- luxmètre
- pluviomètre
- soil temp ?

### Less useful

#### Mesh filter pressure sensors differential (addition by additional I2C I2C expander)

Ajout de 2 capteurs de pression (avant/après filtre à tamis, qui a tétons 1/4" intégrés, cf. marque Azud sur Jardinet) pour savoir qd nettoyer le filtre. Notes :

- seulement utile s'il pose souvent pb en se bouchant, limitant le débit (si les capteurs sont utilisés, alors log la pression et le delta de la pression sous forme de graph dans page history)
- possible aussi, en fonction de comment il se salit, d'ajouter simplement un préfiltre désableur sur le côté aspiration de la pompe

# ⚙️ Tasks

## ✅ OK

- Restructuration du repo en sous-projets

## 🔧 To clarify / questions

How to handle versionning automatically for all projects? (bridge, firmware, webui, hardware)
Add automatic tags or commit version when merge to main?
