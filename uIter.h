//------------------------------------------------------------

#ifndef uIterH
#define uIterH

namespace SHEdit
{
  class Span;
  class NSpan;
  class Mark;
  class Format;
  class Buffer;

  //---------------------------------------------------------------------------
  class Iter
  {
    public:
      Iter(NSpan * line);
      Iter(NSpan * line, int linenum, int pos, Buffer * buffer);
      Iter(int offset, Span * word, NSpan * line, Buffer * buffer, int linenum = -1);
      ~Iter();

      Span * word;
      NSpan * line;
      Buffer * buffer;
      int offset;
      int linenum;
      int pos;
      wchar_t * ptr;

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

      static void MarkupBegin(Mark ** at, int pos, bool begin, Format * format);
      void MarkupBegin(SHEdit::Format * format);
      void MarkupEnd(SHEdit::Format * format);
      void MarkupRem(SHEdit::Format * format);

      Iter * Duplicate();
      void Update();
      void UpdatePos();
      void RecalcPos();

      bool operator==(const Iter& itr);
      bool operator!=(const Iter& itr);
      wchar_t& operator++();
      wchar_t& operator--();
  };
}
//---------------------------------------------------------------------------
#endif
