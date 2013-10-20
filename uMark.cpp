//---------------------------------------------------------------------------
#pragma hdrstop


#include "uMark.h"

using namespace SHEdit;

#pragma package(smart_init)
//---------------------------------------------------------------------------
Mark::Mark(Format * format, bool begin, short pos, Mark * newchild, Mark ** parent)
{
  this->format = format;
  this->begin = begin;
  this->pos = pos;
  this->parent = parent;
  mark = newchild;
}

