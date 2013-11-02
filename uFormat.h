#ifndef uFormatH
#define uFormatH

#include <vcl.h>
#include <list>

namespace SHEdit
{
  class Buffer;
  class Mark;
  class Format;
  //---------------------------------------------------------------------------
  class FontStyle
  {
    public:
      FontStyle();
      FontStyle(TColor * foreground, TColor * background);
      FontStyle(TColor * foreground);
      TColor * foreground;
      TColor * background;
      bool operator==(const FontStyle& f);
      bool operator!=(const FontStyle& f);
      FontStyle& operator=(const FontStyle& f);
      FontStyle& operator=(const Format& f);
  };
  //---------------------------------------------------------------------------
  class Format : public FontStyle
  {
    private:
      std::list<Mark*> marks;
    public:
      Format();
      Format(TColor * foreground, TColor * background);
      ~Format();
      void Remove(Mark * mark);
      void Add(Mark * mark);
      void RemoveAllMarks();
      bool operator==(const Format& f);
      bool operator!=(const Format& f);
      Format& operator=(const Format& f);
  };
}
//---------------------------------------------------------------------------
#endif
