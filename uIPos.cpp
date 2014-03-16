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
  std::list<IPos*> searchtree; //dunno why i called it a tree
  for (std::set<Format*>::iterator itr = buffer->FormatList.begin(); itr != buffer->FormatList.end(); itr++)
  {
    IMark * m = (*itr)->GetMarkBefore(this);
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
bool IPos::Compare(const IPos*& a, const IPos*& b)
{
  if(a->linenum < b->linenum)
    return true;
  if(a->linenum == b->linenum)
  {
    if( a->pos < b->pos)
      return true;
  }
  return false;
}
//---------------------------------------------------------------------------
