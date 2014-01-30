//---------------------------------------------------------------------------


#pragma hdrstop

#include "uBuffer.h"
#include "uSpan.h"
#include "uIter.h"
#include "uMark.h"
#include <stack>
#include <wchar.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <wstring.h>
#include <ctype.h>

using namespace SHEdit;
std::ofstream myfile;

#pragma package(smart_init)
//---------------------------------------------------------------------------
Buffer::Buffer()
{
#ifdef DEBUG
  myfile.open("buffer.txt", ios::out);
#endif
  NSpan * head = new NSpan();
  NSpan * tail = new NSpan();

  markupMask = 0;

  head->next = tail;
  head->nextline = tail;
  tail->prev = head;
  tail->prevline = head;

  data = new Range(head, tail, false, head, tail, false, 0);
  preload = new Range(NULL, NULL, false, NULL, NULL, false, 0);

}
//---------------------------------------------------------------------------
Buffer::~Buffer()
{
}
//---------------------------------------------------------------------------
Iter * Buffer::Begin()
{
  return new Iter(0, data->first->next, data->firstLine);
}
//---------------------------------------------------------------------------
Iter * Buffer::First()   //technically shows wrong location - just if we NEED to maintain a link to fist link no matter what gets inserted
{
  return new Iter(1, data->first, data->firstLine);
}
//---------------------------------------------------------------------------
NSpan * Buffer::FirstLine()
{
  return data->firstLine;
}
//---------------------------------------------------------------------------
Iter * Buffer::End()
{
  return new Iter(0, data->last, data->lastLine->prevline);
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

#ifdef DEBUG
  //Write("delete words "+String((int)word->prev)+String(" ")+String((int)word->next));
#endif
  word->next->prev = word->prev;
  word->prev->next = word->next;
#ifdef DEBUG
  //Write("delete words returning");
#endif
}
//---------------------------------------------------------------------------
void Buffer::_Insert(NSpan * word)
{
  word->nextline->prevline = word;
  word->prevline->nextline = word;
  _Insert((Span*)word);
  for(Span * span = word->next; *(span->string) != '\n'; span = span->next)
    word->prevline->ItersTransmit(span, word);
  word->prevline->ItersTransmit(word->nextline, word);
}
//---------------------------------------------------------------------------
void Buffer::_Delete(NSpan * word)
{
  word->prevline->ItersMove(word, word->prevline, word->next, 0); //some fix - not sure what i am doing
  word->ItersTransmitAll(word->prevline);
  word->nextline->prevline = word->prevline;

  if(word->nextline != NULL);
    word->nextline->prevline = word->prevline;
  if(word->prevline)
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

  if(At->word->mark)
  {
    for(Mark ** m = &(At->word->mark); *m != NULL; m = &((*m)->mark))
      if((*m)->pos < At->offset)
        Iter::MarkupBegin(&(firstnew->mark), (*m)->pos, (*m)->begin, (*m)->format);
  }
  return firstnew;
}
//---------------------------------------------------------------------------
Span* Buffer::_SplitEnd(Iter * At)
{
  wchar_t* secnewstr = new wchar_t[At->word->length-At->offset+1];
  wcscpy( secnewstr, At->word->string+At->offset);
  Span * secondnew = new Span(NULL, At->word->next, secnewstr, At->word->length-At->offset);

  if(At->word->mark)
  {
    for(Mark ** m = &(At->word->mark); *m != NULL; m = &((*m)->mark))
      if((*m)->pos >= At->offset)
        Iter::MarkupBegin(&(secondnew->mark), (*m)->pos-At->offset, (*m)->begin, (*m)->format);
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
  At->line->ItersSplit(At->word, firstnew, secondnew, At->offset);
  return firstnew;
}
//---------------------------------------------------------------------------
int Buffer::Insert(Iter * At, wchar_t * string)
{
  if(IsPlainWord(string))
  {
    if(At->offset == 0 && At->word->prev->length < 20 && IsPlainWord(At->word->prev->string))
    {
      _InsertAt(At->line, At->word->prev, At->word->prev->length, string, true, false);
      return 0;
    }
    else if(At->word->length < 20 && IsPlainWord(At->word->string))
    {
      _InsertAt(At->line, At->word, At->offset, string, true, false);
      return 0;
    }
  }

  wchar_t * ptrend = string + wcslen(string);
  wchar_t * ptr = string;
  int linesInserted = 0;
  Span * prev;
  NSpan * prevN;
  NSpan * FirstN;
  if(At->offset == 0)
  {
    prev = At->word->prev;
    prevN = At->line;
    UndoPush(new Range(prev, prev->next, true, At->line, At->line->nextline, true, 0));
  }
  else
  {
    UndoPush(new Range(At->word, At->word, false, At->line, At->line->nextline, true, 0));
    prev = _SplitAt(At);
    prevN = At->line;
  }
  //fix
  if(prevN->nextline == NULL)
    prevN = prevN->prevline;

  bool first = true;
  while(ptr < ptrend)
  {
    wchar_t * word = _ParseWord(ptr, ptrend) ;    
    if(*word == '\n')
    {
      prevN = new NSpan(prev, prevN);
      prev = prevN;
      _Insert(prevN);
      linesInserted++;
    }
    else
    {
      prev = new Span(prev, prev->next, word, wcslen(word));
      _Insert(prev);
    }
  }
  delete[] string;
  wordBeingEdited = prev;
  data->linecount += linesInserted;
  return linesInserted;
}
//---------------------------------------------------------------------------

int Buffer::Delete(Iter * From, Iter * To)
{
  if(From->word == To->word || (*(From->word->string) != '\n' && From->word->next == To->word && To->offset == 0))
  {
    if(To->offset == 0 && From->offset == 0 && From != To)
    {
      UndoPush(new Range(From->word, From->word, false, From->line->nextline, From->line, false, 0));
      _Delete(From->word);
      From->line->ItersMove(From->word, From->line, To->word, 0);
    }
    else
      _DeleteAt(From, To, true, false);
    return 0;
  }
  else if(*(From->word->string) == '\n' && From->word->next == To->word && To->offset == 0)
  {
    UndoPush(new Range(From->word, From->word, false, From->line->nextline, From->line, false, 1));
    _Delete((NSpan*)(From->word));
    data->linecount--;
    return 1;
  }
  else
  {

#ifdef DEBUG
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
    UndoPush(new Range(undoBegin, undoEnd, false, From->line->nextline, To->line, false, 0));
    begin->next = end;
    end->prev = begin;
    From->line->nextline = To->line->nextline;
    To->line->nextline->prevline = From->line;
    _Insert(begin);
    _Insert(end);
    //fix iters
    NSpan * lastN = From->line;
    for(Span * span = undoBegin; span != undoEnd->next; span = span->next)
    {
      lastN->ItersMove(span, From->line, end, 0);  //modifies all iterators to point to first word after deletion
      lastN->ItersTransmitAll(From->line);         //moves all references to iterators from their formar NLs to previous newline
      if(*(span->string) == '\n')
      {
        lastN = (NSpan*)span;
        linesDeleted++;
      }
    }
    stackUndo.top()->linecount = linesDeleted;
    data->linecount -= linesDeleted;
    return linesDeleted;
  }
}


//---------------------------------------------------------------------------
void Buffer::_DeleteAt(Iter * From, Iter * To, bool writeundo, bool forcenew)
{
  if(*(From->word->string) == '\0' && From->word->next && From->word->next == To->word && To->offset == 0)
  {
    _Delete(From->word);
    return;
  }

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
      UndoPush(new Range(From->word, From->word, false, From->line->nextline, To->line, false, 0));
    _Insert(newword);
    From->line->ItersTranslate(From->word, newword, From->offset, -length);
    From->line->ItersSplit(newword, newword, newword->next, newword->length, true, 0);
    if(From->word->mark)
      for(Mark ** m = &(From->word->mark); *m != NULL; m = &((*m)->mark))
        Iter::MarkupBegin(&(newword->mark), (*m)->pos, (*m)->begin, (*m)->format);
  }
  else
  {
    delete[] From->word->string;
    From->word->string = newstr;
    From->word->length = wcslen(newstr);
    From->line->ItersTranslate(From->word, From->word, From->offset, -length);
    From->line->ItersSplit(From->word, From->word, From->word->next, From->word->length, true, 0);
    newword = From->word;
  }
  if(newword->mark)
  {
    for(Mark ** m = &(newword->mark); *m != NULL; m = &((*m)->mark))
    {
      if((*m)->pos >= To->offset+length)
        (*m)->pos -=length;
      else if((*m)->pos >= To->offset)
        (*m)->pos -= (*m)->pos - From->offset;
    }
  }
}

