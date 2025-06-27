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

#include "cuve.h"

#define DEBUG false  // Afficher une aide sur la console (débogage)

Cuve::Cuve(RCSwitch &radioCmd) : radioCmdRef(radioCmd) {}

/// @brief The setup function runs once when you press reset or power the board//
/// @param modeContinu Si true, envoie un signal radio à chaque appel de run() pour reset le timer du module
// relai radio externe en 230V de la pompe de remplissage. Si false, utilise un relai qui est activé une fois
// lors du passage à l'état vide et éteint à l'état plein. Dans les deux cas, lors d'une coupure de l'ESP32,
// la pompe s'arrêtera aussi (instantanément avec le relai, et en moins de 5s avec le module radio)
void Cuve::setup(bool modeContinu) {
  pinMode(CAPT_NIV_HAUT, INPUT_PULLUP);  // sensor pin
  pinMode(CAPT_NIV_BAS, INPUT_PULLUP);   // sensor pin

  transitionVers(Etat::INTERMEDIAIRE);  // Etat initial de la machine d'état
  etatStr = "CUVE INTERMEDIAIRE";

  if (DEBUG)
    Serial.println("\n✔️ SETUP TERMINE -> DEBUT DU LOOP...");

  modeContinu_ = modeContinu;
}

void Cuve::run() {
  nivHaut = digitalRead(CAPT_NIV_HAUT);
  nivBas  = digitalRead(CAPT_NIV_BAS);

  switch (etat_) {

    case Etat::VIDE:
      if (nivHaut) {
        messageDefaut = "niv h & cuve vide";  //"capteur haut à 1 avec cuve vide";
        transitionVers(Etat::DEFAUT);
        break;
      }
      if (nivBas) {
        timerDelaiRemplissage_ = millis();
        transitionVers(Etat::INTERMEDIAIRE);
        break;
      }
      if (millis() - timerDelaiCapteurBas_ > 60000) {
        messageDefaut = "!niv b & remplissage";  //"capteur bas toujours à 0 malgré remplissage";
        transitionVers(Etat::DEFAUT);
        break;
      }
      break;

    case Etat::INTERMEDIAIRE:
      if (nivHaut) {
        transitionVers(Etat::PLEINE);
        break;
      }
      if (!nivBas) {
        transitionVers(Etat::VIDE);
        break;
      }
      if (dernierEtat_ == Etat::VIDE and millis() - timerDelaiRemplissage_ > 2400000) {
        messageDefaut = "!niv h & remplissage";  //"capteur haut toujours à 0 malgré remplissage";
        transitionVers(Etat::DEFAUT);
        break;
      }
      break;

    case Etat::PLEINE:
      if (!nivBas) {
        messageDefaut = "!niv b & cuve pleine";  // "capteur bas à 0 avec cuve pleine";
        transitionVers(Etat::DEFAUT);
        break;
      }
      if (!nivHaut) {
        transitionVers(Etat::INTERMEDIAIRE);
        break;
      }
      break;

    case Etat::DEFAUT:
      // No transition here
      break;
  }

  if (modeContinu_ && cuveEnRemplissage_) { resetTimerAllumagePompe(); }
}

void Cuve::actionEnEntrantDansEtat(Etat etat) {

  switch (etat) {

    case Etat::VIDE:
      if (DEBUG)
        Serial.println("\n🔲 CUVE VIDE");
      etatStr            = "CUVE VIDE";
      cuveEnRemplissage_ = true;
      if (!modeContinu_) { relaiRemplissageON(); }
      timerDelaiCapteurBas_ = millis();
      if (DEBUG)
        Serial.println();
      break;

    case Etat::INTERMEDIAIRE:
      if (DEBUG)
        Serial.println("\n🔲 CUVE INTERMEDIAIRE\n");
      etatStr = "CUVE INTERMEDIAIRE";
      break;

    case Etat::PLEINE:
      if (DEBUG)
        Serial.println("\n🔲 CUVE PLEINE");
      etatStr = "CUVE PLEINE";
      if (!modeContinu_) { relaiRemplissageOFF(); }
      cuveEnRemplissage_ = false;
      if (DEBUG)
        Serial.println();
      break;

    case Etat::DEFAUT:
      if (DEBUG) {
        Serial.print("\n🛑 Etat::DEFAUT: ");
        Serial.println(messageDefaut);
      }
      if (!modeContinu_) { relaiRemplissageOFF(); }
      cuveEnRemplissage_ = false;
      if (DEBUG)
        Serial.println("⚠️ REGLER PB PUIS RESET L'ESP32");
      break;
  }
}

void Cuve::transitionVers(Etat etat) {
  dernierEtat_ = etat_;
  etat_        = etat;

  actionEnEntrantDansEtat(etat);
}

void Cuve::resetTimerAllumagePompe() {
  if (DEBUG)
    Serial.println("Envoi commande allumage pompe cuve");
  unsigned long sendTime = millis();
  while (millis() - sendTime < 10) {
    radioCmdRef.send("101000000110101010110010");  // = BOUTON "A" télécommande : Remplissage cuve
    if (DEBUG)
      delay(1);
  }
}
void Cuve::relaiRemplissageON() {
  if (DEBUG)
    Serial.println("Relai pompe cuve ON");
  // TODO: Ajouter le controle du relai ici
}

void Cuve::relaiRemplissageOFF() {
  if (DEBUG)
    Serial.println("Relai pompe cuve OFF");
  // TODO: Ajouter le controle du relai ici
}
