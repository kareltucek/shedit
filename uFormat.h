#ifndef uFormatH
#define uFormatH

#include <vcl.h>
#include <list>
#include <set>
#include "uStack.h"
#include "uMark.h"

namespace SHEdit
{
  class Buffer;
  class Mark;
  class IMark;
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
      FontStyle& operator+=(const FontStyle& f);
      FontStyle& operator+=(const Format& f);
      FontStyle& operator=(const FontStyle& f);
      FontStyle& operator=(const Format& f);
  };
  //---------------------------------------------------------------------------
  class Format : public FontStyle
  {
    private:
      std::list< Stack<Mark >::Node* > marks;
      std::set<IPos*, IMark::compare> imarks; //Actually stores IMark pointers casted to pos for comparison

    public:
      friend class Iter;
      Format();
      Format(TColor * foreground, TColor * background);
      ~Format();
      void Remove(Stack<Mark>::Node * mark);
      void Add(Stack<Mark>::Node * mark);
      void RemoveAllMarks();
      bool operator==(const Format& f);
      bool operator!=(const Format& f);
      Format& operator+=(const Format& f);
      Format& operator=(const Format& f);

      IMark * GetMarkBefore(IPos * ipos);
  };
}
//---------------------------------------------------------------------------
#endif
