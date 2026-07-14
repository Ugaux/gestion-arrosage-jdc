# Description projet

Code fonctionnant sur un ESP32 permettant la gestion de l'arrosage pour le Jardin du Ciel à Vitabox (Algolsheim, FR).

Fonctionnalitées :

- remplissage automatique de la cuve de 1000L (pilotage pompe par radio 433 MHz)
- arrosage automatique :
  - paramétrage du calendrier par interface web ou manuellement par boutons de navigation
  - pilotage de 8 vannes 24V AC pour choix de la zone
  - détection de l'humidité du sol pour décision d'arrosage
  - pilotage pompe par radio 433 MHz
- sauvegarde de l'heure en cas de coupure de courant (par horloge externe RTC_DS3231)

Note importante :
Ne laissez pas une pompe centrifuge fonctionner pendant de longues périodes à débit nul. Dans les systèmes résidentiels, le pressostat arrête la pompe lorsque la pression est élevée, ce qui signifie que le débit est faible ou nul.

# Fast web dev

Quick setup for developing and testing the web interface locally.

## Prerequisites

Create the virtual environment and install all python dependencies:

```bash
uv sync --project ./bridge
bun install --cwd ./webui
```

`bun` toolkit is required, install it from [here](https://bun.com/docs/installation).

## 1. Start the server

```bash
cd bridge/scripts
cls && uv run ./start_bridge.py [--mock_esp_api] [--dev_mode]
```

### Options

- `--mock_esp_api`  
  Use a fake API for frontend-only development. requests are sent to the ESP32 API using the URL defined in `/server/mock/config.py`.
- `--dev_mode`  
  Enables development features (see below).

---

## 2. Development mode

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

## 3. Development workflow

Edit the following files:

```
./webui/src/*
./bridge/start_bridge.py
```

### Adding new webfonts

Put the .ttf files that need to be converted in the `webui/fonts` folder and execute in terminal:

```bash
bun run --cwd webui convert-fonts
```

### Tips

- Keep the Python web architecture consistent with the ESP32 implementation (`WebServer.h/cpp`)
- Links:
  - Access website locally at http://127.0.0.1:8000
  - Use the API docs for debugging at http://127.0.0.1:8000/docs

---

## 4. Export for production

Once everything works, try a build (minify+gzip):

```bash
bun run --cwd webui build
```

Note that it is not required to do before uploading to ESP32 filesystem image, since it is done automatically.

---

## 5. Deploy to ESP32

- Build the filesystem image
- Upload it to ESP32 using PlatformIO

# ⚙️ Tasks

## ✅ OK

- Restructuration du repo en sous-projets

## 🔧 To clarify / questions

How to handle versionning automatically for all projects? (bridge, firmware, webui, hardware)
Add automatic tags or commit version when merge to main?

# Notes

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

(same contract, just more fields)
field missing -> not supported on this device
null -> sensor exists but no data
number -> valid reading

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
- Lets you power valves, relays, or pumps directly if needed
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

Use ferrite bead smd, to suppress high-frequency electromagnetic interference (EMI)

## Solution avec contrôleur existant

Besoin des 3 systèmes suivants :

- Si puit trop loin des zones à arroser, ajouter un système automatique DIY de remplissage de la cuve avec détection arrivée d'eau et niveau + désactivation master valve ou pompe arrosage si erreur (cuve vide ou problème de capteur par exemple)\
  &#8594; Permet aussi de recevoir une erreur de débit faible de la part du contrôleur\
  &#8594; Avec intégration dans Home Assistant par msgs MQTT (pour voir niveau cuve et avoir notif en cas d’erreur)
- Contrôleur intelligent utilisant les secondes
- Ajout arrosage avec tuyau manuel si besoin avec système indépendant du contrôleur intelligent, qui comporte une vanne supplémentaire (à placer en amont du capteur de débit utilisé par le contrôleur, pour ne pas déclencher de fuite par exemple) et un bouton qui déclenche relai en parallèle pour allumer la pompe et qui déclenche un compte à rebours de 30min avec possibilité de réappuyer pour le remettre à 30min. Le bouton se trouve au niveau du branchement du tuyau avec un affichage à segments qui indique le temps restant (étiquette avec texte qui dit « purger le tuyau après utilisation » ou « ranger le tuyau après utilisation »)

# Possible improvements

## Very useful

### Internet connectiviy

Pour publier des données par Internet, cf. [achat carte sim](https://www.thingsmobile.com/business/shop) et [tuto](https://randomnerdtutorials.com/esp32-sim800l-publish-data-to-cloud/)

3 façons de faire :

- juste wifi esp32,
- `wifi esp32 + alertes par sms` <- préference pour celle-ci
- wifi esp32 uniquement pour debug serial par ex et serveur web qui communique avec esp via carte sim pour toutes les fonctionnalités (permet de recevoir des notifs par SMS ou eMAIL si défaut rencontré ou même de pouvoir accéder à l'interface de partout avec un serveur web externe)

### Water tank level (replacement)

Upgrade the simplest water tank EMPTY/FULL levels with an ultrasonic distance sensor

### LAN (addition by SPI interface + 2 worth considering pins)

⚠️ Almost all SPI pins (18, 19 and 23) are already in use by some elements such as 7-segment display.

Another external box for RJ45 LAN connectivity with lightning surge isolator by SPI and IF no LED is already present on the femelle port on the module, use the last available pin for a connection status green LED. Connection status LED states :

- off (LAN not available, rare but failure state)
- blinking (connected/communicating)
- solid (active and ready)

Note for RJ45 LAN module (like ENC28J60 module), there are two additional pins that would be worth connecting:

- INT (reduces polling and CPU usage, allow efficient networking)
- RESET (lets the ESP32 recover the Ethernet controller if it locks up or during startup sequencing)

### Valve solenoid current sensor (addition by new I2C expander)

Ajout d'un capteur de courant pr connâitre l'état de santé des solénoïdes des vannes

### Hand watering buzzer (addition by new I2C expander)

Add a buzzer for knowing time left for hand watering

Think in terms of “where am I in the cycle?”, example:
30:00 left → three short beeps
20:00 left → two short beeps
10:00 left → one short beep
Last 30 seconds → slow periodic beep (every ~2 sec) + LED blinking on sync
Last 5 seconds → fast beep + LED blinking in sync
0:00 → long beep (stop)

Nice to have (peut-être pas utile car trop agressif)

### Weather station (addition by new I2C expander)

Another external box for weather station (wind speed/direction, air temperature/humidity, rain, ...) with expander by I2C. Add these sensors:

- air temp/humidity
- pression atmo
- anémomètre sur girouette (pour aussi avoir direction du vent)
- luxmètre
- pluviomètre
- soil temp ?

## Less useful

#### Mesh filter pressure sensors differential (addition by new I2C I2C expander)

Ajout de 2 capteurs de pression (avant/après filtre à tamis, qui a tétons 1/4" intégrés, cf. marque Azud sur Jardinet) pour savoir qd nettoyer le filtre. Notes :

- seulement utile s'il pose souvent pb en se bouchant, limitant le débit (si les capteurs sont utilisés, alors log la pression et le delta de la pression sous forme de graph dans page history)
- possible aussi, en fonction de comment il se salit, d'ajouter simplement un préfiltre désableur sur le côté aspiration de la pompe
