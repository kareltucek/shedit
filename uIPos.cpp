//---------------------------------------------------------------------------


#pragma hdrstop

#include "uIPos.h"
#include "uMark.h"
#include "uBuffer.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

using namespace SHEdit;
//---------------------------------------------------------------------------
IPos::IPos(Buffer * buffer, NSpan * line, int linenum, int pos)
{
  this->type = IPType::iptPos;
  this->pos = pos;
  this->linenum = linenum;
  this->line = line;
  this->buffer = buffer;
  if(buffer)
    buffer->RegisterIP(this);
}
//---------------------------------------------------------------------------
IPos::IPos(const IPos& ip)
{
  this->type = ip.type;
  this->pos = ip.pos;
  this->linenum = ip.linenum;
  this->line = ip.line;
  this->buffer = ip.buffer;
  if(buffer)
    buffer->RegisterIP(this);
}
//---------------------------------------------------------------------------
IPos::~IPos()
{
  if(buffer != NULL)
    buffer->UnregisterIP(this);
}
//---------------------------------------------------------------------------
void IPos::Update()
{}
//---------------------------------------------------------------------------
void IPos::UpdatePos()
{}
//---------------------------------------------------------------------------
void IPos::RecalcPos()
{}
//---------------------------------------------------------------------------
IMark* IPos::IMarkupBegin(SHEdit::Format * format)
{
  return new IMark(format, true, this);
}
//---------------------------------------------------------------------------
IMark* IPos::IMarkupEnd(SHEdit::Format * format)
{
  return new IMark(format, false, this);
}
//---------------------------------------------------------------------------
FontStyle IPos::ReconstructIMarkFontStyle()
{
  FontStyle st = FontStyle();
  if(buffer == NULL)
    return st;
  std::list<IPos*> searchtree; //dont know why i called it a tree
  for (std::set<Format*>::iterator itr = buffer->FormatList.begin(); itr != buffer->FormatList.end(); itr++)
  {
    IMark * m = (*itr)->GetMarkBefore(this);
    assert(m==NULL || m->type == IPType::iptMark);
    if(m != NULL && m->begin)
      searchtree.push_back(m);
  }
  searchtree.sort(IPos::Compare);
  for (std::list<IPos*>::iterator itr = searchtree.begin(); itr != searchtree.end(); itr++)
    st += *(((IMark*)(*itr))->format);
  return st;
}
//---------------------------------------------------------------------------
bool IPos::compare::operator()(const IPos* a, const IPos* b) const
{
  return IPos::Compare(a, b);
}
//---------------------------------------------------------------------------
      bool IPos::operator==(const IPos& p)
      {
        return (p.pos == this->pos && p.linenum == this->linenum);
      }
//---------------------------------------------------------------------------
bool IPos::Compare(const IPos*& a, const IPos*& b)
{
  if(a->linenum < b->linenum)
    return true;
  if(a->linenum == b->linenum)
  {
    if( a->pos < b->pos)
      return true;
    else if(a->pos == b->pos)
    {
      if(a->type == IPType::iptMark  && b->type == IPType::iptMark)
      {
        if (((IMark*)a)->begin == false && ((IMark*)b)->begin == true)
          return true;
        else if (((IMark*)a)->begin == ((IMark*)b)->begin)
          return (int)a < (int)b;
        else
          return false;
      }
    }
  }
  return false;
}
//---------------------------------------------------------------------------
