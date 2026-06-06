#pragma once

#ifdef PLATFORM_NATIVE
#pragma message("Compiling this file only in Native env!")
#endif

class IValve {
public:
  virtual void open()  = 0;
  virtual void close() = 0;
  virtual ~IValve()    = default;
};
