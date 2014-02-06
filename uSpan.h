//---------------------------------------------------------------------------

#ifndef uSpanH
#define uSpanH

#include "uParser.h"
#include "uStack.h"
#include <wchar.h>
#include <list>

namespace SHEdit
{
  class Mark;
  class Iter;
  //---------------------------------------------------------------------------
  struct Span
  {
    Span(Span * prev, Span * next, wchar_t* string, short length);
    Span(Iter * After);
    Span(Span * afterword);
    Span();
    ~Span();

    Span* prev;
    Span* next;

    wchar_t* string;
    short length;
    Stack<Mark> marks;
  };
  //---------------------------------------------------------------------------
  struct NSpan : Span
  {
    NSpan(Span* afterword, NSpan* afterline);
    NSpan(Iter * After);
    NSpan();
    ~NSpan();

    NSpan * nextline;
    NSpan * prevline;

    /*
       std::list<Iter*> ItrList;
       void Register(Iter* itr);
       void Unregister(Iter* itr);

       void Invalidate(Span * from);
       void ItersSplit(Span * from, Span * toFirst, Span * toSec, int byPos, bool custoff = false, int tooffset = 0);
       void ItersTranslate(Span * from, Span * to, int byPos, int inc = 1);
       void ItersTransmitAll(NSpan * to);
       void ItersTransmit(Span * from, NSpan * to);
       void ItersMove(Span * from, NSpan * to, Span * toword, int offset);  */
    Parser::ParserState parserState;

    //int GetPos();
    //whatever
  };
  //---------------------------------------------------------------------------
  struct Range
  {
    Range(Span * first, Span * last, bool empty, NSpan * firstLine, NSpan * lastLine, bool lineempty, int linecount);
    ~Range();
    void Free(); //destructor shall not destroy data it holds, but free shoul

    Span* first;
    Span* last;

    NSpan* firstLine;
    NSpan* lastLine;

    bool empty;
    bool lineempty;

    int linecount;
  };
  //---------------------------------------------------------------------------
  struct Action
  {
    enum ActionType{deletion, insertion};

    Action(int fromlinenum, int tolinenum, int frompos, int topos, ActionType type);
    ~Action();

    ActionType type;
      int fromlinenum;
      int tolinenum;
      int frompos;
      int topos;
  };
  //---------------------------------------------------------------------------
  struct UndoTask
  {
    UndoTask(Action * action, Range * range);
    ~UndoTask();

    Action * action;
    Range * range;
  };
  
};
//---------------------------------------------------------------------------
#endif
