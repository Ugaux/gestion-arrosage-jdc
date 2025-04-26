#ifndef _CUVE_H_
#define _CUVE_H_

void MarcheRemplissageCuve();
void ArretRemplissageCuve();
void MarcheArrosage(void);
void ArretArrosage(void);

class Cuve {
public:
  Cuve();
  void setup();
  void run();

private:
  unsigned long timer             = 0;
  bool          pompeCuveEnMarche = false;
};

extern Cuve cuve;

#endif
