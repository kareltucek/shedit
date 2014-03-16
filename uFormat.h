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
  /*!
   * FontStyle
   * ---------
   * FontStyle represents how font should look like. It does not neccessarily specify all properties of font - just those that are supposed to be changed - null/empty values are properties that are supposed to remain formatted according to lower priority markup (i.e. as syntax highlight). 
   *
   * For more information on formatting see \ref index and Format class documentation.
   * */
  class FontStyle
  {
    public:
      FontStyle();
      FontStyle(TColor * foreground, TColor * background, TFontStyles style);
      FontStyle(TColor * foreground, TColor * background);
      FontStyle(TColor * foreground);
      TColor * foreground;
      TColor * background;
      TFontStyles style;
      bool operator==(const FontStyle& f);
      bool operator!=(const FontStyle& f);
      FontStyle& operator+=(const FontStyle& f);
      FontStyle& operator+=(const Format& f);
      FontStyle& operator=(const FontStyle& f);
      FontStyle& operator=(const Format& f);
  };
  //---------------------------------------------------------------------------
  /*!
   * Fonts, text formatting, markup system
   * =====================================
   *  The basic thing to notice is:
   * - FontStyle represents how a font should be formatted. 
   * - Format represents how the buffer is formatted using its FontStyle.
   *
   * That means that while Format inherits FontStyle, FontStyle class is used just to pass information while Format is used to represent it. 
   *
   * Format
   * ------
   * Formats are applied to buffer using Iter's methods to start and end a position where markup should be applied. Those methods do place markup where it is required, but besides that it also registers all marks within their Format class, which allows to clear all markup easily at once. As mentioned at \ref index there are 2 different markup systems. For details about them see Mark and IMark classes (those represent buffer positions where markup starts and where it ends).
   * */
  class Format : public FontStyle
  {
    private:
      std::list< Stack<Mark >::Node* > marks;
      std::set<IPos*, IMark::compare> imarks; //Actually stores IMark pointers casted to pos for comparison

    public:
      friend class Iter;
      Format();
      Format(TColor * foreground, TColor * background);
      Format(TColor * foreground, TColor * background, TFontStyles style);
      ~Format();
      void Remove(Stack<Mark>::Node * mark);
      void Add(Stack<Mark>::Node * mark);
      void RemoveIM(IMark * mark);
      void AddIM(IMark * mark);
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
