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
  Format::Format(TColor * foreground, TColor * background, TFontStyles style)
: FontStyle(foreground, background, style)
{

};
//---------------------------------------------------------------------------
FontStyle::FontStyle(TColor * foreground, TColor * background)
  : style()
{
  this->foreground = foreground;
  this->background = background;
};
//---------------------------------------------------------------------------
FontStyle::FontStyle(TColor * foreground, TColor * background, TFontStyles style)
  : style(style)
{
  this->foreground = foreground;
  this->background = background;
};
//---------------------------------------------------------------------------
FontStyle::FontStyle(TColor * foreground)
  : style()
{
  this->foreground = foreground;
  this->background = NULL;
};
//---------------------------------------------------------------------------
FontStyle::FontStyle()
  : style()
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
void Format::Remove( Stack<SHEdit::Mark>::Node* mark)
{
  marks.remove(mark);
}
//---------------------------------------------------------------------------
void Format::Add(Stack<SHEdit::Mark>::Node* mark)
{
  marks.push_front(mark);
}
//---------------------------------------------------------------------------
void Format::RemoveIM( IMark * mark)
{
  imarks.erase(mark);
}
//---------------------------------------------------------------------------
void Format::AddIM(IMark * mark)
{
  imarks.insert(mark);
}
//---------------------------------------------------------------------------
void Format::RemoveAllMarks()
{
  while(!marks.empty())
  {
    marks.front()->Remove();
    marks.pop_front();
  }

  while(!imarks.empty())
  {
    std::set<IPos*, IMark::compare>::iterator it = imarks.begin();
    IMark * m = (IMark*)(*it);
    imarks.erase(it);
    delete m;
  }
}
//---------------------------------------------------------------------------
bool Format::operator==(const SHEdit::Format& f)
{
  bool s = this->foreground == f.foreground || (this->foreground && f.foreground && *(this->foreground) == *(f.foreground));
  bool b = this->background == f.background || (this->background && f.background && *(this->background) == *(f.background));
  return b && s && this->style == f.style;
}
//---------------------------------------------------------------------------
bool Format::operator!=(const SHEdit::Format& f)
{
  bool s = this->foreground == f.foreground || (this->foreground && f.foreground && *(this->foreground) == *(f.foreground));
  bool b = this->background == f.background || (this->background && f.background && *(this->background) == *(f.background));
  return !b || !s  || this->style != f.style;
}
//---------------------------------------------------------------------------
SHEdit::Format& Format::operator+=(const SHEdit::Format& f)
{
  if(f.foreground)
    this->foreground = f.foreground;
  if(f.background)
    this->background = f.background;
  this->style = this->style + f.style;   //I have no idea how the + is implemented... what I want is in fact to do "this->style | f.style" (as I suppose it is stored as binary mask)
  return *this;
}
//---------------------------------------------------------------------------
SHEdit::Format& Format::operator=(const SHEdit::Format& f)
{
    this->foreground = f.foreground;
    this->background = f.background;
    this->style = f.style;
  return *this;
}
//---------------------------------------------------------------------------
bool FontStyle::operator==(const SHEdit::FontStyle& f)
{
  bool s = this->foreground == f.foreground || (this->foreground && f.foreground && *(this->foreground) == *(f.foreground));
  bool b = this->background == f.background || (this->background && f.background && *(this->background) == *(f.background));
  return b && s && this->style == f.style;
}
//---------------------------------------------------------------------------
bool FontStyle::operator!=(const SHEdit::FontStyle& f)
{
  return !(*this == f);
}
//---------------------------------------------------------------------------
SHEdit::FontStyle& FontStyle::operator+=(const SHEdit::FontStyle& f)
{
  if(f.foreground)
    this->foreground = f.foreground;
  if(f.background)
    this->background = f.background;
  this->style = this->style + f.style;
  return *this;
}
//---------------------------------------------------------------------------
SHEdit::FontStyle& FontStyle::operator+=(const SHEdit::Format& f)
{
  if(f.foreground)
    this->foreground = f.foreground;
  if(f.background)
    this->background = f.background;
  this->style = this->style + f.style;
  return *this;
}
//---------------------------------------------------------------------------

SHEdit::FontStyle& FontStyle::operator=(const SHEdit::FontStyle& f)
{
    this->foreground = f.foreground;
    this->background = f.background;
  this->style = f.style;
  return *this;
}
//---------------------------------------------------------------------------
SHEdit::FontStyle& FontStyle::operator=(const SHEdit::Format& f)
{
    this->foreground = f.foreground;
    this->background = f.background;
  this->style = f.style;
  return *this;
}
//---------------------------------------------------------------------------
IMark * Format::GetMarkBefore(IPos* ipos)
{
  if(imarks.empty())
    return NULL;
  std::set<IPos*, IMark::compare>::iterator it = imarks.upper_bound(ipos);
  it--;
  return (IMark*)*it;
}
//---------------------------------------------------------------------------
