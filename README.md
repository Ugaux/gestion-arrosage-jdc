# Description projet

Code fonctionnant sur un ESP32 permettant la gestion de l'arrosage pour le Jardin du Ciel à Vitabox à Algolsheim.

Fonctionnalitées :

- remplissage automatique de la cuve de 1000L (pilotage pompe par radio 433 MHz)
- arrosage automatique :
  - paramétrage du planning par interface web ou manuellement par boutons de navigation
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
- Simplifier structure de /src


\*permet de recevoir des notifs par SMS ou eMAIL si défaut rencontré ou même de pouvoir accéder à l'interface de partout avec un serveur web externe

# Compilation rapide avec WSL et Ubuntu 24.04.01 LTS

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
