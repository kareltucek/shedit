//---------------------------------------------------------------------------


#pragma hdrstop

#include "uBuffer.h"
#include "uSpan.h"
#include "uIter.h"
#include "uMark.h"
#include "uFormat.h"
#include <stack>
#include <wchar.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <wstring.h>
#include <ctype.h>

using namespace SHEdit;
#ifdef _DEBUG_LOGGING
std::ofstream myfile;
#endif

#pragma package(smart_init)
//---------------------------------------------------------------------------
Buffer::Buffer()
{
#ifdef _DEBUG_LOGGING
  myfile.open("buffer.txt", ios::out);
#endif
  NSpan * head = new NSpan();
  NSpan * tail = new NSpan();

  markupMask = 0;
  keepHistory = true;

  head->next = tail;
  head->nextline = tail;
  tail->prev = head;
  tail->prevline = head;

  data = new Range(head, tail, false, head, tail, false, 1);
  preload = new Range(NULL, NULL, false, NULL, NULL, false, 1);

}
//---------------------------------------------------------------------------
Buffer::~Buffer()
{
  while(!ItrList.empty())
  {
    ItrList.front()->Invalidate();
    ItrList.pop_front();
  }
  while(!stackUndo.empty())
  {
    stackUndo.top()->range->Free();
    delete stackUndo.top();
    stackUndo.pop();
  }
  while(!stackRedo.empty())
  {
    stackRedo.top()->range->Free();
    delete stackRedo.top();
    stackRedo.pop();
  }
}
//---------------------------------------------------------------------------
Iter * Buffer::Begin()
{
  return new Iter(0, data->first->next, data->firstLine, this, 1);
}
//---------------------------------------------------------------------------
Iter * Buffer::First()   //technically shows wrong location - just if we NEED to maintain a link to fist link no matter what gets inserted
{
  return new Iter(1, data->first, data->firstLine, this, 1);
}
//---------------------------------------------------------------------------
Iter Buffer::begin()
{
  return  Iter(0, data->first->next, data->firstLine, this, 1);
}
//---------------------------------------------------------------------------
Iter Buffer::first()   //technically shows wrong location - just if we NEED to maintain a link to fist link no matter what gets inserted
{
  return Iter(1, data->first, data->firstLine, this, 1);
}
//---------------------------------------------------------------------------
NSpan * Buffer::FirstLine()
{
  return data->firstLine;
}
//---------------------------------------------------------------------------
Iter * Buffer::End()
{
  return new Iter(0, data->last, data->lastLine->prevline, this, data->linecount);
}
//---------------------------------------------------------------------------
Iter Buffer::end()
{
  return Iter(0, data->last, data->lastLine->prevline, this, data->linecount);
}
//---------------------------------------------------------------------------
void Buffer::_Insert(Span * word)
{
  if(word->next)
    word->next->prev = word;
  if(word->prev)
    word->prev->next = word;
}
//---------------------------------------------------------------------------
void Buffer::_Delete(Span * word)
{

#ifdef _DEBUG
  //Write("delete words "+String((int)word->prev)+String(" ")+String((int)word->next));
#endif
  word->next->prev = word->prev;
  word->prev->next = word->next;
#ifdef _DEBUG
  //Write("delete words returning");
#endif
}
//---------------------------------------------------------------------------
void Buffer::_Insert(NSpan * word)
{
  word->nextline->prevline = word;
  word->prevline->nextline = word;
  _Insert((Span*)word);
}
//---------------------------------------------------------------------------
void Buffer::_Delete(NSpan * word)
{
  if(word->nextline != NULL)
    word->nextline->prevline = word->prevline;
  if(word->prevline != NULL)
    word->prevline->nextline = word->nextline;

  _Delete((Span*)word);
}
//---------------------------------------------------------------------------
Span* Buffer::_SplitBegin(Iter * At)
{
  wchar_t* firstnewstr = new wchar_t[At->word->length+1];
  wcscpy(firstnewstr, At->word->string);
  Span * firstnew = new Span(At->word->prev, NULL, firstnewstr, At->offset);
  firstnew->string[At->offset] = '\0';

  if(At->word->marks.top)
  {
    for(Stack<Mark>::Node * m = At->word->marks.top; m != NULL; m = m->next)
      if(m->data.pos < At->offset)
        m->data.format->Add( firstnew->marks.Push(m->data) );
  }
  return firstnew;
}
//---------------------------------------------------------------------------
Span* Buffer::_SplitEnd(Iter * At)
{
  wchar_t* secnewstr = new wchar_t[At->word->length-At->offset+1];
  wcscpy( secnewstr, At->word->string+At->offset);
  Span * secondnew = new Span(NULL, At->word->next, secnewstr, At->word->length-At->offset);

  if(At->word->marks.top)
  {
    for(Stack<Mark>::Node * m = At->word->marks.top; m != NULL; m = m->next)
      if(m->data.pos >= At->offset)
        m->data.format->Add( secondnew->marks.Push(Mark(m->data.format, m->data.begin, m->data.pos-At->offset)));
  }
  return secondnew;
}
//---------------------------------------------------------------------------
Span* Buffer::_SplitAt(Iter * At)
{
  Span * firstnew = _SplitBegin(At); 
  Span * secondnew = _SplitEnd(At); 

  firstnew->next = secondnew;
  secondnew->prev = firstnew;

  _Insert(firstnew); 
  _Insert(secondnew);
  return firstnew;
}
//---------------------------------------------------------------------------
int Buffer::Insert(Iter * At, const wchar_t * string)
{
  if(IsPlainWord(string))
  {
    if(At->offset == 0 && At->word->prev->length < 20 && IsPlainWord(At->word->prev->string))
    {
      Range * r = _InsertAt(At->line, At->word->prev, At->word->prev->length, string, true, false);
      if(r != NULL)
        UndoPush(new UndoTask(new Action(At->linenum, 0, At->pos, At->pos+wcslen(string), Action::ActionType::insertion), r));
      this->ItersTranslateInsert(At->linenum, At->pos, 0, At->pos + wcslen(string), At->line);
      HistoryOnOff();
      return 0;
    }
    else if(At->word->length < 20 && IsPlainWord(At->word->string))
    {
      Range * r = _InsertAt(At->line, At->word, At->offset, string, true, false);
      if(r != NULL)
        UndoPush(new UndoTask(new Action(At->linenum, 0, At->pos, At->pos+wcslen(string), Action::ActionType::insertion), r));
      this->ItersTranslateInsert(At->linenum, At->pos, 0, At->pos + wcslen(string), At->line);
      HistoryOnOff();
      return 0;
    }
  }

  const wchar_t * ptrend = string + wcslen(string);
  const wchar_t * ptr = string;
  int linesInserted = 0;
  Span * prev;
  NSpan * prevN;
  NSpan * FirstN;
  Range * r;
  if(At->offset == 0)
  {
    prev = At->word->prev;
    prevN = At->line;
    r = new Range(prev, prev->next, true, At->line, At->line->nextline, true, 0);
  }
  else
  {
    r = new Range(At->word, At->word, false, At->line, At->line->nextline, true, 0);
    prev = _SplitAt(At);
    prevN = At->line;
  }
  //fix
  if(prevN->nextline == NULL)
    prevN = prevN->prevline;

  bool first = true;
  int pos = At->pos;
  while(ptr < ptrend)
  {
    wchar_t * word = _ParseWord(ptr, ptrend) ;
    if(*word == '\n')
    {
      prevN = new NSpan(prev, prevN);
      prev = prevN;
      _Insert(prevN);
      linesInserted++;
      pos = 0;
      delete[] word;
    }
    else
    {
      prev = new Span(prev, prev->next, word, wcslen(word));
      pos += prev->length;
      _Insert(prev);
    }
  }
  //delete[] string;
  wordBeingEdited = prev;
  data->linecount += linesInserted;
  r->linecount = -linesInserted;
  UndoPush(new UndoTask(new Action(At->linenum, linesInserted, At->pos, pos, Action::ActionType::insertion), r));
  this->ItersTranslateInsert(At->linenum, At->pos, linesInserted, pos, prevN);

  HistoryOnOff();
  return linesInserted;
}
//---------------------------------------------------------------------------

