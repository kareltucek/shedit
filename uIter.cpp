//---------------------------------------------------------------------------


#pragma hdrstop


#include "uIter.h"
#include "uSpan.h"
//#include "uMark.h"
#include "uFormat.h"
#include "uBuffer.h"

using namespace SHEdit;

#pragma package(smart_init)

//---------------------------------------------------------------------------
Iter::Iter(NSpan * line)
  : IPos(NULL, line, -1, 0)
{
  this->type = IPType::iptIter;
  this->offset = 0;
  this->word = (Span*)line;
  if(line->prevline)
    this->line = line->prevline;
  else
    this->line = line;
  if(word->next)
    GoChar();
}
//---------------------------------------------------------------------------
Iter::Iter(NSpan * line, int linenum, int pos, Buffer * buffer)
  : IPos(buffer, line, linenum, pos)
{
  this->type = IPType::iptIter;
  UpdatePos();
  Update();
}
//---------------------------------------------------------------------------
Iter::Iter(int offset, Span * word, NSpan * line, Buffer * buffer, int linenum)
  : IPos(buffer, line, linenum, 0)
{
  this->type = IPType::iptIter;
  this->offset = offset;
  this->word = word;

  RecalcPos();
  this->Update();
}
//---------------------------------------------------------------------------
Iter::~Iter()
{
}
//---------------------------------------------------------------------------
bool Iter::GoLine(bool allowEnd)
{
  if(line->nextline && line->nextline->next)
  {
    if(linenum >= 0)
      linenum++;
    line = line->nextline;
    word = line->next;
    offset = 0;
    pos = 0;
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
      RecalcPos();
      Update();
    }
    return false;
  }
  return false;
}
//---------------------------------------------------------------------------
void Iter::GoToLine(int line)
{
  while(line < linenum && this->line->prevline != NULL)
  {
    this->line = this->line->prevline;
    linenum--;
  }
  while(line > linenum && this->line->nextline != NULL)
  {
    this->line = this->line->nextline;
    linenum++;
  }
  if(this->line->nextline == NULL)
  {
    this->line = this->line->prevline;
      linenum--;
  }
  GoLineStart();

}
//---------------------------------------------------------------------------
bool Iter::GoLineEnd()
{
  if(line->nextline)
  {
    word = line->nextline;
    offset = 0;
    RecalcPos();
    Update();
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
bool Iter::GoLineStart()
{
  if(line->next)
  {
    word = line->next;
    offset = 0;
    pos = 0;
    Update();
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
bool Iter::RevLine()
{
  if(line->prevline)
  {
    if(linenum >= 0)
      linenum--;
    line = line->prevline;
    word = line->next;
    offset = 0;
    pos = 0;
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
    pos += word->length - offset;
    if(*(word->string) == '\n')
    {
      line = ((NSpan*)word);
      pos = 0;
      if(linenum >= 0)
        linenum++;
    }
    word = word->next;
    offset = 0;
    if(*(word->string) == '\0')
      return GoWord();
    else
    {
      Update();
      return true;
    }
  }
  else
    return false;
}
//---------------------------------------------------------------------------
wchar_t Iter::GetNextChar()
{
  GoChar();
  wchar_t c = *ptr;
  RevChar();
  return c;
}
//---------------------------------------------------------------------------
bool Iter::RevWord()
{
  if(word->prev->prev)
  {
    pos -= offset + 1;
    word = word->prev;
    while(*(word->string) == '\0')
      word = word->prev;
    offset = word->length-1;
    if(*(word->string) == '\n' && word->prev)
    {
      line = ((NSpan*)word)->prevline;
      if(linenum > 0)
        linenum--;
      RecalcPos();
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
    pos++;
    offset++;
    ptr++;
  }
  return true;
}
//---------------------------------------------------------------------------
bool Iter::RevChar()
{
  offset--;
  ptr--;
  pos--;
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
  return new Iter(offset, word, line, buffer, linenum);
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
  format->Add( word->marks.Push(Mark(format, true, offset)));
}
//---------------------------------------------------------------------------
void Iter::MarkupEnd( SHEdit::Format * format)
{
  format->Add( word->marks.Push(Mark(format, false, offset)));
}
//---------------------------------------------------------------------------
int Iter::GetLeftOffset()
{
#ifdef ALLOW_TABS
  if(this->word->prev == NULL || (*(word->prev->string) == '\n' && offset == 0))
    return 0;
  Iter * itr = this->Duplicate();
  itr->GoLineStart();
  int pos = 0;
  while(itr->word != word || itr->offset != offset)
  {
    if(*(itr->ptr) == '\t')
      pos += TAB_WIDTH - (pos+1)%TAB_WIDTH;
    else
      pos++;
    itr->GoChar();
  }
  delete itr;
  return pos;
#else
  if(word->prev == NULL)
    return 0;
  Span * w = this->word->prev;
  int count = this->offset;
  while(*(w->string) != '\n')
  {
    count += w->length;
    w = w->prev;
  }
  return count;
#endif
}
//---------------------------------------------------------------------------

void Iter::GoByOffset(int chars)
{
#ifdef ALLOW_TABS
  int pos = 0;
  while(pos < chars && *ptr != '\n')
  {
    if(*(ptr) == '\t')
      pos += TAB_WIDTH - (pos+1)%TAB_WIDTH;
    else
      pos++;
    GoChar();
  }
#else
  GoBy(chars);
#endif
}
//---------------------------------------------------------------------------
void Iter::GoBy(int chars)
{
  while(this->word->length - this->offset <= chars && *(word->string) != '\n')
  {
    chars -= this->word->length - this->offset;
    GoWord();
  }
  if(*(word->string) != '\n')
  {
    this->offset = chars;
    this->pos += chars;
  }
  Update();
}

//---------------------------------------------------------------------------
void Iter::MarkupRem(SHEdit::Format * format)
{
  Stack<Mark>::Node * n = word->marks.top;
  while(n != NULL)
  {
    if(n->data.pos == offset && n->data.format == format)
    {
      n->data.format->Remove(n);
      n = n->Remove();
    }
    else
      n = n->next;
  }
}
//---------------------------------------------------------------------------
void Iter::RecalcPos()
{
  this->pos = 0;
  this->pos += offset;
  if(word->prev)
    for(Span * w = word->prev; *(w->string) != '\n'; w = w->prev)
      this->pos += w->length;
}
//---------------------------------------------------------------------------
void Iter::UpdatePos()
{
  int p = this->pos;
  GoLineStart();
    GoBy(p);
}
//---------------------------------------------------------------------------
void Iter::UpdateNextImark()
{
  if(buffer == NULL)
    return;
  std::set<IPos*, IPos::compare>::iterator it = buffer->IMarkList.upper_bound((IPos*)this);
  if(it == buffer->IMarkList.end())
  {
    nextimark = -1;
    nextimarkln = -1;
  }
  else
  {
    nextimark = (*it)->pos;
    nextimarkln = (*it)->linenum;
  }
}
//---------------------------------------------------------------------------

