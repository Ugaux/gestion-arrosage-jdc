/*
  XKC-Y25-V
  Vcc = marron
  Mode = noir (mettre sur le 0V pour inverser la sortie)
  Gnd = bleu
  Out = jaune

  GPIO05 CAPTEUR BAS
  GPIO19 CAPTEUR HAUT
  GPIO23 CDE RADIO 433 MHZ

*/
#include "Arduino.h"
#include "cuve.h"

#define DEBUG false         // Afficher une aide sur la console (débogage)

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

int captNivHaut = 19;
int NivHaut = 0;
int captNivBas = 5;
int NivBas = 0;
int Liquid_level = 0;
Cuve::Cuve() {
}

// the setup function runs once when you press reset or power the board
void Cuve::setup() {
  timer = millis();
  pinMode(33, OUTPUT);          // sets the digital pin 33 as output LED verte "arrosage en cours"
  //  pinMode(32, OUTPUT);          // sets the digital pin 32 as output

  pinMode(23, OUTPUT);          // sets the digital pin 23 as output
  mySwitch.enableTransmit(23);  // data de l'émetteur sur broche Digital 10 de l'Arduino
  // bien respecter l'ordre de ces 3 instructions qui suivent :
  mySwitch.setProtocol(1);       // à remplacer par votre valeur de protocol
  mySwitch.setPulseLength(346);  // à remplacer par votre valeur de PulseLength
  mySwitch.setRepeatTransmit(5);

  pinMode(captNivHaut, INPUT_PULLUP);  // sensor pin
  pinMode(captNivBas, INPUT_PULLUP);   // sensor pin

}

// the loop function runs over and over again forever
void Cuve::run() {
  NivHaut = digitalRead(captNivHaut);
  NivBas = digitalRead(captNivBas);
  //delay (500);

  if (NivBas == 0 && NivHaut == 1) {
    ArretRemplissageCuve();
    if (DEBUG)
    { Serial.print("PB DE CAPTEUR BAS: ");
      Serial.print(NivBas, DEC);
      Serial.print(" - Arrêt Remplissage Cuve ('B' télécommande) - Cuve pleine: ");
      Serial.println(NivHaut, DEC);
      delay(300);
    }
  }

  if (NivBas == 1 && NivHaut == 1) {
    ArretRemplissageCuve();
    if (DEBUG)
    { Serial.print("NivBas: ");
      Serial.print(NivBas, DEC);
      Serial.print(" - NivHaut: ");
      Serial.print(NivHaut, DEC);
      Serial.print(" - Arrêt Remplissage Cuve ('B' télécommande) - Cuve pleine: ");
      Serial.println(NivHaut, DEC);
      delay(300);
    }
  }

  // Si les deux niveaux sont bas et que la pompe n'est pas déjà en marche
  if (NivBas == 0 && NivHaut == 0 && !pompeCuveEnMarche) {
    MarcheRemplissageCuve();
    pompeCuveEnMarche = true;
    timer = millis();
  }
   // Si la pompe est en marche, on vérifie si 60s se sont écoulées
  if (pompeCuveEnMarche && (millis() - timer > 60000)) {
    // On relit les capteurs
      if (NivBas == 0 && NivHaut == 0) {
        ArretArrosage();
            ArretRemplissageCuve();
                pompeCuveEnMarche = false;
      }
    }
    if (DEBUG)
    { Serial.print("NivBas: ");
      Serial.print(NivBas, DEC);
      Serial.print(" - NivHaut: ");
      Serial.print(NivHaut, DEC);
      Serial.print(" - Marche Remplissage Cuve ('A' télécommande) - Cuve vide: ");
      Serial.println(NivHaut, DEC);
      delay(300);
    }
  }

void MarcheArrosage(void) {
  mySwitch.send("101000000110101010110100");  // = BOUTON "C" télécommande : Arrosage au jet
}

void ArretArrosage(void) {
  mySwitch.send("101000000110101010110001");  // = BOUTON "D" télécommande : Arrêt arrosage au jet
}


void MarcheRemplissageCuve() {
  mySwitch.send("101000000110101010110010");  // = BOUTON "A" télécommande : Remplissage cuve
}

void ArretRemplissageCuve() {
  mySwitch.send("101000000110101010111000");  // = BOUTON "B" télécommande : Arrêt Remplissage cuve
}
