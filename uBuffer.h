//---------------------------------------------------------------------------

#ifndef uBufferH
#define uBufferH

#include "uSpan.h"
#include "uIter.h"
#include <stack>
#include <iostream>
#include <fstream>
namespace SHEdit
{
  class Format;

  /*!
   * Buffer
   * ======
   * The Buffers purpose is to store text, undo/redo information and iterator related positioning information. Text is actually stored as two level linked list, constructed on one level by instances of Span class, that create a continuous stream of text formated as null terminated wide char strings. Each Span represents some span of text, that does not contain a newline and contains the string itself, string's length, markup info and links to next and previous Spans. New lines are represented by NSpan class that inherits from Span, and thus are present in both levels of buffer. NSpans aditionally contain links to previous and next newlines and thus allow simple and fast movement over buffer even without binary tree indexing. 
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.txt}
   * //  ┌---Buffer::data (of class Range)---------------------------------------------------------------------------------------------┐ 
   * //  v                                                                                                                             v
   * //  NSpan <-------------------------------------------> NSpan <------------------------------------> NSpan <--------------------> NSpan
   * //  ├> Span <---------------> Span <------------------><┼> Span <----------------------------------><┼> Span <------------------><┤
   * //  |  ├---------------------┐├-----------------------┐ |  ├---------------------------------------┐ | ├------------------------┐ |                    
   * //  v  v c_string            vv another c_string      v v  v ...                                   v v v                        v v
   * //  \n An example of some text that may be in a buffer. \n Actually a multiline example of some text \n consisting of three lines \n
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * 
   * Edits are processed by Buffer::Insert and Buffer::Delete methods, that further decide which inner methods to use. If edit changes entire word as it is split  in Spans, just Buffer::_Insert or Buffer::_Delete are used. If just a small change is required, it is handled by Buffer::_InsertAt and Buffer::_DeleteAt that decide whether to edit the Span directly or to duplicate it for Undo. In case of larger changes, the Insert/Delete function cuts the beginning and ending spans as needed, and pushes the entire string of Spans and NSpans (that are removed) onto undo stack, while replacing it by new content. Everytime an edit is made, the former spans are pushed onto undo stack (in case of insertion between already existing spans, just empty range with apropriate flags is pushed). Exception are edits that are done consecutively on one Span - in that case only first edit duplicates the Span and pushes it onto undo stack. The last thing that is always done is to call Buffer::ItersTranslateInsert or Buffer::ItersTranslateDelete that updates linenums and positions of iters in ItrList. 
   *
   * All positioned IPos descendants that were registered on their creation (that means were passed a valid Buffer*) are registered in ItrList. That list serves for updating the iterators to correct position which is needed due to absence of newline indexing that would allow to find quickly line by its number or in the other direction to determine line's number. (The reason of it's absence is that is much cheaper to update Iterators than to update an AVL tree as long  as there's just few Iterators. And from design there is)
   *
   * Undo and Redo are handled through Buffer::stackUndo and Buffer::stackRedo that store UndoTask instances. UndoTask consists of Action, that is used to keep information about relative position changes (for Iterator position updating) and of a Range, which holds pointers to ends of the string of Spans that were removed from buffer by the change (in case that nothing was removed fom Buffer but something was inserted, then range pointers do refer to the end points that are to be connected again (on Undo), and Range::empty or Range::emptyline is set to true). Info about where the removed string of Spans does belong is contained directly in the end Spans, whose pointers were not altered when editing was done. For more info see the actual structure of UndoTask or probably the code of Buffer::UndoRedo method.
   * 
   * Here is an example of how may stackUndo look like (in the delete an overlap was shown, while insertion was made exactly at split of two Spans):
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.txt}
   * //  The original structure of spans was apparently:
   * //    v                       v                              v                 v                          v
   * //  \n|Somewhere around here a|part of this sentence has been|cut out, and then|some nonsense was inserted|\n
   * //  
   * //  After mentioned editing it would look like:
   * //  
   * //  ┌---Buffer::data (of class Range)-----------------------------------------------------------------------------------------------┐ 
   * //  v                         ┌--newly created spans-┐                          ┌--new inserted span                                v
   * //  NSpan <-----------------------------------------------------------------------------------------------------------------------> NSpan
   * //  ├> Span <---------------> Span <---------------> Span <-> Span <----------> Span <----------------> Span <--------------------><┤
   * //  |  ├---------------------┐├--┐                   ├------┐ ├----------------┐├----------------------┐├-------------------------┐ |
   * //  ▼  ▼                     ▼▼  ▼                   ▼      ▼ ▼                ▼▼                      ▼▼                         ▼ ▼
   * //  \n Somwhere around here a part                   has been cut out, and then adsf aljf alsd jfla jsfk some nonsense was inserted \n
   * //  ▲▲ ▲                                                      ▲▲                                         ▲                          ▲▲
   * //  || |top of stackUndo                                      |└--first----------------┐      ┌-----last-┘                          ||
   * //  |└firstline-----------------------------------------------┼--------empty=true--UndoTask::range----emptyline=true-------lastline-┘|
   * //  |  |second item of stack                                  |                                                                      |
   * //  └firstline-------------------UndoTask::range--------------┼---empty=false--emptyline=true------------------------------lastline--┘
   * //     |                      ┌-first--┘  └--last--┐          |
   * //     |         the old span v    <---------------┘          └-----------┐           Actions of UndoTasks were omittted
   * //     |           ┌--------- Span ---------------------------┐           |           they would contain just linenums and positions
   * //     |           |          ├-----------------------------┐ |           |
   * //     |           |          v                             v |           |
   * //     └---------- Span::prev part  of this sentence has been Span::next--┘
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * Last two things the Buffer contains are Format list, which keeps pointers to formats that were applied to buffer by Iterator handled markup, to let anyone check whether the format is applied at some location (through binary search tree of the Format class in question). The other list contains links to all Iterator marks that are applied in Buffer, which allows to find out quickly at which positions the Iterator markup changes are. For more detail see Format and IMark classes.
   *
   * Members are not documented because the names are quite self explaining and anyway mostly explained above.
   */