//---------------------------------------------------------------------------
void Buffer::_InsertAt(NSpan * line, Span * word, int pos, wchar_t * string, bool writeundo, bool forcenew)   //OK
{
  //wchar_t* newstr = (wchar_t*) malloc((at->word->length+wcslen(string)+1) * sizeof(wchar_t));
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
      UndoPush(new Range(word, word, false, line->nextline, line, false, 0));
    _Insert(newword);
    line->ItersTranslate(word, newword, pos, wcslen(string));
    if(word->mark)
      for(Mark ** m = &(word->mark); *m != NULL; m = &((*m)->mark))
        Iter::MarkupBegin(&(newword->mark), (*m)->pos, (*m)->begin, (*m)->format);
  }
  else
  {
    delete[] word->string;
    word->string = newstr;
    word->length = wcslen(newstr);
    line->ItersTranslate(word, word, pos, wcslen(string));
    newword = word;
  }
  if(word->mark)
    for(Mark ** m = &(newword->mark); *m != NULL; m = &((*m)->mark))
      if((*m)->pos >= pos)
        (*m)->pos += wcslen(string);
}

//---------------------------------------------------------------------------
wchar_t* Buffer::_ParseWord(wchar_t*& ptr, wchar_t*& ptrend)             //OK
{
  short type;
  wchar_t * ptrstart = ptr;
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
  if(*ptr == '\n')
  {
    ptr ++;
  }
  else
  {
    while(*ptr != '\n' && *ptr != '\0')
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
bool Buffer::IsPlainWord(wchar_t * string)                //OK
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
void Buffer::LoadFile(wchar_t * filename)
{
  LoadFileAsync(filename);
  Preload(-1);
  FlushPreload();
}
//---------------------------------------------------------------------------
void Buffer::LoadFileAsync(wchar_t * filename)
{
  preloadFile = new std::ifstream();
  preloadFile->open("test.txt");
  preload->first = data->first;
  preload->firstLine = data->firstLine;
  preload->last = data->last;
  preload->lastLine = data->lastLine;
  data->linecount = 0;
}
//---------------------------------------------------------------------------
void Buffer::Redo()
{
  UndoRedo(&stackRedo, &stackUndo);
}
//---------------------------------------------------------------------------
void Buffer::Undo()
{
  UndoRedo(&stackUndo, &stackRedo);
}
//---------------------------------------------------------------------------
void Buffer::UndoPush(Range * event)
{
   stackUndo.push(event);
   while(stackRedo.size() > 0)
   {
     Range * redoevent = stackRedo.top();
     stackRedo.pop();
     redoevent->Free();
     delete redoevent;
   }
}
//---------------------------------------------------------------------------
void Buffer::UndoRedo(std::stack<Range*> * stackUndo, std::stack<Range*> * stackRedo)
{
#ifdef DEBUG
  //Write(String("undo called"));
#endif
  if((*stackUndo).size() == 0)
    return;
  Range * event = (*stackUndo).top();
  (*stackUndo).pop();

   //undo = lines that are gonna to be removed from buffer
   //rep = lines that are  gonna to be placed to buffer
   Span * undoFirst = event->empty ? event->first->next : event->first->prev->next;
   Span * undoLast = event->empty ? event->last->prev : event->last->next->prev;
   NSpan * undoLineFirst = event->lineempty ? event->firstLine : event->firstLine->prevline;
   NSpan * replLineLast = event->lineempty ? event->firstLine : event->lastLine;


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
  this->data->linecount = data->linecount - linestoberemoved + event->linecount;


  {
    Span * redoFirst;
    Span * redoLast;
    bool redoempty;
    NSpan * redoFirstLine;
    NSpan * redoLastLine;
    bool redoemptyline;

    if(event->empty)
    {
      redoFirst = event->first->next;
      redoLast = event->last->prev;
      redoempty = false;
    }
    else if(event->first->prev == event->last->next->prev)
    {
      redoFirst = event->first->prev;
      redoLast = event->last->next;
      redoempty = true;
    }
    else
    {
      redoFirst = event->first->prev->next;
      redoLast = event->last->next->prev;
      redoempty = false;
    }

    if(event->lineempty)
    {
      redoFirstLine = event->firstLine->nextline;
      redoLastLine = event->lastLine->prevline;
      redoemptyline = false;
    }
    else if(event->firstLine->prevline == event->lastLine->nextline->prevline)
    {
      redoFirstLine = event->firstLine->prevline;
      redoLastLine = event->lastLine->nextline;
      redoemptyline = true;
    }
    else
    {
      redoFirstLine = event->firstLine->prevline->nextline;
      redoLastLine = event->lastLine->nextline->prevline;
    }

    (*stackRedo).push(new Range(redoFirst, redoLast, redoempty, redoFirstLine, redoLastLine, redoemptyline, linestoberemoved));
  }

  if(event->empty)
  {
    event->first->next = event->last;
    event->last->prev = event->first;
  }
  else
  {
    event->first->prev->next = event->first;
    event->last->next->prev = event->last;
  }

  if(event->lineempty)
  {
    event->firstLine->nextline = event->lastLine;
    event->lastLine->prevline = event->firstLine;
  }
  else
  {
    event->firstLine->prevline->nextline = event->firstLine;
    event->lastLine->nextline->prevline = event->lastLine;
  }

  delete event;
#ifdef DEBUG
  //Write(String("undone"));
#endif
}
//---------------------------------------------------------------------------
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
}
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
int Buffer::CheckIntegrity(int& emptyCount)
{
#ifdef DEBUG
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
#ifdef DEBUG
  //Write(String("integrity check done"));
#endif
  return 0;
}
//---------------------------------------------------------------------------
#ifdef DEBUG
void Buffer::Write(AnsiString message)
{
#ifdef DEBUG_LOGGING
  myfile << message.c_str() << std::endl;
#endif
}
#endif
//---------------------------------------------------------------------------
