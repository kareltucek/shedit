//------------------------------------------------------------

#ifndef uIterH
#define uIterH

#include "uIPos.h"

namespace SHEdit
{
  class Span;
  class NSpan;
  class Mark;
  class IMark;
  class Format;
  class Buffer;
  class FontStyle;
  //---------------------------------------------------------------------------
  class Iter : public IPos
  {
    public:
      Iter(NSpan * line);
      Iter(NSpan * line, int linenum, int pos, Buffer * buffer);
      Iter(int offset, Span * word, NSpan * line, Buffer * buffer, int linenum = -1);
      ~Iter();

      Span * word;
      int offset;
      wchar_t * ptr;

      int nextimark;
      int nextimarkln;

      bool GoLine(bool allowEnd = false);
      bool GoLineEnd();
      bool GoLineStart();
      bool RevLine();
      bool RevLineBegin();
      bool GoWord();
      bool RevWord();
      bool RevWordBegin();
      bool GoChar();
      bool RevChar();

      wchar_t GetNextChar();

      int GetLeftOffset();
      void GoByOffset(int chars);
      void GoBy(int chars);

      void MarkupBegin(SHEdit::Format * format);
      void MarkupEnd(SHEdit::Format * format);
      void MarkupRem(SHEdit::Format * format);

      Iter * Duplicate();
      virtual void Update();
      virtual void UpdatePos();
      virtual void RecalcPos();
      void UpdateNextImark();

      void GoToLine(int line);

      bool operator==(const Iter& itr);
      bool operator!=(const Iter& itr);
      wchar_t& operator++();
      wchar_t& operator--();
  };
}
//---------------------------------------------------------------------------
#endif
