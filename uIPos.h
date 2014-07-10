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

  /*!
   * IPos 
   * ===
   *  IPos is base class for all positioning elements of this project. IPos stores a relative location in buffer using the line number and line position. IPos and Iterator (as its descendant) can work in 2 modes - a "full" mode and a positionless mode. Its full mode features are dependent on valid IPos::buffer pointer and on correct initialization of IPos::linenum and IPos::line. The "full mode" IPos and Iter register themselves within the buffer, and later are guaranteed to maintain their positions through all editations of buffer (that means that as long as they are initialized with correct information, it's linenum will remain correct even if it's value has changed). Positionless IPos and Iter classes are not guaranteed anything, but may be useful for quick local changes. 
   *
   * IPos itself is just a relative-location storing class, intended for marking of statical positions. If you need to move and access data, you should use Iter class.
   * */

  class IPos
  {
    public:
      enum IPType{iptPos, iptIter, iptMark};

      IPos(const IPos& ipos);
      IPos(Buffer * buffer, NSpan * line, int linenum, int pos);
      ~IPos();

      Buffer * buffer;  /*!< A pointer to buffer that owns data pointed to by IPos instance. If buffer is null, then IPos or Iter respectively works in a "position less mode", and in that case does not guarantee to remain valid*/
      NSpan * line; /*!< Pointer to the befinning of current line. Positioning relies on correct line and linenum initialization. Other members can be recalculated */
      int linenum; /*!< Line number of relative line. Is correct as long as it was correctly initialized and the buffer pointer is valid */
      int pos; /*!< Left offset. Does treat tabs as characters. */
      IPType type; /*!< Determines type of object - allows IPos casted IMark/Iter comparisons without requiring dynamic casting. */

      virtual void Update(); /*!< Is called when structure of underlying buffer changed, to allow descendants to update their links to the physical structure. */
      virtual void UpdatePos(); /*!< Basically same as Update(), but handles repositioning */
      virtual void RecalcPos(); /*!< Recalculates IPos::pos, according to descendant's own positioning */

      bool operator==(const IPos& p);

      static bool Compare(const IPos*& a, const IPos*& b); /*!< Returns true if "a" is to be considered to go befor "b" in buffer. For descendants should be also able to take into acount their positioning needs (i.e. when the position itself is the same, but some markup should be processed before other; its implemented this way to work with template asociative containers that do not allow comparing against other then template type) */
      struct compare
      {
        bool operator()(const IPos* a, const IPos* b) const;
      };

      IMark* IMarkupBegin(SHEdit::Format * format); /*!< Places Iterator handled mark at IPos's position. For formatting overview see \ref index . For more specific info see IMark class */
      IMark* IMarkupEnd(SHEdit::Format * format);/*!< Places Iterator handled mark at IPos's position. For formatting overview see \ref index . For more specific info see IMark class */
      FontStyle ReconstructIMarkFontStyle();/*!< Returns current FontStyle determined by Iterator handled markup. For formatting overview see \ref index . For more specific info see IMark class */
  };
}
//---------------------------------------------------------------------------
#endif
