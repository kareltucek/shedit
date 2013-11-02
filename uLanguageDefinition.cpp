//---------------------------------------------------------------------------
#pragma hdrstop


#include "uLanguageDefinition.h"
#include "uFormat.h"
#include <string>
#include <stdio.h>

using namespace SHEdit;

//---------------------------------------------------------------------------
#pragma package(smart_init)

LanguageDefinition::TreeItem::TreeItem(wchar_t c, LangDefSpecType type, SHEdit::FontStyle * format, SHEdit::LanguageDefinition::TreeItem * at)
{
  this->thisItem = c;
  this->type = type;
  this->format = format;
  this->nextTree = at == NULL ? this : at;
  for(int i = 0; i < 128; i++)
    map[i] = NULL;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall LanguageDefinition::LanguageDefinition()
{
  masksUsed = 0;
  defFormat = new Format();
  defFormat->foreground = new TColor(clBlack);
  defFormat->background = new TColor(clWhite);
  tree = AddNewTree(defFormat);
  tree->format = defFormat;
}
//---------------------------------------------------------------------------
bool LanguageDefinition::IsAl(wchar_t c)
{
  return iswalpha(c) || c == '_';
}
//---------------------------------------------------------------------------
bool LanguageDefinition::IsAlNum(wchar_t c)
{
  return iswalnum(c) || c == '_';
}
//---------------------------------------------------------------------------
bool LanguageDefinition::IsNum(wchar_t c)
{
  return iswdigit(c);
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
//at may be NULL
void LanguageDefinition::AddReservedNames(wchar_t * string, FontStyle * format, TreeItem * at)
{
  if(at == NULL)
    at = tree;
  FontStyle * newformat = new FontStyle();
  *newformat = *(at->format);
  *newformat = *format;
  wchar_t * ptrend = string + wcslen(string); 
  while(string != ptrend)
  {
    while(*string == ' ')
      string++;
    wchar_t * ptr = string;
    TreeItem * item = at;
    while(*ptr != ' ' && *ptr != '\0')
    {
      item = FindOrCreateItem(item, *ptr, at);
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

LanguageDefinition::TreeItem* LanguageDefinition::AddLine(wchar_t * string, FontStyle * format, TreeItem * at, TreeItem * to)
{
  if(at == NULL)
    at = tree;
  if(to == NULL)
    to = AddNewTree(format);

  AddJump(string, format, LangDefSpecType::LineTag, at, to);
  return to;
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddWord(wchar_t * string, FontStyle * format, TreeItem * at)
{
  wchar_t * ptr = string;
  if(at == NULL)
    at = tree;
  TreeItem * item = at;
  while(*ptr != ' ' && *ptr != '\0')
  {
    item = FindOrCreateItem(item, *ptr, at);
    ptr++;
  }
  item->type = LangDefSpecType::WordTag;
  item->format = new FontStyle();
  *(item->format) = *(at->format);
  *(item->format) = *format;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeItem* LanguageDefinition::AddPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeItem * at)
{
  if(at == NULL)
    at = tree;
  TreeItem * newTree = AddNewTree(format);

  AddJump(opening, format, LangDefSpecType::PairTag, at, newTree);
  AddJump(closing, format, LangDefSpecType::PairTag, newTree, at);

  return newTree;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeItem * LanguageDefinition::AddNewTree(FontStyle * format)
{
  TreeItem * newTree = new TreeItem('\0', LangDefSpecType::Empty, format, NULL);
  newTree->format = format;
  newTree->mask = masksUsed;
  masksUsed++;
  specItems.push_back(newTree);
  return newTree;
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddJump(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeItem * at, TreeItem * to)
{
  if(at == NULL)
    at = tree;
  if(to == NULL)
    to = tree;
  wchar_t * ptr = string;
  TreeItem * item = at;
  while(*ptr != ' ' && *ptr != '\0')
  {
    item = FindOrCreateItem(item, *ptr, at);
    ptr++;
  }
  item->type = type;
  item->format = format;
  item->nextTree = to;
  item->mask = to->mask;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeItem* LanguageDefinition::FindOrCreateItem(TreeItem * item, wchar_t c, TreeItem * at)
{
  wchar_t comp = towupper(c);
  if((int)comp < (int)0xEF && item->map[(int)comp] != NULL)
    return item->map[(int)comp];
  for (std::list<TreeItem*>::const_iterator itr = item->items.begin(); itr != item->items.end(); ++itr)
  {
    if((*itr)->thisItem == comp)
      return *itr;
  }
  if((int)comp < (int)0xEF)
  {
    item->map[(int)comp] = new TreeItem(comp, LangDefSpecType::Nomatch, defFormat, at);
    return item->map[(int)comp];
  }
  else
  {
    item->items.push_back(new TreeItem(comp, LangDefSpecType::Nomatch, defFormat, at));
    return item->items.back();
  }
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
beginning:
  if((int)c < (int)0xEF)
  {
    if(item->map[(int)comp])
    {
      item = item->map[(int)comp];
      return item->type;
    }
  }
  else
  {
    for (std::list<TreeItem*>::const_iterator itr = item->items.begin(); itr != item->items.end(); ++itr)
    {
      if((*itr)->thisItem == comp)
      {
        item = *itr;
        return item->type;
      }
    }
  }
  if(item != item->nextTree)
  {
    item = item->nextTree;
    if((int)c < (int)0xEF)
    {
      if(item->map[(int)comp])
      {
        item = item->map[(int)comp];
        return item->type == LangDefSpecType::Nomatch ? LangDefSpecType::NoEmpty : item->type;
      }
    }
    else
    {
      for (std::list<TreeItem*>::const_iterator itr = item->items.begin(); itr != item->items.end(); ++itr)
      {
        if((*itr)->thisItem == comp)
        {
          item = *itr;
          return item->type == LangDefSpecType::Nomatch ? LangDefSpecType::NoEmpty : item->type;
        }
      }
    }
  }
  return item->type;
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
