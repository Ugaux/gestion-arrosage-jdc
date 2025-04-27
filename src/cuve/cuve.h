#ifndef _CUVE_H_
#define _CUVE_H_

#include <string.h>

void MarcheRemplissageCuve();
void ArretRemplissageCuve();
void MarcheArrosage();
void ArretArrosage();

class Cuve {
public:
  enum class Etat {
    DEFAUT,
    VIDE,
    INTERMEDIAIRE,
    PLEINE,
  };
  Cuve();
  void   setup();
  void   run();
  String messageDefaut = "";

private:
  void transitionVers(Etat etat);
  void actionEnEntrantDansEtat(Etat etat);

  unsigned long timerDelaiCapteurBas_  = 0;
  unsigned long timerDelaiRemplissage_ = 0;
  Etat          etat_;
  Etat          dernierEtat_;
};

extern Cuve cuve;

#endif