int Buffer::Delete(Iter * From, Iter * To)
{
  if(*From == *To)
    return 0;
  if(From->word == To->word || (*(From->word->string) != '\n' && From->word->next == To->word && To->offset == 0))
  {
    Range * r = NULL;
    if(To->offset == 0 && From->offset == 0 && From != To)
    {
      r = new Range(From->word, From->word, false, From->line->nextline, From->line, false, 0);
      _Delete(From->word);
    }
    else
      r = _DeleteAt(From, To, true, false);
    if(r != NULL)
      UndoPush(new UndoTask(new Action(From->linenum, To->linenum, From->pos, To->pos, Action::ActionType::deletion), r));
    ItersTranslateDelete(From->linenum, From->pos, To->linenum, To->pos, From->line);
    HistoryOnOff();
    return 0;
  }
  else if(*(From->word->string) == '\n' && From->word->next == To->word && To->offset == 0)
  {
    Range * r = new Range(From->word, From->word, false, From->line->nextline, From->line->nextline, false, 1);
    _Delete((NSpan*)(From->word));
    data->linecount--;
    UndoPush(new UndoTask(new Action(From->linenum, To->linenum, From->pos, To->pos, Action::ActionType::deletion), r));
    ItersTranslateDelete(From->linenum, From->pos, To->linenum, To->pos, From->line);
    HistoryOnOff();
    return 1;
  }
  else
  {

#ifdef _DEBUG
    //Write("Delete cycle");
#endif
    Span * begin, * end;
    Span * undoBegin, * undoEnd;
    int linesDeleted = 0;
    //begin
    undoBegin = From->word;
    if(From->offset == 0)
      begin = From->word->prev;
    else
      begin = _SplitBegin(From); 

    //end
    if(To->offset == 0)
    {
      undoEnd = To->word->prev;
      end = To->word;
    }
    else
    {
      undoEnd = To->word;
      end = _SplitEnd(To);
    }
    Range * r = new Range(undoBegin, undoEnd, false, From->line->nextline, To->line, false, 0);
    begin->next = end;
    end->prev = begin;
    From->line->nextline = To->line->nextline;
    To->line->nextline->prevline = From->line;
    _Insert(begin);
    _Insert(end);

    linesDeleted = To->linenum - From->linenum;
    r->linecount = linesDeleted;
    data->linecount -= linesDeleted;
    UndoPush(new UndoTask(new Action(From->linenum, To->linenum, From->pos, To->pos, Action::ActionType::deletion), r));
    ItersTranslateDelete(From->linenum, From->pos, To->linenum, To->pos, From->line);
    HistoryOnOff();
    return linesDeleted;
  }
}


