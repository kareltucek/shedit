//---------------------------------------------------------------------------


#pragma hdrstop


#include "uSpan.h"
#include "uIter.h"
#include <string>
#include <stdio.h>
#include <list>

using namespace SHEdit;

#pragma package(smart_init)
//---------------------------------------------------------------------------
Span::Span(Iter * After)
{
  prev = After->word;
  next = After->word->next;
  string = NULL;
  mark = NULL;
  length = 0;
}
//---------------------------------------------------------------------------
Span::Span(Span * afterword)
{
  prev = afterword;
  if(afterword)
    next = afterword->next;
  string = NULL;
  mark = NULL;
  length = 0;
}
//---------------------------------------------------------------------------
Span::Span()
{
  prev = NULL;
  next = NULL;
  string = NULL;
  mark = NULL;
  length = 0;
}
//---------------------------------------------------------------------------
Span::Span(Span * prev, Span * next, wchar_t * string, short length)
{
  mark = NULL;
  this->prev = prev;
  this->next = next;
  this->string = string;
  this->length = length;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
  NSpan::NSpan(Iter * After)
: Span(After)
{
  prevline = After->line;
  nextline = After->line->nextline;
  string = new wchar_t[2];
  wcscpy(string, L"\n");
  length = 1;
}
//---------------------------------------------------------------------------
  NSpan::NSpan(Span* afterword, NSpan* afterline)
: Span(afterword)
{
  prevline = afterline;
  nextline = afterline->nextline;
  string = new wchar_t[2];
  wcscpy(string, L"\n");
  length = 1;
}
//---------------------------------------------------------------------------
  NSpan::NSpan()
: Span()
{
  prevline = NULL;
  nextline = NULL;
  string = new wchar_t[2];
  wcscpy(string, L"\n");
  length = 1;
}
//---------------------------------------------------------------------------
void NSpan::ItersSplit(Span * from, Span * toFirst, Span * toSec, int byPos, bool custoff, int tooffset)
{
  for (std::list<Iter*>::const_iterator itr = ItrList.begin(); itr != ItrList.end(); ++itr)
  {
    if((*itr)->word == from)
    {
      if((*itr)->offset < byPos)
      {
        (*itr)->word = toFirst;
      }
      else
      {
        (*itr)->word = toSec;
        if(custoff)
          (*itr)->offset = tooffset;
        else
          (*itr)->offset -= byPos;
      }
      (*itr)->Update();
    }
  }
}
//---------------------------------------------------------------------------
void NSpan::Invalidate(Span * from)
{
  for (std::list<Iter*>::const_iterator itr = ItrList.begin(); itr != ItrList.end(); ++itr)
  {
    if((*itr)->word == from)
    {
      (*itr)->Update();
    }
  }
}
//---------------------------------------------------------------------------
void NSpan::ItersTranslate(Span * from, Span * to, int byPos, int inc)
{
  for (std::list<Iter*>::const_iterator itr = ItrList.begin(); itr != ItrList.end(); ++itr)
  {
    if((*itr)->word == from)
    {
      (*itr)->word = to;
      if((*itr)->offset >= byPos)
      {
        if(inc >0)  //for insertions
          (*itr)->offset += inc;
        else        //for deletions - case when iterator is in the middle of delet has to be handled
        {
          if((*itr)->offset < byPos-inc)
            (*itr)->offset = byPos;
          else
            (*itr)->offset += inc;
        }
      }
      (*itr)->Update();
    }
  }
}
//---------------------------------------------------------------------------
void NSpan::ItersTransmitAll(NSpan * to)
{
  if(to == this)
    return;
  while(ItrList.size() > 0)
  {
    to->Register(ItrList.front());
    ItrList.front()->line = to;
    ItrList.pop_front();
  }
}
//---------------------------------------------------------------------------
void NSpan::ItersTransmit(Span * from, NSpan * to)
{
  for (std::list<Iter*>::iterator itr = ItrList.begin(); itr != ItrList.end(); )
  {
    if((*itr)->word == from)
    {
      to->Register(*itr);
      (*itr)->line = to;
      itr = ItrList.erase(itr);
    }
    else
      itr++;
  }
}
//---------------------------------------------------------------------------
void NSpan::ItersMove(Span * from, NSpan * to, Span * toword, int offset)
{
  for (std::list<Iter*>::iterator itr = ItrList.begin(); itr != ItrList.end(); itr++)
  {
    if((*itr)->word == from)
    {
      (*itr)->line = to;
      (*itr)->word = toword;
      (*itr)->offset = offset;
      (*itr)->Update();
    }
  }
}
//---------------------------------------------------------------------------
void NSpan::Register(Iter* itr)
{
  ItrList.push_back(itr);
}
//---------------------------------------------------------------------------
void NSpan::Unregister(Iter* itr)
{
  ItrList.remove(itr);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

Range::Range(Span * first, Span * last, bool lineempty, NSpan * firstLine, NSpan * lastLine, bool empty)
{
  this->first = first;
  this->last = last;
  this->firstLine = firstLine;
  this->lastLine = lastLine;
  this->empty = empty;
  this->lineempty = lineempty;
}
//---------------------------------------------------------------------------
Range::~Range()
{

}
//---------------------------------------------------------------------------
