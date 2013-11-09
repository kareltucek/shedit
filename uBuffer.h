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

      std::stack<Range*> stackUndo;
      std::stack<Range*> stackRedo;

      void _Insert(Span * word);
      void _Delete(Span * word);
      void _Insert(NSpan * word);
      void _Delete(NSpan * word);

      void _InsertAt(NSpan * line, Span * word, int pos, wchar_t * string, bool writeundo, bool forcenew);
      void _DeleteAt(Iter * From, Iter * To, bool writeundo, bool forcenew);
      Span* _SplitAt(Iter * At);
      Span* _SplitBegin(Iter * At);
      Span* _SplitEnd(Iter * At);

      wchar_t * _ParseWord(wchar_t*& ptr, wchar_t*& ptrend);

#ifdef DEBUG
      void Write(AnsiString message);
#endif

    public:
      Buffer();
      ~Buffer();

      void Undo();
      void Redo();

      int Insert(Iter * At, wchar_t * string);
      int Delete(Iter * From, Iter * To);

      wchar_t* GetText(Iter * From, Iter * To);

      void LoadFile(wchar_t * filename);
      void LoadFileAsync(wchar_t * filename);
        bool Preload(int lines);
        void FlushPreload();

      Iter * Begin();
      Iter * First();
      Iter * End();

      bool IsPlainWord(wchar_t * string);

      int CheckIntegrity(int& emptyCount);

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
