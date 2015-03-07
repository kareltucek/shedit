//------------------------------------------------------------

#ifndef uIterH
#define uIterH

#include <vcl.h>
#include "config.h"
#include "uIPos.h"
#include <iterator>

namespace SHEdit
{
  class Span;
  class NSpan;
  class Mark;
  class IMark;
  class Format;
  class Buffer;
  class FontStyle;
  class TSHEdit;

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
   *
   * The /*AUTOGEN  tags serve for automatical generation of the Cursor wrapper. In case you edit this file, you probably need to rerun the generator. See uCursor.h documentation for details.
   * */

  //---------------------------------------------------------------------------
  class Iter : protected IPos, public std::iterator<std::bidirectional_iterator_tag, const wchar_t>
  {
    private:
      inline bool IsWordChar(wchar_t c);
    protected:
      friend class TSHEdit;
      friend class Buffer;
      friend class Parser;
      friend class Span;
      friend class NSpan;

      Span * word;                                                                                   /*!< Pointer to current Span in buffer's structure */
      int offset;                                                                                    /*!< Offset from beginning of current Span */
      wchar_t * ptr;                                                                                 /*!< Direct pointer to current character */

      int nextimark;                                                                                 /*!< See the Iter class info for explanation */
      int nextimarkln;                                                                               /*!< See the Iter class info for explanation */

      bool GoWord();                                                                                 /*!< Goes to beginning of next Span */
      bool RevWord();
      bool RevWordBegin();

      virtual void Update();                                                                         /*!< Is called when structure of underlying buffer changed, to allow descendants to update their links to the physical structure. */
      virtual void UpdatePos();                                                                      /*!< Basically same as Update(), but handles repositioning */
      virtual void RecalcPos();                                                                      /*!< Recalculates IPos::pos, according to descendant's own positioning */
      void UpdateNextImark();                                                                        /*!< See the Iter class info for explanation */


    public:
      Iter();
      Iter(const Iter& itr);
      Iter(NSpan * line);
      Iter(NSpan * line, int linenum, int pos, Buffer * buffer);
      Iter(int offset, Span * word, NSpan * line, Buffer * buffer, int linenum = -1);
      ~Iter();

      void Invalidate();
      bool Valid() /*AUTOGEN_PASS*/;

      bool GoLine(bool allowEnd = false)/*AUTOGEN_UPDATE*/;                                          /*!< Goes to the pos 0 of next line. That is the offset 0 of first Span after nextline NSpan */
      bool GoLineEnd()/*AUTOGEN_UPDATE*/;
      bool GoLineStart()/*AUTOGEN_UPDATE*/;
      bool RevLine()/*AUTOGEN_UPDATE*/;
      bool RevLineBegin()/*AUTOGEN_UPDATE*/;
      bool GoChar()/*AUTOGEN_UPDATE*/;                                                               /*!< Increments iterator by 1 */
      bool RevChar()/*AUTOGEN_UPDATE*/;

      wchar_t GetNextChar()/*AUTOGEN_PASS*/;
      wchar_t GetPrevChar()/*AUTOGEN_PASS*/;
      wchar_t GetChar()/*AUTOGEN_PASS*/;

      int GetLineNum()/*AUTOGEN_PASS*/;

      void GoWordLiteral()/*AUTOGEN_UPDATE*/;                                                        /*!< goes to next start of a word (alnum + underscore) */
      void RevWordLiteral()/*AUTOGEN_UPDATE*/;                                                       /*!< goes to previous start of a word (alnum + underscore) */
      void GoWordEndLiteral();                                                                       /*!< goes to next end of a word (alnum + underscore)*/

      int GetLeftOffset()/*AUTOGEN_PASS*/;                                                           /*!< Returns left offset counting tabs as multiple characters according to the tabstop */
      void GoByOffset(int chars)/*AUTOGEN_UPDATE*/;                                                  /*!< Goes forward countint tabs as multiple characters according to the tabstop. Always stays on the same line. */
      void GoBy(int chars, bool multiline = false)/*AUTOGEN_UPDATE*/;                                /*!< Goes forward counting tabs as single character. Always stays on the same line. */
      void GoLeft(int chars, bool multiline = false)/*AUTOGEN_UPDATE*/;                              /*!< Goes forward counting tabs as single character. always stays on the same line */

      int GetDistance(Iter* itr)/*AUTOGEN_PASS*/;

      void MarkupBegin(SHEdit::Format * format)/*AUTOGEN_PASS*/;                                     /*!< Adds positionless markup. For formatting overview see \ref index */
      void MarkupEnd(SHEdit::Format * format)/*AUTOGEN_PASS*/;                                       /*!< Adds positionless markup. For formatting overview see \ref index */
      void MarkupRem(SHEdit::Format * format)/*AUTOGEN_PASS*/;                                       /*!< Removes all positionless markup from current position. For formatting overview see \ref index */

      bool FindNext(wchar_t * string, bool skip = true, bool caseSensitive = true, bool wholeword = false)/*AUTOGEN_UPDATE*/; /*!< skip defines whether to match word directly at cursor or not */
      bool FindPrev(wchar_t * string, bool skip = true, bool caseSensitive = true, bool wholeword= false)/*AUTOGEN_UPDATE*/; /*!< same as findnext */
      bool IsUnderCursor(const wchar_t *& string, bool caseSensitive, bool wholeword)/*AUTOGEN_PASS*/; /*!< Tests if the string is at the position of cursor. Serves for FindNext/FindNext. */
      bool LineIsEmpty( bool allowWhite = false)/*AUTOGEN_PASS*/;

      void GoToLine(int line)/*AUTOGEN_UPDATE*/;
      String GetLine()/*AUTOGEN_PASS*/;

      Iter * Duplicate();                                                                            /*!<  returns pointer to valid duplicate of Iter. */

      Iter& operator=(const Iter& itr);
      bool operator==(const Iter& itr);
      bool operator!=(const Iter& itr);
      bool operator>(const Iter& itr);
      bool operator<(const Iter& itr);
      Iter& operator++();
      Iter& operator--();
      wchar_t& operator*();
  };
}
//---------------------------------------------------------------------------
#endif
