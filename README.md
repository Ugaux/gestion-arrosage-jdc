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
- Simplification structure de /src

## 🔧 To clarify / questions

- codage en natif sur windows avec unit testing + OTA upload + debug dans platform IO et release ensuite

- check si quand esp est down dans console log je vois websocket error
- conversion code en API pour dev web plus facile (avec esp connecté en wifi)
- maj avec beau code papa avec sidebar(page home avec status cuve/vannes/capteurs/... et arrosage + settings)
- Ajouts capteurs (débit + présence eau pompe remplissage cuve)
- réduire envoi radio avec biblioteque (RadioHead qui a send non bloquant) pr code papa car ajoute pb de reactivité lors d'un appui sur bouton avec arrosage en cours (seulement besoin d'envoyer 3-5 commandes, pas besoin de le faire pdt 10ms) https://chatgpt.com/c/682541fa-2364-800e-8035-e06e5f3cb1cc

- Pb vanne si pas de planning de fait
- Loop main par rapport à 0 de chaque minute et non random
- empêcher plus d'1 vannes (avoir algo qui exécute chaque watering du jour successivement après heure fixe décidée dans .ini)
- button released logic pour écran, du genre act on press pour la navigation et release pour démarrer arrosage (dont si arrosage manuel en cours, pouvoir uniquement arrêter celui là, ne rien afficher d'autre)
- bouton dans web pour sync heure avec tel

- Edition zones sur web
- Défilement sur ecran oled pr texte trop long
- Garder info de "en remplissage cuve" si système s'éteint, pr continuer
- Captive portal en mode AP ? cf. notes
- Captive portal qui demande de rien faire si pas sûr, avec lien pour visiter le site une fois sortie du portail
- ajout check si capteur connectés ou valeurs ok

- avoir rappel régulier sur app pour nettoyage filtre (à valider pour l'enlever) -> à aussi avoir dans futur version intégrée dans HA
- Vérif dans watering.cpp : "if (at + (m_duration \* 60)" et "if (at + 10 < now)"
- Ajout si appuie successif dans les 5 1ere secondes, augmenter temps total (commencer avec 10min, puis 20min, ...)
- Pour publier des données par Internet, cf. [achat carte sim](https://www.thingsmobile.com/business/shop) et [tuto](https://randomnerdtutorials.com/esp32-sim800l-publish-data-to-cloud/) (3 façons : juste wifi esp32, `wifi esp32 + alertes par sms`, wifi esp32 uniquement pour debug serial par ex et serveur web qui communique avec esp via carte sim pour toutes les fonctionnalités)\*

\*permet de recevoir des notifs par SMS ou eMAIL si défaut rencontré ou même de pouvoir accéder à l'interface de partout avec un serveur web externe

# Features d'un contrôleur intelligent souhaitées

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

# Pinout

interrupteur général pour couper alim

Indicateurs lumineux (ou garder écran?) :

- led verte pour power on
- led verte pour network on
- system state avec led rouge/verte (indication si erreur/bug)
- led verte pour arrosage en cours

watchdog externe ou interne

Capteurs :

- _flow_
- soil humidity + soil temp ?
- comme station météo : air temp/humidity + pression atmo + anémomètre/girouette + luxmètre + pluviomètre
- niveaux haut/bas cuve &#8594; à remplacer par capteur ultrason
- _présence arrivée d'eau sur tuyau remplissage cuve_
- courant pompe arrosage (if pump is rapidly cycling ON/OFF → pb pression air du ballon, pb pression du circuit d'eau, permet d'analyser si le ballon de 50L est trop petit, ...)
- courant pr état des solenoide des vannes

2 boutons pr controle ecran
Oled screen
&#8594; à retirer

RTC clock
433Hz radio emitter
8 vannes 24v ac
Alim 12v avec regulateur 5v

Bouton avec led
afficheur 7 segments
une vanne 12v
buzzer

Analyse des GPIO restant pr flow arrosage, 3 en plus pr cuve, 4 vannes

- 36 input only - moisture
- 39 input only - flow
- 33
- 32
- 27 - input cuve 0
- 26 - input cuve 2
- 25 - r5
- 19 - input cuve 3
- 18 - r4
- 17 - r3
- 16 - r2
- 15 - r1
- 14 - r7
- 13 - r8
- 12 Output only (should be at low for flashing)
- 5 - input cuve 1
- 4 - r6
- 2 output only (should be at low for flashing)

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
