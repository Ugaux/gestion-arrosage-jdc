#include "Frequency.h"

bool toChar(Frequency freq, unsigned char &c) {

  switch (freq) {

    case Frequency::EveryDay:
      c = '*';
      return true;
    case Frequency::EvenDays:
      c = 'e';
      return true;
    case Frequency::OddDays:
      c = 'o';
      return true;
    case Frequency::SpecificDays:
      c = 's';
      return true;
  }

  return false;
}

bool fromChar(unsigned char c, Frequency &freq) {

  switch (c) {

    case '*':
      freq = Frequency::EveryDay;
      return true;
    case 'e':
      freq = Frequency::EvenDays;
      return true;
    case 'o':
      freq = Frequency::OddDays;
      return true;
    case 's':
      freq = Frequency::SpecificDays;
      return true;

    default:
      return false;
  }
}