  class Buffer
  {
    private:
      Range* data;

      Range* preload;
      std::ifstream * preloadFile;

      Span * wordBeingEdited;

      short markupMask;

      std::stack<UndoTask*> stackUndo;
      std::stack<UndoTask*> stackRedo;

      std::list<IPos*> ItrList;
      std::set<Format*> FormatList;
      std::set<IPos*, IPos::compare> IMarkList;

      void _Insert(Span * word);
      void _Delete(Span * word);
      void _Insert(NSpan * word);
      void _Delete(NSpan * word);

      Range * _InsertAt(NSpan * line, Span * word, int pos, wchar_t * string, bool writeundo, bool forcenew);
      Range * _DeleteAt(Iter * From, Iter * To, bool writeundo, bool forcenew);
      Span* _SplitAt(Iter * At);
      Span* _SplitBegin(Iter * At);
      Span* _SplitEnd(Iter * At);

      wchar_t * _ParseWord(wchar_t*& ptr, wchar_t*& ptrend);

      Iter * UndoRedo(std::stack<UndoTask*> * stackUndo, std::stack<UndoTask*> * stackRedo, Iter *& begin);
      void UndoPush(UndoTask * event);

      void ItersTranslateInsert(int linenum, int pos, int bylines, int topos, NSpan * toline);
      void ItersTranslateDelete(int fromlinenum, int frompos, int tolinenum, int topos, NSpan * toline);

#ifdef DEBUG
      void Write(AnsiString message);
#endif

    public:
      Buffer();
      ~Buffer();

      friend class Iter;
      friend class IPos;

      Iter * Undo(Iter *& begin);
      Iter * Redo(Iter *& begin);

      int Insert(Iter * At, wchar_t * string);
      int Delete(Iter * From, Iter * To);

      wchar_t* GetText(Iter * From, Iter * To);
      String GetLine(Iter * line, bool replaceTabs);
      String GetLineTo(Iter * To, bool replaceTabs);

      void SimpleLoadFile(char * filename);
      /*
         void LoadFile(wchar_t * filename);
         void LoadFileAsync(wchar_t * filename);
         bool Preload(int lines);
         void FlushPreload();    */

      void RegisterIP(IPos * itr);
      void UnregisterIP(IPos * itr);

      void RegisterIM(IMark * itr);
      void UnregisterIM(IMark * itr);

      void RegisterF(Format * form);
      void UnregisterF(Format * form);

      Iter * Begin();
      Iter * First();
      Iter * End();

      NSpan * FirstLine();

      bool IsPlainWord(wchar_t * string);

      int CheckIntegrity(int& emptyCount);

      int GetLineCount();

      /*
         bool IsAl(wchar_t c);
         bool IsAlNum(wchar_t c);
         bool IsNum(wchar_t c);
         bool IsWhite(wchar_t c);
         */
  };
}
//---------------------------------------------------------------------------
#endif
