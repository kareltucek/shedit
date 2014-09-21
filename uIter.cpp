//---------------------------------------------------------------------------


#pragma hdrstop


#include "uIter.h"
#include "uSpan.h"
//#include "uMark.h"
#include "uFormat.h"
#include "uBuffer.h"
#include <assert.h>

#include <vcl.h>

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
  Iter::Iter()
: IPos(), word(NULL), offset(0), ptr(NULL)
{
}
//---------------------------------------------------------------------------
  Iter::Iter(const Iter& it)
: IPos(it)
{
  this->offset = it.offset;
  this->ptr = it.ptr;
  this->word = it.word;
  this->nextimark = it.nextimark;
  this->nextimarkln = it.nextimarkln;
}
//---------------------------------------------------------------------------
  Iter::Iter(NSpan * line, int linenum, int pos, Buffer * buffer)
: IPos(buffer, line, linenum, pos)
{
  this->word = line;
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
bool Iter::Valid()
{
  return this->IPos::Valid();
}
//---------------------------------------------------------------------------
void Iter::Invalidate()
{
  this->IPos::Invalidate();
  word = NULL;
  offset = 0;
  ptr = NULL;
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
  {
    GoLineStart(); //not sure whether I can break anything; The problem is on first line - one expects this to return handle to the beginning of line
    return false;
  }
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
wchar_t Iter::GetChar()
{
  return *ptr;
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
bool Iter::operator<(const Iter& itr)
{
  if(linenum < itr.linenum)
    return true;
  if(linenum == itr.linenum && pos <= itr.pos)
    return true;
  return false;
}
//---------------------------------------------------------------------------
bool Iter::operator>(const Iter& itr)
{
  if(linenum < itr.linenum)
    return false;
  if(linenum == itr.linenum && pos <= itr.pos)
    return false;
  return true;
}
//---------------------------------------------------------------------------
bool Iter::operator==(const Iter& itr)
{
  assert(!(!(this->word == itr.word && this->offset == itr.offset) && ((IPos)(*this) == (IPos)itr)));
  return (this->word == itr.word && this->offset == itr.offset);
}
//---------------------------------------------------------------------------
bool Iter::operator!=(const Iter& itr)
{
  return !(*this == itr);
}
//---------------------------------------------------------------------------
Iter& Iter::operator++()
{
  GoChar();
  return *this;
}
//---------------------------------------------------------------------------
Iter& Iter::operator--()
{
  RevChar();
  return *this;
}
//---------------------------------------------------------------------------
wchar_t& Iter::operator*()
{
  return *(this->ptr);
}

//---------------------------------------------------------------------------
Iter& Iter::operator=(const Iter& itr)
{
  if(&itr == this)
    return *this;

  ((IPos*)this)->operator=((const IPos&)itr);
  word = itr.word;
  offset = itr.offset;
  ptr = itr.ptr;
  return *this;
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
void Iter::GoBy(int chars, bool multiline)
{
  if(word->next == NULL)
    return;
  if(chars < 0)
  {
    GoLeft(-chars, multiline);
    return;
  }
  while(this->word->length - this->offset <= chars && (multiline || *(word->string) != '\n'))
  {
    chars -= this->word->length - this->offset;
    if(!GoWord())
      break;
  }
  this->offset += chars;
  this->pos += chars;
  if(this->offset > word->length)
  {
    this->offset = word->length-1;
    this->pos = -chars+word->length-1;
  }
  Update();
}
//---------------------------------------------------------------------------
void Iter::GoLeft(int chars, bool multiline)
{
  if(word->prev == NULL || (word->prev->prev == NULL && offset == 0))
    return;
  if(chars < 0)
  {
    GoBy(-chars, multiline);
    return;
  }
  while(this->offset < chars && (multiline || word->prev != (Span*)line))
  {
    chars -= 1+ this->offset;
    if(!RevWord())
      break;
  }

  this->offset -= chars;
  this->pos -= chars;

  if(this->offset < 0)
  {
    this->pos = 0;
    this->offset = 0;
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
int Iter::GetDistance(Iter* second)
{
  if(buffer == NULL || second->buffer == NULL)
  {
    throw std::invalid_argument( "cant get distance of a positionless iterator" );
    return -1;
  }

  if(*second < *this)
    return second->GetDistance(this);

  Iter* itr = this->Duplicate();
  int len=0;
  while(itr->word != second->word)
  {
    len += itr->word->length - itr->offset;
  }
  len += second->offset;
  delete itr;
  return len;
}
//---------------------------------------------------------------------------
String Iter::GetLine()
{
  if(buffer == NULL)
  {
    throw std::invalid_argument( "Cant return line from positionless iterator; use Buffer::GetLine(Iter*) instead" );
    return "";
  }
  else
    return buffer->GetLine(this, false);

}
//---------------------------------------------------------------------------
bool Iter::FindNext(wchar_t * string, bool skip, bool caseSensitive, bool wholeword)
{
  if(skip)
    GoChar();

  do
  {
    if(IsUnderCursor(string, caseSensitive, wholeword))
      return true;
  }
  while (GoChar());

  return false;
}
//---------------------------------------------------------------------------
bool Iter::FindPrev(wchar_t * string, bool skip, bool caseSensitive, bool wholeword)
{
  if(skip)
    RevChar();

  do
  {
    if(IsUnderCursor(string, caseSensitive, wholeword))
      return true;
  }
  while (RevChar());

  return false;
}
//---------------------------------------------------------------------------
bool Iter::IsUnderCursor(const wchar_t *& string, bool caseSensitive, bool wholeword)
{
  if(towupper(*ptr) != towupper(*string) || (caseSensitive && *ptr != *string))
    return false;

  Iter * itr = this->Duplicate();
  itr->GoChar();

  const wchar_t * xptr = string+1;

  while(*xptr != '\0')
  {
    if(*xptr == *(itr->ptr) || (!caseSensitive && towupper(*(itr->ptr) == towupper(*xptr))))
      xptr++;
    else
    {
      delete itr;
      return false;
    }

    if(!itr->GoChar())
    {
      delete itr;
      return false;
    }
  }
  bool result = !wholeword || !iswalpha(itr->GetNextChar());
  delete itr;
  return result;
}
//---------------------------------------------------------------------------
bool Iter::LineIsEmpty()
{
  if(line->next != NULL && line->next == (Span*)line->nextline)
    return true;
    return false;
}
//---------------------------------------------------------------------------
int Iter::GetLineNum()
{
  return linenum;
}
//---------------------------------------------------------------------------
void Iter::GoWordLiteral()
{
  while ((iswalnum(*ptr) || *ptr == '_') && GoChar());
  while ((!(iswalnum(*ptr) || *ptr == '_')) && GoChar());
}
//---------------------------------------------------------------------------
void Iter::GoWordEndLiteral()
{
  while ((!(iswalnum(*ptr) || *ptr == '_')) && GoChar());
  while ((iswalnum(*ptr) || *ptr == '_') && GoChar());
}
//---------------------------------------------------------------------------
void Iter::RevWordLiteral()
{
  RevChar();
  while ((!(iswalnum(*ptr) || *ptr == '_')) && RevChar());
  while ((iswalnum(*ptr) || *ptr == '_') && RevChar());            //this moves one char before the word
  if( word->prev->prev != NULL || offset != 0)                     //this corrects the one char
    GoChar();
}
//---------------------------------------------------------------------------
