#ifndef _CUVE_H_
#define _CUVE_H_

#include <Arduino.h>
#include <RCSwitch.h>

#define CAPT_NIV_HAUT 19
#define CAPT_NIV_BAS  5

class Cuve {
public:
  enum class Etat {
    DEFAUT,
    VIDE,
    INTERMEDIAIRE,
    PLEINE,
  };
  Cuve(RCSwitch &switchRef);
  void   setup(bool modeContinu);
  void   run();
  Etat   getCurrentState() { return etat_; }
  String getCurrentDefault() { return messageDefaut; }
  String getCurrentStateStr() { return etatStr; }

private:
  void      transitionVers(Etat etat);
  void      actionEnEntrantDansEtat(Etat etat);
  void      resetTimerAllumagePompe();
  void      relaiRemplissageON();
  void      relaiRemplissageOFF();
  int       nivHaut;
  int       nivBas;
  RCSwitch &radioCmdRef;

  unsigned long timerDelaiCapteurBas_  = 0;
  unsigned long timerDelaiRemplissage_ = 0;
  Etat          etat_;
  Etat          dernierEtat_;
  String        messageDefaut = "";
  String        etatStr       = "";
  bool          modeContinu_;
  bool          cuveEnRemplissage_;
};

extern Cuve cuve;

#endif
