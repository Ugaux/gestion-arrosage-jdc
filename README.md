# Description projet

Code fonctionnant sur un ESP32 permettant la gestion de l'arrosage pour le Jardin du Ciel à Vitabox à Algolsheim.

Fonctionnalitées :

- remplissage automatique de la cuve de 1000L (pilotage pompe par radio 433 MHz)
- arrosage automatique :
  - paramétrage du planning par interface web ou manuellement par boutons de navigation
  - pilotage de 4 vannes 12V pour choix de la zone
  - détection de l'humidité du sol pour décision d'arrosage
  - pilotage pompe par radio 433 MHz
- sauvegarde de l'heure en cas de coupure de courant (par horloge externe RTC_DS3231)

# Tâches

✔️ bug import dans SPIFFSIniFile.h avec #include <SPIFFS.h>\
✔️ copier upgrade cuve\
❌ RTC détection plus de pile + si déconnection du spi/5v/gnd (doit reset puis dans setup mettre msg erreur)\
❌ affichage erreurs sur ecran + sur interface html?\
❌ defilement sur ecran oled pr texte trop long\
❌ pb vanne si pas de planning de fait\
❌ pb capteur humidité\
❌ dans watering.cpp : "if (at + (m_duration \* 60)"\
❌ changement design interface web cf. [lien](https://www.google.com/search?sca_esv=9373e67870ea7967&q=esp32+good+looking+web+interface&udm=2fbs=ABzOT_CWdhQLP1FcmU5B0fn3xuWpA-dk4wpBWOGsoR7DG5zJBkzPWUS0OtApxR2914vrjk4ZqZZ4I2IkJifuoUeV0iQtITiOPPo9tDzmt9ZPGYJiIba3ipclDVbOjJlvTbgEP2s-bkOIhr5ELgbQI8I7zKhriYCgRXaYljMf-YpaNgLRzy2fJ38VbFwBTF_D5ZCA5_SutZQD&sa=X&ved=2ahUKEwjA_sOx2vWMAxUFKvsDHcgkMJkQtKgLegQIIRAB&biw=1920&bih=919&dpr=1#vhid=uo5Y2WpNP6a2vM&vssid=mosaic)\
❌ changement systematique pour sol sec uniquement\
❌ Publier des données par Internet cf. [achat carte sim](https://www.thingsmobile.com/business/shop) et [tuto](https://randomnerdtutorials.com/esp32-sim800l-publish-data-to-cloud/)\*

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
$ arduino-cli compile -v ~/gestion_arrosage_jdc --build-path ~/gestion_arrosage_jdc/build
```

# Autres

Projet [wokwi](https://wokwi.com/projects/429204852876880897) en ligne pour simulation de la cuve avec un ESP32

arduino-cli board details -b esp32:esp32:esp32da
arduino-cli upload \\wsl.localhost\Ubuntu-24.04\home\user\gestion_arrosage_jdc --build-path \\wsl.localhost\Ubuntu-24.04\home\user\gestion_arrosage_jdc\build -v

arduino-cli compile ~/gestion_arrosage_jdc --build-path ~/gestion_arrosage_jdc/build -v

arduino-cli compile D:\Stockage\Downloads\MyProject --build-path D:\Stockage\Downloads\MyProject/build -v
