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
  //---------------------------------------------------------------------------
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

      std::list<Iter*> ItrList;

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

      Iter * Undo(Iter *& begin);
      Iter * Redo(Iter *& begin);

      int Insert(Iter * At, wchar_t * string);
      int Delete(Iter * From, Iter * To);

      wchar_t* GetText(Iter * From, Iter * To);
      String GetLine(Iter * line, bool replaceTabs);
      String GetLineTo(Iter * To, bool replaceTabs);

      void SimpleLoadFile(wchar_t * filename);
      /*
      void LoadFile(wchar_t * filename);
      void LoadFileAsync(wchar_t * filename);
      bool Preload(int lines);
      void FlushPreload();    */

      void Register(Iter * itr);
      void Unregister(Iter * itr);

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
