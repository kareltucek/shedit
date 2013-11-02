//---------------------------------------------------------------------------
#pragma hdrstop


#include "uMark.h"
#include "uFormat.h"
#include "uBuffer.cpp"

using namespace SHEdit;

#pragma package(smart_init)
//---------------------------------------------------------------------------
Format::Format()
{
  foreground = new TColor(clBlack);
  background = new TColor(clWhite);
};
//---------------------------------------------------------------------------
  Format::Format(TColor * foreground, TColor * background)
: FontStyle(foreground, background)
{

};
//---------------------------------------------------------------------------
FontStyle::FontStyle(TColor * foreground, TColor * background)
{
  this->foreground = foreground;
  this->background = background;
};
//---------------------------------------------------------------------------
FontStyle::FontStyle(TColor * foreground)
{
  this->foreground = foreground;
  this->background = NULL;
};
//---------------------------------------------------------------------------
FontStyle::FontStyle()
{
  this->foreground = NULL;
  this->background = NULL;
};
//---------------------------------------------------------------------------
Format::~Format()
{
  //you should definitely not do this
};
//---------------------------------------------------------------------------
void Format::Remove(SHEdit::Mark* mark)
{
  marks.remove(mark);
}
//---------------------------------------------------------------------------
void Format::Add(SHEdit::Mark* mark)
{
  marks.push_front(mark);
}
//---------------------------------------------------------------------------
void Format::RemoveAllMarks()
{
  while(marks.size() > 0)
  {
    Mark * old = marks.front();
    *(old->parent) = old->mark;
    if(old->mark)
      old->mark->parent = old->parent;
    delete old;
    marks.pop_front();
  }
}
//---------------------------------------------------------------------------
bool Format::operator==(const SHEdit::Format& f)
{
  return (*(this->foreground) == *(f.foreground) &&  *(this->background) == *(f.background));
}
//---------------------------------------------------------------------------
bool Format::operator!=(const SHEdit::Format& f)
{
  return (*(this->foreground) != *(f.foreground) ||  *(this->background) != *(f.background));
}
//---------------------------------------------------------------------------
SHEdit::Format& Format::operator=(const SHEdit::Format& f)
{
  if(f.foreground)
    this->foreground = f.foreground;
  if(f.background)
    this->background = f.background;
  return *this;
}
//---------------------------------------------------------------------------
bool FontStyle::operator==(const SHEdit::FontStyle& f)
{
  bool s = this->foreground == f.foreground || (this->foreground && f.foreground && *(this->foreground) == *(f.foreground));
  bool b = this->background == f.background || (this->background && f.background && *(this->background) == *(f.background));
  return b & s;
}
//---------------------------------------------------------------------------
bool FontStyle::operator!=(const SHEdit::FontStyle& f)
{
  return !(*this == f);
}
//---------------------------------------------------------------------------
SHEdit::FontStyle& FontStyle::operator=(const SHEdit::FontStyle& f)
{
  if(f.foreground)
    this->foreground = f.foreground;
  if(f.background)
    this->background = f.background;
  return *this;
}
//---------------------------------------------------------------------------
SHEdit::FontStyle& FontStyle::operator=(const SHEdit::Format& f)
{
  if(f.foreground)
    this->foreground = f.foreground;
  if(f.background)
    this->background = f.background;
  return *this;
}
