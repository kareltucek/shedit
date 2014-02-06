//---------------------------------------------------------------------------
#ifndef uMarkH
#define uMarkH

#include "uIPos.h"

namespace SHEdit
{
  class Iter;
  class Format;
  //---------------------------------------------------------------------------
  class Mark
  {
    public:
      Mark(Format * format, bool begin, short pos);
      Format * format;
      bool begin;
      short pos;
  };

  //---------------------------------------------------------------------------
  class IMark : public IPos
  {
    public:
      bool begin;
    Format * format;
    IMark(Format * format, bool begin, IPos * itr);
    ~IMark();
  };

}
//---------------------------------------------------------------------------

#endif




