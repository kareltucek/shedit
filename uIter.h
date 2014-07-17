//------------------------------------------------------------

#ifndef uIterH
#define uIterH

#include <vcl.h>
#include "config.h"
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

  /*!
   * Iter
   * ===
   *  Iter class is a specialization of IPos. While IPos maintains relative position in buffer, not caring of link to it's physical structure, Iter keeps pointers and structure - related info that allow direct acces to the data, and comfortable movement over buffer.
   *
   *  Iter allows to maintain the positionless markup. Regarding Iterator - handled markup, Iterator provides Iter::UpdateNextImark() method that updates the nextimark and nextimarkln member variables that specify the next nearest change (in forward direction) in Iterator handled markup, and thus allows parser to draw these properly. The owner of iterator is responsible for keeping this up-to-date using Iter::UpdateNextImark() method whenever he needs to have it up-to-date.
   *
   * If you wish to use Iter with full positioning, you need to pass correct line, linenum and buffer to constructor. But Iterator will work as well positionlessly.
   *
   * All IPos descendants that are given proper Buffer pointer are registered and taken care of by their Buffer. That means you should always take care of destroying any Iterators you create (or whose duplicate are returned to you), when you dont need them any longer. Not doing so will slow down editing of buffer if you let unused iterators to cumulate.
   * */

  //---------------------------------------------------------------------------
  class Iter : public IPos
  {
    public:
      Iter(const Iter& itr);
      Iter(NSpan * line);
      Iter(NSpan * line, int linenum, int pos, Buffer * buffer);
      Iter(int offset, Span * word, NSpan * line, Buffer * buffer, int linenum = -1);
      ~Iter();

      Span * word; /*!< Pointer to current Span in buffer's structure */
      int offset; /*!< Offset from beginning of current Span */
      wchar_t * ptr; /*!< Direct pointer to current character */

      int nextimark;/*!< See the Iter class info for explanation */
      int nextimarkln;/*!< See the Iter class info for explanation */

      bool GoLine(bool allowEnd = false); /*!< Goes to the pos 0 of next line. That is the offset 0 of first Span after nextline NSpan */
      bool GoLineEnd(); 
      bool GoLineStart(); 
      bool RevLine();
      bool RevLineBegin();
      bool GoWord(); /*!< Goes to beginning of next Span */
      bool RevWord(); 
      bool RevWordBegin();
      bool GoChar(); /*!< Increments iterator by 1 */
      bool RevChar();

      wchar_t GetNextChar();

      int GetLeftOffset(); /*!< Returns left offset counting tabs as multiple characters according to the tabstop */
      void GoByOffset(int chars); /*!< Goes forward countint tabs as multiple characters according to the tabstop. Always stays on the same line. */
      void GoBy(int chars, bool multiline = false); /*!< Goes forward counting tabs as single character. Always stays on the same line. */
      void GoLeft(int chars, bool multiline = false); /*!< Goes forward counting tabs as single character. always stays on the same line */

      int GetDistance(Iter* itr);

      void MarkupBegin(SHEdit::Format * format); /*!< Adds positionless markup. For formatting overview see \ref index */
      void MarkupEnd(SHEdit::Format * format);/*!< Adds positionless markup. For formatting overview see \ref index */
      void MarkupRem(SHEdit::Format * format);/*!< Removes all positionless markup from current position. For formatting overview see \ref index */

      Iter * Duplicate(); /*!<  returns pointer to valid duplicate of Iter. This is the only way to properly copy the position-aware iterators! */
      virtual void Update();/*!< Is called when structure of underlying buffer changed, to allow descendants to update their links to the physical structure. */
      virtual void UpdatePos();/*!< Basically same as Update(), but handles repositioning */
      virtual void RecalcPos();/*!< Recalculates IPos::pos, according to descendant's own positioning */
      void UpdateNextImark();/*!< See the Iter class info for explanation */

      bool FindNext(wchar_t * string, bool skip = true, bool caseSensitive = true, bool wholeword = false); /*!< skip defines whether to match word directly at cursor or not */
      bool FindPrev(wchar_t * string, bool skip = true, bool caseSensitive = true, bool wholeword= false);    /*!< same as findnext */
      inline bool IsUnderCursor(const wchar_t *& string, bool caseSensitive, bool wholeword);  //used as test for search;
      bool LineIsEmpty();

      void GoToLine(int line);
      String GetLine();

      bool operator==(const Iter& itr);
      bool operator>(const Iter& itr);
      bool operator<(const Iter& itr);
      bool operator!=(const Iter& itr);
      wchar_t& operator++();
      wchar_t& operator--();
  };
}
//---------------------------------------------------------------------------
#endif
