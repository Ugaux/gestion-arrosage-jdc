# 📖 Documentation

`For embedded/hardware control code, it is generally advised to choose boring and obvious over clever and compact.`

On every new version:

- Build/upload in release mode
- Generate dependency graph

## Available VSCode tasks

```
🧪 esp32-fm: test native
🔨 esp32-fm: build debug
⬆️ esp32-fm: upload debug (USB)
🔨 esp32-fm: build release
⬆️ esp32-fm: upload release (USB)
⬆️ esp32-fm: upload release (OTA)
```

## Websocket tips

Use the ArduinoJson Assitant v7 at https://arduinojson.org/v7/assistant/#/step1

## API definition

### calls

/api/status
/api/cmd
/api/version

### health response

```JSON
"wifi": {
  "connected": true,
  "rssi": -62,
  "ip": "192.168.1.42"
},
```

```JSON
"hardware": {
  "valves": {
    "1": "ok",
    "2": "ok",
    "3": "fault"
  }
},
```

In case the schedule is computed on an external server and sent to ESP32 periodically, the following could be added to API health call:

```JSON
{
  "last_schedule_sync": "2026-06-09T10:15:00Z",
  "schedule_sync_age_s": 12,
  "schedule_version": 42,
}
```

## schedule.ini entry format

```ini
; *********************************************************************
; *********************** Schedule entry format ***********************
; *********************************************************************
;
; Each [schedule] section defines one watering schedule.
;
; Keys:
;
;   id=<UUID>               Version 4 UUID (RFC 4122)
;   line=<ID number>        Line it belongs to
;   name=<text>             Display name (optional)
;   def=<HH>:<MM>,<duration>,<onlyIfDrySoil>,<frequency>
;
; The def value is a comma-separated list with fields:
;
;   HH              hour of day (00-23)
;   MM              minute of hour (00-59)
;   duration        watering duration in minutes (1-255)
;   onlyIfDrySoil   0 = always water
;                   1 = water only if soil is dry
;   frequency is one of:
;     *          every day
;     e          even days of the month (2, 4, 6, ..., 30)
;     o          odd days of the month (1, 3, 5, ..., 31)
;     s-<mask>   specific weekdays using a bitmask
;                weekdays are specified by OR-ing the values:
;                  Mon=1, Tue=2, Wed=4, Thu=8, Fri=16, Sat=32, Sun=64
;                e.g. Mon | Wed | Fri | Sat = 1 + 4 + 16 + 32 = 53
;
; Example:
;
;   [schedule]
;   id=e27eb9df-f08d-4f88-84fb-a7a553dbbc83
;   line=1
;   name=Morning
;   def=22:00,30,0,s-53   (=> runs every monday, wednesday, friday
;                         and saturday at 22:00 for 30 minutes,
;                         regardless of soil moisture)
```

## ESP32 pinout

Expander

- _IOExpander0_: relay 0
- _IOExpander1_: relay 1
- _IOExpander2_: relay 2
- _IOExpander3_: relay 3
- _IOExpander4_: relay 4
- _IOExpander5_: relay 5
- _IOExpander6_: relay 6
- _IOExpander7_: relay 7

Watchdog

- _!WDO_: LED verte (status)
- _RST_ : No connected
- _!RST_ : ESP32 EN pin with pull-up

Left side

- _GPIO36_: courant pompe arrosage input - **ADC1**
- _GPIO39_: flow sensor input - **ADC1**
- _GPIO34_: soil moisture sensor input - **ADC1**
- _GPIO35_: présence arrivée eau remplissage cuve input - **ADC1**
- _GPIO32_: 433Hz radio tx output - **ADC1**
- _GPIO33_: **_UNUSED_** - **ADC1**
- _GPIO25_: Watchdog WDI - **ADC1**
- _GPIO26_: LED bleu/orange (arrosage en cours)
- _GPIO27_: LED rouge (erreur fonctionnelle)
- _GPIO14_: mosfet (LED bouton arrosage à la main)
- _GPIO13_: mosfet (vanne 12v arrosage à la main)

Right side

- _GPIO23_: bouton arrosage à la main input
- _GPIO22_ (sda): RTC i2c
- _GPIO22_ (sda): IO Expander i2c
- _GPIO21_ (scl): RTC i2c
- _GPIO21_ (scl): IO Expander i2c
- _GPIO19_: afficheur 7 segments dio
- _GPIO18_: afficheur 7 segments clk
- _GPIO17_ (tx2): cuve niv haut input
- _GPIO16_ (rx2): cuve niv bas input

