//---------------------------------------------------------------------------
#pragma hdrstop


#include "uMark.h"
#include "uBuffer.h"

using namespace SHEdit;

#pragma package(smart_init)
//---------------------------------------------------------------------------
Mark::Mark(SHEdit::Format * format, bool begin, short pos)
{
  this->format = format;
  this->begin = begin;
  this->pos = pos;
}
//---------------------------------------------------------------------------

  IMark::IMark(SHEdit::Format * format, bool begin, IPos * itr)
: IPos(itr->buffer, itr->line, itr->linenum, itr->pos)
{
  this->type = IPType::iptMark;
  this->format = format;
  this->begin = begin;
  format->AddIM(this);
  buffer->RegisterIM(this);
  buffer->RegisterF(format);
}
//---------------------------------------------------------------------------
IMark::~IMark()
{
  buffer->UnregisterIM(this);
  format->RemoveIM(this);
}
//---------------------------------------------------------------------------
