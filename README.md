# Description projet

Code fonctionnant sur un ESP32 permettant la gestion de l'arrosage pour le Jardin du Ciel à Vitabox à Algolsheim.

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

# ⚙️ Tâches

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

## 🔧 To clarify / questions

- Check TODOs avec extension vscode todo tree
- Calibrate adc1 avec platform io
- Pins no utilisés en tant qu output à low + qu'en est-il des input only ?

- Conversion code en API pour dev web plus facile (avec esp connecté en wifi)
- connection state dans sidebar à connecter à socket + si api call marche pas afficher dialog
- Check si quand esp est down dans console log je vois websocket error
- WEB UI:
  - changement en SPA avec sidebar et overlay/animations (et sidebar présente tout le temps sans close si grande fenêtre)
  - séparation en pages, home (avec status cuve/vannes/capteurs/...), schedule et settings
- bouton dans web pour sync heure avec tel, avoir le sync heure été hiver auto plutôt ?

## Next

### best

- page séparée pour arrosage manuel, et non plus dans page planning + pouvoir faire un arrosage manuel des zones selectionnées qui s'enchaînent
- for large screens, display the panels in a grid pattern instead of each under the other
- handle settings modal page for large screens (have a ) + 404 not found (return the correct HTTP status code + a helpful 404 page that include clear messaging and links to popular or related content to help users recover)
- Fault led en clignotement
- Pb vanne si pas de planning de fait
- Loop main par rapport à 0 de chaque minute et non random
- Vérif dans watering.cpp : "if (at + (m_duration \* 60)" et "if (at + 10 < now)"
- empêcher plus d'1 vannes (avoir algo qui exécute chaque watering du jour successivement après heure fixe décidée dans .ini)
- button released logic pour écran, du genre act on press pour la navigation et release pour démarrer arrosage (dont si arrosage manuel en cours, pouvoir uniquement arrêter celui là, ne rien afficher d'autre)
- Pour arrosage à la main, ajout si appuie successif dans les 5 1ere secondes, augmenter temps total (commencer avec 10min, puis 20min, ...)
- Short press/release pour activation vanne arrosage à la main, and for really short press/release or long press/release do nothing (so it act as a cancel).
- Timing drift in main loop avec afficheur 7 segments pour arrosage à la main
- versionning
- Refactor complet du code pour enlever inter-dépendances et ajout :
  - codage en natif sur windows avec unit testing
  - debug dans platform IO et release ensuite
  - OTA upload
- Ajouts capteurs (débit + présence eau pompe remplissage cuve) dont check capteur humidité (cf. vidéo youtube)
- Ajout afficheur à segments
- History page simple avec bouton recherche
- mdp pr page settings
- Réduire envoi radio avec biblioteque ([RCSwitchRmt](https://github.com/Upartech/RCSwitchRmt/tree/main) qui a send non bloquant) pr code papa car ajoute pb de reactivité lors d'un appui sur bouton avec arrosage en cours (seulement besoin d'envoyer 3-5 commandes, pas besoin de le faire pdt 10ms) https://chatgpt.com/c/682541fa-2364-800e-8035-e06e5f3cb1cc
- wifi sleep mode when no one connected

### second

- Edition zones sur web
- Garder info de "en remplissage cuve" si système s'éteint, pr continuer
- mDNS pour accès avec http://arrosage.local
- Captive portal en mode AP ? cf. notes
- Captive portal qui demande de rien faire si pas sûr, avec lien pour visiter le site une fois sortie du portail
- ajout check si capteur connectés ou valeurs ok (ajout filtres sur capteurs pour lisser valeurs)

### lastly

- Afficher historique des démarrages et voir la raison (watchdog, power-on, crash, etc.)
- utiliser les watchdog interne en plus du externe
- avoir rappel régulier sur app pour nettoyage filtre (à valider pour l'enlever) -> à aussi avoir dans futur version intégrée dans HA
- Pour publier des données par Internet, cf. [achat carte sim](https://www.thingsmobile.com/business/shop) et [tuto](https://randomnerdtutorials.com/esp32-sim800l-publish-data-to-cloud/) (3 façons : juste wifi esp32, `wifi esp32 + alertes par sms`, wifi esp32 uniquement pour debug serial par ex et serveur web qui communique avec esp via carte sim pour toutes les fonctionnalités)\*

\*permet de recevoir des notifs par SMS ou eMAIL si défaut rencontré ou même de pouvoir accéder à l'interface de partout avec un serveur web externe

# Features souhaitées d'un contrôleur intelligent

- sécurité :
  - détection du débit (comme ça message erreur au cas où pompe arrosage ne s'allume pas, tuyau est pincé/débranché, vanne est bloqué, plusieurs vannes sont ouvertes, ...)
  - freeze prevention (températures proches de 0°C)
  - water hammer reduction (by opening next valve 10s prior to its scheduled time, and each valve a few seconds after the other)
- plannificateur optimal :
  - utilise les secondes au lieu des minutes
  - cycle and soak
  - skip by rain/humidity sensor
  - skip by chance of rain (from internet local weather)
  - scheduling on sunrise/sunset
  - seasonal adjustment
- praticité :
  - enable/disable zone
  - skip/delay for x days manually
  - synchro de l’heure avec horloge internet (inclus le décalage automatique de l’horaire)
  - graphiques (total water usage de type histogramme, et pour chaque capteurs, ...)
  - historique des actions (log)
  - arrosage à la main avec tuyau souple et pistolet (utilise une vanne 12v en plus)
  - intégration dans Home Assistant par msgs MQTT (pour voir état des capteurs dont niveau cuve et avoir des notifs en cas d’erreur, d'inactivité, ...)

# Possible improvements

## Very useful

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