//---------------------------------------------------------------------------
Range * Buffer::_DeleteAt(Iter * From, Iter * To, bool writeundo, bool forcenew)
{
  if(*(From->word->string) == '\0' && From->word->next && From->word->next == To->word && To->offset == 0)
  {
    _Delete(From->word);
    return NULL;
  }
  Range * r = NULL;

  int toOffset = To->offset;
  if(From->word->next == To->word && To->offset == 0)
    toOffset = From->word->length;
  int length = (toOffset-From->offset);
  wchar_t* newstr = new wchar_t[From->word->length-length+1];

  wcsncpy( newstr, From->word->string, From->offset);
  wcscpy( newstr+From->offset, From->word->string+toOffset);

  Span * newword;
  if((wordBeingEdited != From->word && writeundo) || forcenew)
  {
    newword = new Span(From->word->prev, From->word->next, newstr, wcslen(newstr));
    wordBeingEdited = newword;
    if(writeundo)
      r = new Range(From->word, From->word, false, From->line->nextline, To->line, false, 0);
    _Insert(newword);
    if(From->word->marks.top)
      for(Stack<Mark>::Node * m = From->word->marks.top; m != NULL; m = m->next)
        m->data.format->Add( newword->marks.Push(m->data));
  }
  else
  {
    delete[] From->word->string;
    From->word->string = newstr;
    From->word->length = wcslen(newstr);
    newword = From->word;
  }
  if(newword->marks.top)
  {
    for(Stack<Mark>::Node * m = newword->marks.top; m != NULL; m = m->next)
    {
      if(m->data.pos >= To->offset+length)
        m->data.pos -=length;
      else if(m->data.pos >= To->offset)
        m->data.pos -= m->data.pos - From->offset;
    }
  }
  return r;
}

//---------------------------------------------------------------------------
/*!
 * returns range for undo event
 */
