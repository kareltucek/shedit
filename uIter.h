//------------------------------------------------------------

#ifndef uIterH
#define uIterH

namespace SHEdit
{
  class Span;
  class NSpan;
  class Mark;
  class Format;
  //---------------------------------------------------------------------------
  class Iter
  {
    public:
      Iter(NSpan * line);
      Iter(int offset, Span * word, NSpan * line);
      ~Iter();

      Span * word;
      NSpan * line;
      int offset;
      wchar_t * ptr;

      bool GoLine(bool allowEnd = false);
      bool RevLine();
      bool RevLineBegin();
      bool GoWord();
      bool RevWord();
      bool RevWordBegin();
      bool GoChar();
      bool RevChar();

      int GetLeftOffset();
      void GoBy(int chars);

      static void MarkupBegin(Mark ** at, int pos, bool begin, Format * format);
      void MarkupBegin(SHEdit::Format * format);
      void MarkupEnd(SHEdit::Format * format);
      void MarkupRem(SHEdit::Format * format);

      Iter * Duplicate();
      void Update();

      bool operator==(const Iter& itr);
      bool operator!=(const Iter& itr);
      wchar_t& operator++();
      wchar_t& operator--();
  };
}
//---------------------------------------------------------------------------
#endif
