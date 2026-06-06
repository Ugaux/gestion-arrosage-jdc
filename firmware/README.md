# ESP32 pinout

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