Range * Buffer::_InsertAt(NSpan * line, Span * word, int pos, const wchar_t * string, bool writeundo, bool forcenew)   //OK
{
  //wchar_t* newstr = (wchar_t*) malloc((at->word->length+wcslen(string)+1) * sizeof(wchar_t));
  Range * r = NULL;

  wchar_t* newstr = new wchar_t[word->length+wcslen(string)+1];
  wcscpy( newstr, word->string);
  wcscpy( newstr+pos, string);
  wcscpy( newstr+(pos+wcslen(string)), word->string+pos);

  Span * newword;
  if((wordBeingEdited != word && writeundo) || forcenew)
  {
    newword = new Span(word->prev, word->next, newstr, wcslen(newstr));
    wordBeingEdited = newword;
    if(writeundo)
      r = new Range(word, word, false, line->nextline, line, false, 0);
    _Insert(newword);
    if(word->marks.top)
      for(Stack<Mark>::Node * m = word->marks.top; m != NULL; m = m->next)
        m->data.format->Add(newword->marks.Push(Mark(m->data)));
  }
  else
  {
    delete[] word->string;
    word->string = newstr;
    word->length = wcslen(newstr);
    newword = word;
  }
  if(word->marks.top)
    for(Stack<Mark>::Node * m = newword->marks.top; m != NULL; m = m->next)
      if(m->data.pos >= pos)
        m->data.pos += wcslen(string);
  return r;
}

//---------------------------------------------------------------------------
wchar_t* Buffer::_ParseWord(const wchar_t *& ptr, const wchar_t * ptrend)             //OK
{
  short type;
  const wchar_t * ptrstart = ptr;
#ifdef BREAK
  /*
     if(IsAl(*ptr))
     {
     while(IsAlNum(*ptr) && *ptr != '\0')
     ptr++;
     }
     else if(IsNum(*ptr))
     {
     while(IsNum(*ptr) && *ptr != '\0')
     ptr ++;
     }
     else if(IsWhite(*ptr))
     {
     while(IsWhite(*ptr) && *ptr != '\0')
     ptr ++;
     }
     else
     {
     if((*ptr == '/' && *(ptr+sizeof(wchar_t)) == '*') || (*ptr == '*' && *(ptr+sizeof(wchar_t)) == '/') || (*ptr == '-' && *(ptr+sizeof(wchar_t)) == '-'))
     ptr += 2;
     else
     ptr ++;
     }*/
#else
  while(*ptr == '\r') //ignore them and process *ALL*!
  {
    ptrstart++;
    ptr++;
  }
  if(*ptr == '\n')
  {
    ptr ++;
  }
  else
  {
    while(*ptr != '\r' && *ptr != '\n' && *ptr != '\0')
      ptr ++;
  }
#endif
  //wchar_t* newstr = (wchar_t*) malloc((ptr-ptrstart) + sizeof(wchar_t));
  int dbg = ptr-ptrstart;
  int dbg2 = sizeof(wchar_t);
  wchar_t* newstr = new wchar_t[(ptr-ptrstart)+1];
  memcpy(newstr, ptrstart, (ptr-ptrstart)*sizeof(wchar_t));
  newstr[ptr-ptrstart]='\0';
  return newstr; 

}
//---------------------------------------------------------------------------
bool Buffer::IsPlainWord(const wchar_t * string)                //OK
{
#ifdef BREAK
  /*
     if(!IsAl(*string))
     return false;
     while(*string != '\0')
     {
     if(!IsAlNum(*string))
     return false;
     string++;
     }
     return true;*/
#else
  while(*string != '\0')
  {
    if('\n' == *string)
      return false;
    string++;
  }
  return true;
#endif
}
//---------------------------------------------------------------------------
/*
   void Buffer::LoadFile(wchar_t * filename)
   {
   LoadFileAsync(filename);
   Preload(-1);
   FlushPreload();
   }                     */
//---------------------------------------------------------------------------
/*
   void Buffer::LoadFileAsync(wchar_t * filename)
   {
   preloadFile = new std::ifstream();
   preloadFile->open("test.txt");
   preload->first = data->first;
   preload->firstLine = data->firstLine;
   preload->last = data->last;
   preload->lastLine = data->lastLine;
   data->linecount = 0;
   }                        */
