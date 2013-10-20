//---------------------------------------------------------------------------
#pragma hdrstop


#include "uLanguageDefinition.h"
#include "uFormat.h"
#include <string>
#include <stdio.h>

using namespace SHEdit;

//---------------------------------------------------------------------------
#pragma package(smart_init)

LanguageDefinition::TreeItem::TreeItem(wchar_t c, LangDefSpecType type, SHEdit::FontStyle * format)
{
  this->thisItem = c;
  this->type = type;
  this->format = format;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall LanguageDefinition::LanguageDefinition()
{
  masksUsed = 1;
  defFormat = new Format();
  defFormat->foreground = new TColor(clBlack);
  defFormat->background = new TColor(clWhite);
  tree = new TreeItem('\0', LangDefSpecType::Empty, defFormat);
  tree->format = defFormat;
}
//---------------------------------------------------------------------------
bool LanguageDefinition::IsAl(wchar_t c)
{
  return isalpha(c) || c == '_' || (int)c > 127;
}
//---------------------------------------------------------------------------
bool LanguageDefinition::IsAlNum(wchar_t c)
{
  return isalnum(c) || c == '_' || (int)c > 127;
}
//---------------------------------------------------------------------------
bool LanguageDefinition::IsNum(wchar_t c)
{
  return isdigit(c);
}
//---------------------------------------------------------------------------
bool LanguageDefinition::IsWhite(wchar_t c)
{
  return (int)c < 33 && c != '\n';
}
//---------------------------------------------------------------------------
void LanguageDefinition::SetDefaultColor(TColor * defColor)
{
  this->defFormat->foreground = defColor;
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddReservedNames(wchar_t * string, TColor * color)
{
  FontStyle * newformat = new FontStyle(color, defFormat->background);
  wchar_t * ptrend = string + wcslen(string); 
  while(string != ptrend)
  {
    while(*string == ' ')
      string++;
    wchar_t * ptr = string;
    TreeItem * item = tree;
    while(*ptr != ' ' && *ptr != '\0')
    {
      item = FindOrCreateItem(item, *ptr);
      ptr++;
    }
    item->type = LangDefSpecType::Normal;
    item->format = newformat;
    string = ptr;
  }
}
//---------------------------------------------------------------------------
void LanguageDefinition::SetCaseSensitive(bool casesensitive)
{
  this->caseSensitive = caseSensitive;
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddSpecial(wchar_t * opening, wchar_t * closing, LangDefSpecType type, TColor * color)
{
  wchar_t * ptr = opening;
  TreeItem * item = tree;
  while(*ptr != ' ' && *ptr != '\0')
  {
    item = FindOrCreateItem(item, *ptr);
    ptr++;
  }
  item->type = type;
  item->pair = closing;
  item->format->foreground;
  masksUsed++;
  item->mask = 1 << masksUsed;
  specItems.push_back(item);
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeItem* LanguageDefinition::FindOrCreateItem(TreeItem * item, wchar_t c)
{
  wchar_t comp = towupper(c);
  for (std::list<TreeItem*>::const_iterator itr = item->items.begin(); itr != item->items.end(); ++itr)
  {
    if((*itr)->thisItem == comp)
      return *itr;
  }
  item->items.push_back(new TreeItem(comp, LangDefSpecType::Nomatch, defFormat));
  return item->items.back();
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeItem* LanguageDefinition::GetTree()
{
  return tree;
}
//---------------------------------------------------------------------------
LangDefSpecType LanguageDefinition::Go(TreeItem *& item, wchar_t c)
{
  wchar_t comp = towupper(c);
  for (std::list<TreeItem*>::const_iterator itr = item->items.begin(); itr != item->items.end(); ++itr)
  {
    if((*itr)->thisItem == comp)
    {
      item = *itr;
      return item->type;
    }
  }
  item = tree;
  return tree->type;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeItem * LanguageDefinition::GetSpecItem(short mask)
{

  for (std::list<TreeItem*>::const_iterator itr = specItems.begin(); itr != specItems.end(); ++itr)
  {
    if((*itr)->mask == mask)
      return *itr;
  }
  return this->tree;
}
//---------------------------------------------------------------------------
LanguageDefinition::~LanguageDefinition()
{
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
