//---------------------------------------------------------------------------

#ifndef uIPosH
#define uIPosH
//---------------------------------------------------------------------------

namespace SHEdit
{
  class Buffer;
  class NSpan;
  class IMark;
  class Format;
  class FontStyle;

  class IPos
  {
    public:
      enum IPType{iptPos, iptIter, iptMark};

      IPos(Buffer * buffer, NSpan * line, int linenum, int pos);
      ~IPos();

      Buffer * buffer;
      NSpan * line;
      int linenum;
      int pos;
      IPType type;

      virtual void Update();
      virtual void UpdatePos();
      virtual void RecalcPos();

      static bool Compare(const IPos*& a, const IPos*& b);
      struct compare
      {
        bool operator()(const IPos* a, const IPos* b) const;
      };

      IMark* IMarkupBegin(SHEdit::Format * format);
      IMark* IMarkupEnd(SHEdit::Format * format);
      FontStyle ReconstructIMarkFontStyle();
  };
}
//---------------------------------------------------------------------------
#endif