//---------------------------------------------------------------------------
Iter * Buffer::Redo(Iter *& begin)
{
  return UndoRedo(&stackRedo, &stackUndo, begin);
}
//---------------------------------------------------------------------------
Iter * Buffer::Undo(Iter *& begin)
{
  return UndoRedo(&stackUndo, &stackRedo, begin);
}
//---------------------------------------------------------------------------
void Buffer::HistoryOnOff()
{
  if( !keepHistory )
  {
    PurgeStack(stackRedo);
      PurgeStack(stackUndo);
  }
}
//---------------------------------------------------------------------------
void Buffer::PurgeStack(std::stack<UndoTask*>& stack)
{
  while(stack.size() > 0)
  {
    UndoTask * redoevent = stack.top();
    stack.pop();
    redoevent->range->Free();
      delete redoevent;
  }
}
//---------------------------------------------------------------------------
void Buffer::UndoPush(UndoTask * event)
{
  stackUndo.push(event);
  PurgeStack(stackRedo);
}
//---------------------------------------------------------------------------
Iter * Buffer::UndoRedo(std::stack<UndoTask*> * stackUndo, std::stack<UndoTask*> * stackRedo, Iter *& begin)
{
#ifdef _DEBUG
  //Write(String("undo called"));
#endif
  if((*stackUndo).size() == 0)
    return NULL;
  UndoTask * event = (*stackUndo).top();
  (*stackUndo).pop();

  //undo = lines that are gonna to be removed from buffer
  //rep = lines that are  gonna to be placed to buffer
  Span * undoFirst = event->range->empty ? event->range->first->next : event->range->first->prev->next;
  Span * undoLast = event->range->empty ? event->range->last->prev : event->range->last->next->prev;
  NSpan * undoLineFirst = event->range->lineempty ? event->range->firstLine : event->range->firstLine->prevline;
  NSpan * replLineLast = event->range->lineempty ? event->range->firstLine : event->range->lastLine;

  /*
     int linestoberemoved = 0;
     NSpan * line = undoLineFirst;
     Span * span = undoFirst;
     if(undoFirst->prev != undoLast)
     {
     for(; span != undoLast->next; span = span->next)
     {
     line->ItersTransmit(span, replLineLast);
     replLineLast->ItersMove(span, replLineLast, undoLast->next, 0);
     if(*(span->string) == '\n')
     {
     line = (NSpan*)span;
     linestoberemoved++;
     }
     }
     }
     while(*(span->string) != '\n')
     {
     line->ItersTransmit(span, replLineLast);
     span = span->next;
     }
     line->ItersTransmit(span, replLineLast); //we want even the last one
     this->data->linecount = data->linecount - linestoberemoved + event->linecount;    */

  this->data->linecount = data->linecount + event->range->linecount;

  //prepare new range
  Range * range;
  {
    Span * redoFirst;
    Span * redoLast;
    bool redoempty;
    NSpan * redoFirstLine;
    NSpan * redoLastLine;
    bool redoemptyline;

    if(event->range->empty)
    {
      redoFirst = event->range->first->next;
      redoLast = event->range->last->prev;
      redoempty = false;
    }
    else if(event->range->first->prev == event->range->last->next->prev)
    {
      redoFirst = event->range->first->prev;
      redoLast = event->range->last->next;
      redoempty = true;
    }
    else
    {
      redoFirst = event->range->first->prev->next;
      redoLast = event->range->last->next->prev;
      redoempty = false;
    }

    if(event->range->lineempty)
    {
      redoFirstLine = event->range->firstLine->nextline;
      redoLastLine = event->range->lastLine->prevline;
      redoemptyline = false;
    }
    else if(event->range->firstLine->prevline == event->range->lastLine->nextline->prevline)
    {
      redoFirstLine = event->range->firstLine->prevline;
      redoLastLine = event->range->lastLine->nextline;
      redoemptyline = true;
    }
    else
    {
      redoFirstLine = event->range->firstLine->prevline->nextline;
      redoLastLine = event->range->lastLine->nextline->prevline;
    }

    range = new Range(redoFirst, redoLast, redoempty, redoFirstLine, redoLastLine, redoemptyline, -event->range->linecount);
  }

  //undo physical structure
  if(event->range->empty)
  {
    event->range->first->next = event->range->last;
    event->range->last->prev = event->range->first;
  }
  else
  {
    event->range->first->prev->next = event->range->first;
    event->range->last->next->prev = event->range->last;
  }

  if(event->range->lineempty)
  {
    event->range->firstLine->nextline = event->range->lastLine;
    event->range->lastLine->prevline = event->range->firstLine;
  }
  else
  {
    event->range->firstLine->prevline->nextline = event->range->firstLine;
    event->range->lastLine->nextline->prevline = event->range->lastLine;
  }

  Action * action;
  Iter * itr;
  if(event->action->type == Action::ActionType::deletion)
  {
    ItersTranslateInsert(event->action->fromlinenum, event->action->frompos, event->action->tolinenum - event->action->fromlinenum, event->action->topos, replLineLast);
    action = new Action(event->action->fromlinenum, event->action->tolinenum - event->action->fromlinenum, event->action->frompos, event->action->topos, Action::ActionType::insertion);
    itr = new Iter(replLineLast, event->action->tolinenum, event->action->topos, this);
    begin = new Iter(undoLineFirst, event->action->fromlinenum, event->action->frompos, this);
  }
  else
  {
    ItersTranslateDelete(event->action->fromlinenum, event->action->frompos, event->action->fromlinenum+event->action->tolinenum, event->action->topos, undoLineFirst);
    action = new Action(event->action->fromlinenum,event->action->fromlinenum+event->action->tolinenum, event->action->frompos, event->action->topos, Action::ActionType::deletion);
    itr = new Iter(undoLineFirst, event->action->fromlinenum, event->action->frompos, this);
    begin = NULL;
  }

  (*stackRedo).push(new UndoTask(action, range));

  delete event;
  return itr;
#ifdef _DEBUG
  //Write(String("undone"));
#endif
}
//---------------------------------------------------------------------------
void Buffer::SimpleLoadFile(const wchar_t * filename)
{
  std::ifstream * file = new std::ifstream();
  file->open(filename);
  if (file && file->is_open())
  {
    Iter * a = Begin();
    Iter * b = End();
    Delete(a, b);
    delete a;
    delete b;

    std::string line;
    Span * lastword = data->first;
    NSpan * lastline = data->firstLine;
    while(getline(*file,line))
    {
      data->linecount++;
      wchar_t * str;
      int wchars_num = 0;
      if(line.size() > 0)
      {
        wchars_num =  MultiByteToWideChar( CP_UTF8 , 0 , line.c_str() , -1, NULL , 0 );
        str = new wchar_t[wchars_num];
        MultiByteToWideChar( CP_UTF8 , 0 , line.c_str() , -1, str , wchars_num );
        lastword = new Span(lastword, data->last, str, wcslen(str));
        _Insert(lastword);
      }
      lastline = new NSpan(lastword, lastline);
      _Insert(lastline);
      lastword = lastline;
    }
    Range * r = new Range(data->first, data->last, true, data->firstLine, data->lastLine, true, -data->linecount+1); //- is not typo
    Action * ac = new Action(1, data->linecount, 0, 0, Action::ActionType::insertion);
    UndoPush(new UndoTask(ac, r));
    ItersTranslateInsert(1, 1, 1, 1, data->firstLine);
  }
  delete file;
}
//---------------------------------------------------------------------------
void Buffer::SimpleSaveFile(const wchar_t * filename)
{
  std::ofstream * file = new std::ofstream();
  file->open(filename);
  if (file && file->is_open())
  {
    Iter * itr = Begin();
    while(itr->line->nextline != NULL)
    {
      int size_needed = WideCharToMultiByte(CP_UTF8, 0, itr->word->string, -1, NULL, 0, NULL, NULL);
        std::string strTo( size_needed, 0 );
        char * utf8 = new char[size_needed+1];
        utf8[size_needed] = '\0';
        WideCharToMultiByte(CP_UTF8, 0, itr->word->string, -1, utf8, size_needed, NULL, NULL);
        *file << utf8;
        delete utf8;
    }
    delete itr;
  }
  delete file;
}
//---------------------------------------------------------------------------
/*
   bool Buffer::Preload(int lines)
   {
   std::string line;
   if (preloadFile && preloadFile->is_open())
   {
   using namespace std;
   int i;
   for(i = 0; i != lines && getline(*preloadFile,line); i++)
   {
   data->linecount++;
   wchar_t * str;
   int wchars_num = 0;
   if(line.size() > 0)
   {
   wchars_num =  MultiByteToWideChar( CP_UTF8 , 0 , line.c_str() , -1, NULL , 0 );
   str = new wchar_t[wchars_num];
   MultiByteToWideChar( CP_UTF8 , 0 , line.c_str() , -1, str , wchars_num );
   }
   else
   {
   str = new wchar_t[1];
   str[0] = '\0';
   }
   Span * text = new Span(preload->last, preload->lastLine, str, 0);
   text->length = wcslen(str);
   NSpan * line = new NSpan(text, preload->lastLine);
   text->next = line;
   if(i == 0)
   {
   text->prev = preload->first;
   line->prevline = preload->firstLine;
   preload->first = text;
   preload->firstLine = line;
   line->next = preload->last;
   line->nextline = preload->lastLine;
   }
   else
   {
   line->next = preload->last->next;
   line->nextline = preload->lastLine->nextline;
   preload->last->next = text;
   preload->lastLine->nextline = line;
   }
   preload->last = line;
   preload->lastLine = line;
   }
   if(i != lines)
   {
   preloadFile->close();
   delete preloadFile;
   }
   return true;
   }
   return false;
   }
//---------------------------------------------------------------------------
void Buffer::FlushPreload()
{
//undo
if(preload->first == data->first)
return; //invalid file or whatever alike
if(preload->first->prev->next == preload->last->next)
UndoPush(new Range(preload->first->prev, preload->last->next, true, preload->firstLine->prevline, preload->lastLine->nextline, true, 0));
else
UndoPush(new Range(preload->first->prev->next, preload->last->next->prev, false, preload->firstLine->prevline->nextline, preload->lastLine->nextline->prevline, false, 0));

int linesremoved = 0;
NSpan * lastN = preload->firstLine->prevline;
bool firstnl = true;
lastN->ItersSplit(preload->first->prev->next, preload->first, preload->first->prev->next, 1, true, 0);
for(Span * span = preload->first->prev->next; span && span != preload->last->next; span = span->next)
{
  lastN->ItersMove(span, preload->lastLine, preload->lastLine->next, 0);  //modifies all iterators to point to first word after deletion
  if(!firstnl)
    lastN->ItersTransmitAll(preload->lastLine);         //moves all references from their former NLs to last inserted NL    //do not copy this, generally incorrect, though here working
  else
    lastN->ItersTransmit(span, preload->lastLine);

  if(*(span->string) == '\n')
  {
    linesremoved++;
    while(((NSpan*)span)->ItrList.size() == 0 && span != preload->lastLine->nextline)
    {
      span = ((NSpan*)span)->nextline;
      linesremoved++;
    }

    lastN = (NSpan*)span;
    firstnl = false;
  }
}
stackUndo.top()->linecount = linesremoved;
preload->firstLine->prevline->ItersTransmit( preload->lastLine->nextline, preload->lastLine); //to fix iters pointing to place, assumes endline just after end of insertion

_Insert(preload->first);
_Insert(preload->firstLine);
_Insert(preload->lastLine);
_Insert(preload->last); //??probably wrong - added later
preload->first = preload->last;
preload->firstLine = preload->lastLine;
preload->last = preload->last->next;
preload->lastLine = preload->lastLine->nextline;
}               */
//---------------------------------------------------------------------------
wchar_t * Buffer::GetText(Iter * From, Iter* To)
{
  Iter * itr = From->Duplicate();
  std::wstring str;
  if(From->word != To->word)
  {
    str += itr->ptr;
    itr->GoWord();
    while(itr->word != To->word)
    {
      str += itr->word->string;
      itr->GoWord();
    }
    str += std::wstring(itr->word->string).substr(0, To->offset);
  }
  else
  {
    str = std::wstring(itr->word->string).substr(From->offset, To->offset);
  }
  delete itr;
  wchar_t * cstr = new wchar_t[str.length()+1];
  wcscpy(cstr, str.c_str());
  return cstr;
}
//---------------------------------------------------------------------------
String Buffer::GetLine(Iter * line, bool replaceTabs)
{
  Iter * itr = line->Duplicate();
  itr->GoLineStart();
  String str;
  int pos = 0;
  while(*(itr->ptr) != '\n')
  {
    if(*(itr->ptr) != '\t' || !replaceTabs)
    {
      str += *(itr->ptr);
      pos++;
    }
    else
    {
      for(int i = TAB_WIDTH - (pos+1)%TAB_WIDTH; i > 0; i--)
      {
        pos++;
        str += " ";
      }
    }
    itr->GoChar();
  }

  delete itr;
  return str;
}
//---------------------------------------------------------------------------
String Buffer::GetLineTo(Iter* To, bool replaceTabs)
{
  Iter * itr = To->Duplicate();
  itr->GoLineStart();
#ifdef ALLOW_TABS
  if(!replaceTabs)
  {
    String ret = GetText(itr, To);
    delete itr;
    return ret;
  }
  String str;
  int pos = 0;
  while(To->word != itr->word ||To->offset != itr->offset)
  {
    if(*(itr->ptr) != '\t' || !replaceTabs)
    {
      str += *(itr->ptr);
      pos++;
    }
    else
    {
      for(int i = TAB_WIDTH - (pos+1)%TAB_WIDTH; i > 0; i--)
      {
        pos++;
        str += " ";
      }
    }
    itr->GoChar();
  }

  delete itr;
  return str;
#else
  String ret = GetText(itr, To);
  delete itr;
  return ret;
#endif
}
//---------------------------------------------------------------------------
int Buffer::GetLineCount()
{
  return data->linecount;
}
//---------------------------------------------------------------------------
void Buffer::RegisterIP(IPos * itr)
{
  ItrList.push_back(itr);
}
//---------------------------------------------------------------------------
void Buffer::UnregisterIP(IPos * itr)
{
  ItrList.remove(itr);
}
//---------------------------------------------------------------------------
void Buffer::RegisterF(SHEdit::Format * f)
{
  FormatList.insert(f);
}
//---------------------------------------------------------------------------
void Buffer::UnregisterF(SHEdit::Format * f)
{
  FormatList.erase(f);
}
//---------------------------------------------------------------------------
void Buffer::RegisterIM(IMark * itr)
{
  IMarkList.insert(itr);
}
//---------------------------------------------------------------------------
void Buffer::UnregisterIM(IMark * itr)
{
  IMarkList.erase(itr);
}
//---------------------------------------------------------------------------
void Buffer::ItersTranslateInsert(int linenum, int pos, int bylines, int topos, NSpan * toline)
{
  for (std::list<IPos*>::iterator itr = ItrList.begin(); itr != ItrList.end(); itr++)
  {
#ifdef _DEBUG
    int size = ItrList.size();
    IPos *debug = *itr;
#endif
    if((*itr)->linenum > linenum ||((*itr)->linenum == linenum && (*itr)->pos >= pos))
    {
      (*itr)->linenum += bylines;
      if((*itr)->linenum == linenum+bylines)
      {
        (*itr)->pos += topos - pos;
        (*itr)->line = toline;
      }
    }
    if((*itr)->linenum == linenum || (*itr)->linenum == linenum+bylines)
    {
      (*itr)->UpdatePos();
      (*itr)->Update();
    }
  }
}
//---------------------------------------------------------------------------
void Buffer::ItersTranslateDelete(int fromlinenum, int frompos, int tolinenum, int topos, NSpan * toline)
{
  for (std::list<IPos*>::iterator itr = ItrList.begin(); itr != ItrList.end(); itr++)
  {
#ifdef _DEBUG
    int size = ItrList.size();
    IPos *debug = *itr;
#endif
    if((*itr)->linenum > fromlinenum || ((*itr)->linenum == fromlinenum && (*itr)->pos >= frompos))
    {
      if((*itr)->linenum < tolinenum || ((*itr)->linenum == tolinenum && (*itr)->pos <= topos))
      {
        (*itr)->line = toline;
        (*itr)->linenum = fromlinenum;
        (*itr)->pos = frompos;
      }
      else if((*itr)->linenum == tolinenum && (*itr)->pos > topos)
      {
        (*itr)->line = toline;
        (*itr)->linenum = fromlinenum;
        (*itr)->pos = frompos+(*itr)->pos-topos;
      }
      else if((*itr)->linenum > tolinenum)
      {
        (*itr)->linenum += fromlinenum-tolinenum;
      }
    }
    if((*itr)->linenum == fromlinenum)
    {
      (*itr)->UpdatePos();
      (*itr)->Update();
    }
  }
}
//---------------------------------------------------------------------------
int Buffer::CheckIntegrity(int& emptyCount)
{
#ifdef _DEBUG
  //Write(String("checking integrity"));
#endif
  emptyCount = 0;
  Span* lastword = data->first;
  Span* word = data->first->next;
  NSpan* line = data->firstLine;
  NSpan* nextline = data->firstLine->nextline;

  while(word != data->last)
  {
    if(word->prev != lastword)
      return 1;
    if(*(word->string) == '\n')
    {
      if(word != (Span*)nextline)
        return 2;
      line = (NSpan*)word;
      nextline = line->nextline;
      /*
         for (std::list<Iter*>::const_iterator itr = line->ItrList.begin(); itr != line->ItrList.end(); ++itr)
         {
         try
         {
         (*itr)->word->prev++;
         (*itr)->word->prev--;
         }
         catch(...)
         {
         return 3;
         }
         }  */
    }
    if(*(word->string) == '\0')
      emptyCount++;
    lastword = word;
    word = word->next;
  }
#ifdef _DEBUG
  //Write(String("integrity check done"));
#endif
  return 0;
}
//---------------------------------------------------------------------------
#ifdef _DEBUG
void Buffer::Write(AnsiString message)
{
#ifdef _DEBUG_LOGGING
  myfile << message.c_str() << std::endl;
#endif
}
#endif
//---------------------------------------------------------------------------
int Buffer::GetItrCount()
{
  return ItrList.size();
}
//---------------------------------------------------------------------------
