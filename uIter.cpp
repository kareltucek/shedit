//---------------------------------------------------------------------------


#pragma hdrstop


#include "uIter.h"
#include "uSpan.h"
#include "uMark.h"
#include "uFormat.h"

using namespace SHEdit;

#pragma package(smart_init)
//---------------------------------------------------------------------------
Iter::Iter(NSpan * line)
{
  if(line->next)
  {
    this->offset = 0;
    this->word = line->next;
    this->line = line;
    line->Register(this);
    this->Update();
  }
  else
  {
    this->offset = 0;
    this->word = (Span*)line;
    this->line = line;
    line->Register(this);
  }
}
//---------------------------------------------------------------------------
Iter::Iter(int offset, Span * word, NSpan * line)
{
  this->offset = offset;
  this->word = word;
  this->line = line;
  line->Register(this);
  this->Update();
}
//---------------------------------------------------------------------------
Iter::~Iter()
{
  line->Unregister(this);
}
//---------------------------------------------------------------------------
bool Iter::GoLine(bool allowEnd)
{
  if(line->nextline && line->nextline->next)
  {
    line->Unregister(this);
    line = line->nextline;
    line->Register(this);
    word = line->next;
    offset = 0;
    Update();
    return true;
  }
  else if(line->nextline)
  {
    if(allowEnd && line->nextline)
    {
      line = line->nextline;
      word = line;
      offset = 0;
      Update();
    }
    return false;
  }
  return false;
}
//---------------------------------------------------------------------------
bool Iter::RevLine()
{
  if(line->prevline)
  {
    line->Unregister(this);
    line = line->prevline;
    line->Register(this);
    word = line->next;
    offset = 0;
    Update();
    return true;
  }
  else
    return false;
}
//---------------------------------------------------------------------------
bool Iter::GoWord()
{
  if(word->next)
  {
    if(*(word->string) == '\n')
    {
      line->Unregister(this);
      line = ((NSpan*)word);
      line->Register(this);
    }
    word = word->next;
    offset = 0;
    Update();
    return true;
  }
  else
    return false;
}
//---------------------------------------------------------------------------
bool Iter::RevWord()
{
  if(word->prev->prev)
  {
    word = word->prev;
    offset = word->length-1;
    if(*(word->string) == '\n' && word->prev)
    {
      line->Unregister(this);
      line = ((NSpan*)word)->prevline;
      line->Register(this);
    }
    Update();
    return true;
  }
  else
    return false;
}

//---------------------------------------------------------------------------
bool Iter::GoChar()
{
  if(offset >= word->length-1)
    return GoWord();
  else
  {
    offset++;
    ptr++;
  }
  return true;
}
//---------------------------------------------------------------------------
bool Iter::RevChar()
{
  offset--;
  if(offset < 0)
  {
    if(!RevWord())
    {
      offset = 0;
      return false;
    }
  }
  return true;
}
//---------------------------------------------------------------------------
Iter * Iter::Duplicate()
{
  return new Iter(offset, word, line);
}
//---------------------------------------------------------------------------
void Iter::Update()
{
  ptr = word->string + offset;
}
//---------------------------------------------------------------------------
bool Iter::operator==(const Iter& itr)
{
  return (this->word == itr.word && this->offset == itr.offset);
}
//---------------------------------------------------------------------------
bool Iter::operator!=(const Iter& itr)
{
  return !(this->word == itr.word && this->offset == itr.offset);
}
//---------------------------------------------------------------------------
wchar_t& Iter::operator++()
{
  GoChar();
  return *ptr;
}
//---------------------------------------------------------------------------
wchar_t& Iter::operator--()
{
  RevChar();
  return *ptr;
}
//---------------------------------------------------------------------------
void Iter::MarkupBegin( SHEdit::Format * format)
{
  MarkupBegin(&(word->mark), offset, true, format);
}
//---------------------------------------------------------------------------
void Iter::MarkupEnd( SHEdit::Format * format)
{
  MarkupBegin(&(word->mark), offset, false, format);
}
//---------------------------------------------------------------------------
void Iter::MarkupBegin(SHEdit::Mark ** at, int pos, bool begin, SHEdit::Format * format)
{
  *at = new Mark(format, begin, pos, *at, at);
  if((*at)->mark)
    (*at)->mark->parent = &((*at)->mark);
  format->Add(*at);
}

//---------------------------------------------------------------------------
void Iter::MarkupRem(SHEdit::Format * format)
{
  if(word->mark != NULL)
  {
    Mark ** m = &(word->mark);
    while(*m != NULL)
    {
      if((*m)->format == format && (*m)->pos == offset)
      {
        Mark *n = *m;
        *m = (*m)->mark;
        if(*m != NULL)
          (*m)->parent = m;
        format->Remove(n);
        delete n;
      }
      m = &((*m)->mark);
    }
  }
}
//---------------------------------------------------------------------------