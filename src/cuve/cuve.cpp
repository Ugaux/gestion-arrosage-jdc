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

#include <Arduino.h>
#include <RCSwitch.h>

#include "cuve.h"

#define DEBUG false  // Afficher une aide sur la console (débogage)

RCSwitch mySwitch = RCSwitch();

int captNivHaut  = 19;
int NivHaut      = 0;
int captNivBas   = 5;
int NivBas       = 0;
int Liquid_level = 0;

Cuve::Cuve() {
}

// the setup function runs once when you press reset or power the board
void Cuve::setup() {
  pinMode(23, OUTPUT);          // sets the digital pin 23 as output
  mySwitch.enableTransmit(23);  // data de l'émetteur sur broche Digital 23
  // bien respecter l'ordre de ces 3 instructions qui suivent :
  mySwitch.setProtocol(1);       // à remplacer par votre valeur de protocol
  mySwitch.setPulseLength(346);  // à remplacer par votre valeur de PulseLength
  mySwitch.setRepeatTransmit(5);

  pinMode(captNivHaut, INPUT_PULLUP);  // sensor pin
  pinMode(captNivBas, INPUT_PULLUP);   // sensor pin

  transitionVers(Etat::INTERMEDIAIRE);  // Etat initial de la machine d'état

  if (DEBUG)
    Serial.println("\n✔️ SETUP TERMINE -> DEBUT DU LOOP...");
}

// the loop function runs over and over again forever
void Cuve::run() {
  NivHaut = digitalRead(captNivHaut);
  NivBas  = digitalRead(captNivBas);

  switch (etat_) {

    case Etat::VIDE:
      if (NivHaut) {
        messageDefaut = "capteur haut à 1 avec cuve vide";
        transitionVers(Etat::DEFAUT);
        break;
      }
      if (NivBas) {
        timerDelaiRemplissage_ = millis();
        transitionVers(Etat::INTERMEDIAIRE);
        break;
      }
      if (millis() - timerDelaiCapteurBas_ > 60000) {
        messageDefaut = "capteur bas toujours à 0 malgré remplissage";
        transitionVers(Etat::DEFAUT);
        break;
      }
      break;

    case Etat::INTERMEDIAIRE:
      if (NivHaut) {
        transitionVers(Etat::PLEINE);
        break;
      }
      if (!NivBas) {
        transitionVers(Etat::VIDE);
        break;
      }
      if (dernierEtat_ == Etat::VIDE and millis() - timerDelaiRemplissage_ > 1200000) {
        messageDefaut = "capteur haut toujours à 0 malgré remplissage";
        transitionVers(Etat::DEFAUT);
        break;
      }
      break;

    case Etat::PLEINE:
      if (!NivBas) {
        messageDefaut = "capteur bas à 0 avec cuve pleine";
        transitionVers(Etat::DEFAUT);
        break;
      }
      if (!NivHaut) {
        transitionVers(Etat::INTERMEDIAIRE);
        break;
      }
      break;

    case Etat::DEFAUT:
      // No transition here
      break;
  }
}

void Cuve::actionEnEntrantDansEtat(Etat etat) {

  switch (etat) {

    case Etat::VIDE:
      if (DEBUG)
        Serial.println("\n🔲 CUVE VIDE");
      MarcheRemplissageCuve();
      timerDelaiCapteurBas_ = millis();
      break;

    case Etat::INTERMEDIAIRE:
      if (DEBUG)
        Serial.println("\n🔲 CUVE INTERMEDIAIRE");
      break;

    case Etat::PLEINE:
      if (DEBUG)
        Serial.println("\n🔲 CUVE PLEINE");
      ArretRemplissageCuve();
      break;

    case Etat::DEFAUT:
      if (DEBUG) {
        Serial.print("\n🛑 Etat::DEFAUT: ");
        Serial.println(messageDefaut);
      }
      ArretRemplissageCuve();
      ArretArrosage();
      if (DEBUG)
        Serial.print("\n⚠️ REGLER PB PUIS RESET L'ESP32");
      break;
  }
}

void Cuve::transitionVers(Etat etat) {
  dernierEtat_ = etat_;
  etat_        = etat;

  actionEnEntrantDansEtat(etat);
}

void MarcheArrosage() {
  if (DEBUG)
    Serial.println("Envoi commande Arret Arrosage :");
  //if (etat_ == Etat::INTERMEDIAIRE || etat_ == Etat::PLEINE) {
  unsigned long sendTime = millis();
  while (millis() - sendTime < 20) {
    mySwitch.send("101000000110101010110100");  // = BOUTON "C" télécommande : Arrosage au jet
    if (DEBUG)
      delay(1);
  }
  Serial.println();
  //}
}

void ArretArrosage() {
  if (DEBUG)
    Serial.println("Envoi commande Arret Arrosage :");
  unsigned long sendTime = millis();
  while (millis() - sendTime < 20) {
    mySwitch.send("101000000110101010110001");  // = BOUTON "D" télécommande : Arrêt arrosage au jet
    if (DEBUG)
      delay(1);
  }
  Serial.println();
}
void MarcheRemplissageCuve() {
  if (DEBUG)
    Serial.println("Envoi commande Marche Remplissage Cuve :");
  unsigned long sendTime = millis();
  while (millis() - sendTime < 20) {
    mySwitch.send("101000000110101010110010");  // = BOUTON "A" télécommande : Remplissage cuve
    if (DEBUG)
      delay(1);
  }
  Serial.println();
}

void ArretRemplissageCuve() {
  if (DEBUG)
    Serial.println("Envoi commande Arret Remplissage Cuve :");
  unsigned long sendTime = millis();
  while (millis() - sendTime < 20) {
    mySwitch.send("101000000110101010111000");  // = BOUTON "B" télécommande : Arrêt Remplissage cuve
    if (DEBUG)
      delay(1);
  }
  Serial.println();
}
