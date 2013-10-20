//---------------------------------------------------------------------------
#ifndef uMarkH
#define uMarkH

namespace SHEdit
{
  class Format;
  //---------------------------------------------------------------------------
  class Mark
  {
    public:
      Mark(Format * format, bool begin, short pos, Mark * newchild, Mark ** parent);
      Format * format;
      bool begin;
      short pos;
      Mark * mark;
      Mark ** parent;
  };
}
//---------------------------------------------------------------------------

#endif