## Compilation rapide avec WSL et Ubuntu 24.04.01 LTS

`Prend seulement une dizaine de seconde contre facilement 1min10 sur Windows avec l'IDE d'Arduino`

Commandes :

```
$ sudo apt update
$ sudo apt upgrade
$ sudo snap install arduino-cli
$ sudo apt install libstdc++6
$ sudo apt install python-is-python3
$ arduino-cli board details -b esp32:esp32:esp32da
$ arduino-cli compile -v ~/gestion_arrosage_jdc --build-path ~/gestion_arrosage_jdc/build
```

# ⚙️ Tasks

## ✅ OK

- Copier upgrade cuve
- Renommage wifi en "Arrosage Jardin du Ciel"
- Pb capteur humidité
- Changement terme "Systématique" pour "Arroser même en cas de pluie"
- Affichage erreurs sur écran
- RTC détection plus de pile + si déconnection du spi/5v/gnd (doit reset puis dans setup mettre msg erreur)
- Changement design interface web avec websockets basé sur travail de [rgodin974](https://github.com/rgodin974/ESP32_sprinkler_timer/tree/main)
- Conversion SPIFFS en LittleFS en personnalisation la librairie SPIFFSIniFile
- Simplification structure de /src
- mDNS pour accès avec `http://<adresse>.local` au lieu de 10.10.10.1

## 🔧 To clarify / questions

class errorManager ? gere les leds du boitier et les faults string

Wire.begin() ?
augmenter clock i2c avec Wire.setClock(400000);

### Refactor complet du code pour enlever inter-dépendances et ajout :

- Remplacer class statique Way actuelle par WayManager et fonctionnera comme SensorManager passée par ref dans begin() de WebsocketServer (cf.fichier exemple dans dossier `Future refactor`)
- possibilité d'avoir un way non rattaché à une zone pour éviter un nommage étrange
- better config file format (JSON?) and structure (cf. folder in `doc`)
- codage en natif sur windows avec unit testing
- debug dans platform IO et release ensuite
- OTA upload

Bugfix:

- Pb vanne si pas de planning de fait
- Loop main par rapport à 0 de chaque minute et non random
- Vérif dans watering.cpp : "if (at + (m_duration \* 60)" et "if (at + 10 < now)"

### Features importantes

- _ID ON ERROR FOR DELETION IF MULTIPLE PRESS OF CLEAR AT SAME TIME_
- Utiliser nvs pour stocker pump cycles et ajouter une date pour savoir depuis quand ça incrémente (commissioningTimeSec), et pour last command et last schedule update et last config modified ... + ne pas écraser lors d'un upload firmware et littlefs
- Quand activation manuelle de la vanne dans settings, la fermer automatiquement après un timeout
  => Dépend de comment c'est géré par rgodin

### Serveur

- /api/… returns first (keep at least API call for device status (GET) and to reset (POST) device with CURL command and one/two other relevant calls)
- all others url paths return index.html
  The index.html handles "Page Not Found"
- Check si quand esp est down dans console log je vois websocket error
- on config change, first do checks if config is valid (like maximum size check, valid INI syntax check or required sections/keys check), if so update last modified to current time + faire suivant pour éviter qu'une perte de courant corrompe les données :
- envoyer ack que au client specifique :

```
Client A                 ESP32                  Client B
   |                       |                       |
   |--- toggleValve ------>|                       |
   |                       |                       |
   |<-- ACK success -------|                       |
   |                       |                       |
   |<---- valveChanged -------------------------->|
```

### ini save()

LittleFS is explicitly designed to be fail-safe. Its documentation states that
"All POSIX operations, such as remove and rename, are atomic, even in the event of power-loss."

config.ini → active configuration
config.ini.tmp → incomplete update in progress

```
Validate config object
  |
  |-> Failed -> Return error
  |
  v
Write in settings.ini.tmp
(checks all write/flush/close errors)
  |
  |-> Failed -> Delete .tmp -> Return error
  |
  v
Rename settings.ini.tmp in settings.ini
(overwrites old settings.ini)
  |
  |-> Failed -> Delete .tmp -> Return error
  |
  v
Done
```

Then on boot:
if settings.ini exists and is valid:
use it
delete stale settings.ini.tmp (if present)
else:
no valid configuration available

Note: avoir même mécanisme pour aussi schedules.ini

### Hand watering

- Ajout afficheur à segments
- Timing drift in main loop avec afficheur 7 segments pour arrosage à la main
- Ajout si appuie successif dans les 5 1ère secondes, augmenter temps total (commencer avec 10min, puis 20min, ...)
- Short press/release pour activation vanne arrosage à la main, and for really short press/release or long press/release do nothing (so it act as a cancel).

### WiFi

- How to detect Access Point failure on ESP32?
- Sleep mode when no one connected
- Captive portal en mode AP ? (cf. dossier dans `doc`)
- Captive portal qui demande de rien faire si pas sûr, avec lien pour visiter le site une fois sortie du portail

### mix

- rename Watering in Schedule (peut etre pas finalement car groupe auto et manual)
- rename ConfigLoader in IniSettings (pas besoin en fait cest le parseur de ini dans dossier config et il a obtenu les infos de settings.ini)
- rename spiffsinifile in littlefsinifile (et fichiers de descriptions à aussi modifier)
- change for pragma once everywhere + commentaire du nom du fichier actuel en haut ex: "// Settings.h"
- Debug generalisé et false si release
- Use tasks for building + check si upload va aussi faire un build avec ma task custom "upload" -> en fct ajouter task build ou non dans full deploy
  (pio run -e ESP32-release --target upload)
- Enable brownout detector
- Pins no utilisés en tant qu'output à low + qu'en est-il des input only ?
- Ajout hand watering
- Ajouts capteurs :
  - Débit
  - Capteur présence arrivée eau dans la cuve (ajout tempo de 10s quand cuve devient vide pour laisser le temps à l'eau d'arriver avant de détecter la présence eau, si capteur ne détecte plus d'eau avant que la cuve soit pleine, afficher erreur pompe)
  - Aussi check capteur humidité (_cf. vidéo youtube_)
- Avoir fault led en clignotement et non statique en cas de problème
- Utiliser les watchdog interne en plus du externe
- Stocker l'état de la cuve pour que si l'esp s'éteint, au redémarrage le remplissage continuer
- Réduire envoi radio avec biblioteque ([RCSwitchRmt](https://github.com/Upartech/RCSwitchRmt/tree/main) qui a send non bloquant) pr code papa car ajoute pb de reactivité lors d'un appui sur bouton avec arrosage en cours (seulement besoin d'envoyer 3-5 commandes, pas besoin de le faire pdt 10ms) https://chatgpt.com/c/682541fa-2364-800e-8035-e06e5f3cb1cc
- mDNS pour accès avec http://arrosage.local
- use .csv for tailles partitions esp32 (à ajouter dans "platformio.ini") ? cf. fichier "huge_app.csv" de rgodin976

## Next steps

API calls implementation

OTA upload implementation

Codage en natif sur windows avec test unitaires

### Hand watering

- Ajout afficheur 7 segments et régler problème de timing drift entre le décompte sur l'écran et le main loop
- Ajout réglage du temps d'arrosage par appuies successifs dans les 5 1ère secondes, (commencer avec 10min, puis 20min, ...)

### Calibrations

- Calibration des gpios (ADC1) avec Platform IO (pour ceux qui en auraient besoin comme le capteur d'humidité du sol et le capteur de courant)
- Calibration des capteurs p/r à des valeurs données par un autre appareil (multimètre par exemple)

### Security concerns

Handle security concerns as ap/wifi credentials (store them in encrypted storage of ESP32 and transmitted via an encrypted canal, like TLS) -> should absolutely think about security problems if selling this as a product.

During first-time setup, the physical device could display/provide a unique setup code (for example on a label, display, or serial console). The user uses that code to establish trust during provisioning.

Then you can give the ESP a unique certificate/private key.

For a hobby/open-source project, another pragmatic option is:

- HTTP/WS for normal local UI
- HTTPS/WSS specifically for credential provisioning
- never expose the ESP's management interface outside the LAN
- authenticate sensitive operations
- encrypt stored credentials with NVS encryption

That provides most of the practical benefit without turning the watering controller into a certificate-management project.

If a strong MITM protection with no cloud dependency and no browser certificate warnings is wanting, though, the interesting problem is really how the first trust relationship is established.
That's the part that should be designed carefully.

### Logique cuve (avec nouveau capteur distance à ultrason)

- Cuve vide, niv faible, niv moyen, niv haut et cuve pleine (avec 4 capteurs, et arrosage qui se coupe qd cuve vide) + check que pompe remplissage a un débit suffisant pour que la cuve n'atteigne jamais le niveau vide (ou après un long moment) lorsque arrosage est à son débit max -> faire calcul avec Bernoulli
- Suspension arrosage quand cuve vide jusqu'à faible (environ 2 bar de pertes de charge sur tuyau remplissage cuve, donc débit plus faible que débit pompe arrosage, la pompe va s'éteindre et s'allumer souvent si fonctionnement proche des 4600 L/h à cause du fait que la cuve va se vider plus vite qu'elle se remplie et va trigger le niv bas) + suspension timer de remplissage cuve jusqu'à que arrosage soit fini (pour avoir une erreur si le tuyau de remplissage n'est pas dans la cuve)

### Watchdog

#### Internal vs External

If I use an external watchdog, should I still use the ESP32 internal watchdogs?
Yes. In most cases, use both. Think of them as different layers of protection.

Internal watchdogs protect against:

- task deadlocks
- infinite loops
- blocked code
- FreeRTOS task failures
  They can usually recover the system much faster than an external watchdog.

External watchdog protects against:

- ESP32 completely freezing
- software accidentally disabling internal watchdogs
- clock problems
- severe firmware bugs
- rare hardware lockups
  The external watchdog doesn't care what the software thinks—if it stops receiving heartbeats, it resets the board.

#### Heartbeat

Conditions indispensables pour que l'équipement remplisse sa mission :

- CPU fonctionnel
- RAM suffisante
- Tâches critiques vivantes
- Boucles de contrôle opérationnelles
- Wi-Fi indisponible
- perte d'accès à l'interface web
- erreur NTP

LED FAULT + journal d'événements, défauts importants mais non fatals :

- câble Ethernet débranché
- capteur secondaire absent
- etc

Ainsi, le watchdog ne redémarre pas la machine pour des problèmes qu'un redémarrage ne résoudra probablement pas

En résumé. heartbeat seulement si :

- WiFi AP actif
- serveur web actif
- tâche principale vivante
- mémoire disponible suffisante
- aucune erreur critique

```cpp
bool healthy =
    wifi_ok &&
    webserver_ok &&
    task1_ok &&
    task2_ok;

if (healthy) {
    kickWatchdog();
}
```

_Note:_ ne plus gérer les erreurs si système pas healthy pour ne jamais avoir la led fault allumée sans que la led de status ne soit allumée !
-> Je déconseille le cas "STATUS OFF + FAULT ON", car si l'ESP32 est réellement planté, il ne devrait plus être capable de piloter la LED rouge. Cela rend le diagnostic moins clair.

##### WebServer health check

```cpp
HTTPClient http;
http.begin("http://127.0.0.1/health");
int code = http.GET();
web_ok = (code == 200);
```

--> pour check que interface web fonctionne, mais seulement arrêter heartbeat si serveur web inaccessible pendant un délai de x secondes ou minutes

#### Indicateurs LED

Si ton watchdog externe possède une sortie "OK" ou "HEALTHY", tu peux l'utiliser pour autoriser l'allumage de la LED rouge.

Signification :

- LED Verte STATUS État global du contrôleur ESP32
- LED Rouge FAULT Défaut fonctionnel de l'installation ou d'un périphérique

Led WiFi AP sur boîtier ? Non car la LED de status indique que le hardware propre a l'ESP fonctionne (puce elle-même et wifi). La LED de status allumée indique donc que le système est opérationnel et quand elle est éteinte elle indique un problème (puce cramée, loop frozen, resets en boucle, WiFi AP non opérationnel, ...)

Le watchdog coupera la LED de status (avec un MOSFET probablement) en l'absence de heartbeat.
En fonctionnement normal, la LED est contrôlée par l'ESP32 :

- solid on : système OK
- blinking : en attente de connexion à la station WiFi
- pulsing : en attente que l'AP soit opérationnel
- solid off : système non opérationnel

Le watchdog apporte donc une sécurité supplémentaire lorsque l'ESP32 n'est plus elle-même capable de diagnostiquer ou de signaler son état.
Ainsi, une LED de status allumée implique que l'ESP32 est toujours en mesure de signaler sa présence au watchdog.

Watchdog resets Suggested action:

- 1 reset Log it only
- 2 resets within 24 hours. Warning in web UI/logs
- 3–5 resets within 24 hours. Turn on fault LED
  Continuous reset loop (e.g., >3 within 10 minutes) Immediate fault LED

### Updated scheduling

#### Arrosage manuel

If concurrent watering is enabled and working:
🟢 Enough flow → start manual watering normally.
🟡 Insufficient flow (even if 0 L/min) → show dialog.

E.g. dialog:

```
🟡 Not enough flow available!
Zone C requires 8 L/min, but only 2 L/min is currently available.

What would you like to do?

Run Zone C now
  "Shift current watering"
  -> I want mine now; make room for it. And continue current watering later when flow becomes available.
  "Start anyway"
  -> Keep the current watering running and start Zone C. Flow may be lower than expected, so watering may take longer.
  "Stop current watering"
  -> Cancel the currently running zones and give Zone C the required flow.

Wait
  "Queue for later"
  -> Start Zone C when enough flow becomes available (show in UI "⏳ Zone C — waiting for flow").

"Cancel"
-> do nothing
```

If concurrent watering is disabled:
🟢 Nothing running → start manual watering normally.
🟡 A watering(s) will run before complete execution of manual watering OR a watering(s) is running → show dialog.

E.g. dialog:

```
🟡 A watering(s) is already running!

What would you like to do?

Run Zone C now
  "Shift current watering"
  -> I want mine now; make room for it.
  "Start anyway"
  -> Keep the current watering running and start Zone C.
  "Stop current watering"
  -> Cancel the currently running zones.

Wait
  "Queue for later"
  -> I can wait. Start Zone C when current/following schedules finishes until a slow with desired duration is available.
    (display time to wait for better decision taking, like "Starts in ~7 min · expected finish 18:47")

"Cancel"
-> do nothing
```

And if the current watering contains a fixed schedule that cannot be moved, the UI can simply disable the "Shift current watering" button with a message "Some current watering cannot be shifted because it is fixed."

#### Planned execution

- Si un créneau est supprimé/modifié pendant un arrosage, le supprime et se ré-adapte avec ce qu'il reste à faire
  Affichage sous forme de tableau fixe avec 1 ligne par vanne ou panneau vertical (https://dev.to/crayoncode/building-a-vertical-calendar-with-html-css-js-2po2) pour avoir toutes les info de visible avec sélection par drop-down ou slideshow ou mosaique de boutons + ajouter une photo par zone
- cf. `Features souhaitées d'un contrôleur intelligent` du README principal + if greenhouse temperature > 30°C → augmenter durée arrosage (avec temp de seuil à pouvoir changer dans settings)
- Capteur de débit :
  - Ajouter tempo de 10s le temps que le débit ait le temps de se stabiliser (initial unstable period), anomaly thresholds (low et high), baseline flowrate
  - Ajout détection erreur si pas de débit arrosage après 5-10s alors que vanne censé être ouverte (vanne ou pompe cassée, et si ajout capteur de pression, c'est possible de savoir si c'est l'un ou l'autre, cad pompe ou capteur pression / vanne ou capteur débit. NB: analyser comportement en situation réelle pour logique du code)
  - Ajout check pr fuites à la fin d'un cycle d'arrosage (vannes fermées et analyse débit si supérieur à 1L/min environ ou sinon avec capteur pression sur 15 min et si pression descend légèrement ou non)
- cf. fichier exporté "ChatGPT-Irrigation Scheduling Design" pour developpement algo
- Python `astral` lib for sunrise/sunset times
- Rain:
  - If soil is dry AND no rain → ✅ Water plants
  - If soil is dry BUT it’s raining → ❌ Do not water
  - If soil is moist → ❌ Do not water (regardless of rain)

Garder le fixed scheduling, et ajouter les autres types d'exécutions par dessus.
=> All execution types (watering modes) can coexist:

- Manual
- Scheduled(fixed/flexible)
- Optimized

Suppose there is:

```
Fixed schedule:
Zone A → 06:30–06:50

Morning window optimized:
Zone B → 20 min
Zone C → 15 min
Zone D → 10 min
Window → 05:00–08:00
```

The optimizer knows that 06:30–06:50 is unavailable. So it might produce:

```
05:00  B ───────────────
05:20  C ──────────
05:35  D ───────
             ...
06:30  A ───────────────  ← manual, fixed
             ...
06:50  optimized jobs can continue
```

The fixed schedule becomes an immovable block, while the optimized schedules fill the available space around it.
=> Fixed schedules has priority over optimized schedules.

Priority 0. Manual (user can choose what to do if chances of clashing are present)
Priority 1. Fixed "Start exactly at 18:00."
Priority 2. Flexible "18:00 is the preferred time. The controller may start earlier or later when that produces a better execution opportunity."
Priority 3. Optimized "Water during the morning/evening period; optimize the entire group."
It goes from "I want control" to "help me a little" to "you figure it out."

##### 0. Fixed scheduling (current)

Allowed to overlap.

Aucune vérification n'est faite. En fonction de la plannification des zones, plusieurs vannes peuvent se retrouver à être ouvertes en mm temps, et les zones risquent de ne plus avoir assez de débit.

##### 1. Flexible scheduling

Timing option:
Exact — start at the specified time.
Flexible — start around the specified time to not clash with other schedules (user should not have to specify a time window, maximum delay would be an internal setting)

##### 2. Optimized grouping at sunset/sunrise

Algo exécute chaque watering du jour successivement après heure fixe décidée dans fichier de config, ou heure de levé/couché du soleil

#### Concurrent valves execution

System capability/setting "Concurrent watering": ☑ Enabled
(either one valve at a time or concurrent when flow allows)
-> ❌ fixed scheduling intentionally unaffected (always stupid simple)

Note: Concurrent mode available only when flow sensor is installed and all active lines have verified flow characteristics.

##### Old notes

Au lieu d'éxécuter successivement l'arrosage pour chaque voie, le faire de manière concurrente si possible.

Optimise l'arrosage avec débit max possible pour arroser plusieurs zones en même temps :

- Placer une zone pdt le soak (du cycle and soak) d'une autre
- Implémentation d'un overlap entre les créneaux des zones pour réduction du water hammer

Lors de l'ajout d'une voie par l'appuie du bouton "ajouter", ouvrir un dialog box avec notification que le systeme va mesurer le debit et si utilisateur clique sur continuer, une barre de progression d'une durée de 10-15s s'affiche pour laisser le temps au systeme de mesurer le débit
-> et sinon msg "Impossible d'ajouter la voie (pas assez de débit disponible pour pouvoir réaliser le test de débit)" ou
(Le débit minimal requis pour pouvoir réaliser le test de débit n'est pas disponible à cause du débit déjà utilisé par l'arrosage en cours)
Avoir aussi fonction de setup pour mesurer le débit max que peut fournir la pompe (dire de retirer tous les tuyaux pour avoir les vannes à nu)

Avant chaque créneaux d'arrosage, si au moins 2 vannes vont fonctionner en meme tps, faire démarrage successif des vannes et vérif du débit :

- si débit plus faible pour une zone, envoie warning par SMS (plutot log erreur dans onglet history avec notif si pas de sms) disant que le valeur a été automatiquement réadaptee, et que si c'est dû à l'action de quelqu'un par changement d'arroseur alors pas de soucis mais sinon problème d'obstruction (tuyau écrasé/bouché, arroseur cassé, pompe arrosage pas allumée, vanne bloquée, ...), puis réparer problème et refaire test débit dans onglet maintenance
- si débit plus élevé, envoie SMS warning pareil que cas précédent (avec comme raisons tuyau percé/débranché, plusieurs vannes ouvertes, ...) si assez de débit dans créneau sinon ajouter que tous les créneaux où pas assez de débit ont été désactivés

Check le nombre max de vannes activables en mm temps, à noter que :

- la limite est imposée par le courant max que peut fournir l'alimentation 24VAC
- allumer chaque vanne en différé permet d'éviter les pics de courant cumulés et donc d'avoir plus de vannes actives en même temps (exemple: 4 vannes allumées en mm temps = 4x0.7A = 2.8A d'un coup puis = 4x0.3A = 1.2A, alors que l'une après l'autre = {0.7A->0.3A, 0.3+0.7=1A->0.6A, 0.3x2+0.7=1.3A->0.9A, 0.3x3+0.7=1.6A->1.2A}, donc max 3 vannes si alim 2.2A avec allumage en mm temps contre 6 vannes avec allumage différé)

Avoir aussi extinction en différée pour réduction des chocs hydrauliques

#### Automatic watering requirement

Allows to get watering duration/frequency automatically computed from evapo-transpiration model (e.g. using a simplified version of FAO-56 Penman–Monteith).

Water line/zone option named "Watering requirement": "User defined" or "Automatic"
"Automatic" option:
-> only available if there is a weather access, either by local sensors (soil moisture / air humidity / wind speed and direction, ...), by internet, or a combination of both.
-> disables manual scheduling
-> requires a deep description of that line/zone with soil/sprinkler/plant/sun-orientation data.

https://www.yardian.com/blogs/articles/how-smart-watering-works/ :

- Field Capacity (FC)
- Permanent Wilting Point (PWP)
- Plant Available Water (PAW)
- Maximum Allowable Depletion (MAD)
