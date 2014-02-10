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
  this->jumpcount = 0;
  this->popmask = 0;
  for(int i = 0; i < 128; i++)
    map[i] = NULL;
}
//---------------------------------------------------------------------------
bool LanguageDefinition::SearchIt::operator==(const SearchIt& sit)
{
  return (this->base == sit.base && this->mask == sit.mask);
}

//---------------------------------------------------------------------------
bool LanguageDefinition::SearchIt::operator!=(const SearchIt& sit)
{
  return (this->base != sit.base || this->mask != sit.mask);
}

//---------------------------------------------------------------------------
LanguageDefinition::Jump::Jump(short _pushmask, short _newmask, TreeItem * _next)
  : pushmask(_pushmask), newmask(_newmask), nextTree(_next)
{
 }

//---------------------------------------------------------------------------
LanguageDefinition::Jump::Jump()
  : pushmask(0), newmask(0), nextTree(NULL)
{
 }

//---------------------------------------------------------------------------
void LanguageDefinition::TreeItem::AddJump(short pushmask, short newmask, TreeItem * to)
{
  Jump * newarray = new Jump[jumpcount+1];
  for(int i = 0; i < jumpcount; i++)
    newarray[i] = jumps[i];
  newarray[jumpcount] = Jump(pushmask, newmask, to);
  delete [] jumps;
  jumps = newarray;
  jumpcount++;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
LanguageDefinition::LanguageDefinition()
{
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

  AddJump(opening, format, LangDefSpecType::Jump, at, newTree);
  AddJump(closing, format, LangDefSpecType::Jump, newTree, at);

  return newTree;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeItem * LanguageDefinition::AddNewTree(FontStyle * format)
{
  TreeItem * newTree = new TreeItem('\0', LangDefSpecType::Empty, format);
  newTree->format = format;
  //newTree->id = idsUsed;
  //idsUsed++;
  //specItems.push_back(newTree);
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
  item->AddJump(0, 0, to);
  //item->id = to->id;
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddPush(wchar_t * string, FontStyle * format, TreeItem * at, TreeItem * to, short pushmask, short newmask)
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
  item->type = LangDefSpecType::PushPop;
  if(format != NULL)
    item->format = format;
  item->AddJump(pushmask, newmask, to);
  //item->id = to->id;
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddPop(wchar_t * string, FontStyle * format, TreeItem * at, short popmask)
{
  if(at == NULL)
    at = tree;
  wchar_t * ptr = string;
  TreeItem * item = at;
  while(*ptr != ' ' && *ptr != '\0')
  {
    item = FindOrCreateItem(item, *ptr, at);
    ptr++;
  }
  item->type = LangDefSpecType::PushPop;
  if(format != NULL)
    item->format = format;
  item->popmask = item->popmask | popmask;
  //item->id = to->id;
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
    item->map[(int)comp] = new TreeItem(comp, LangDefSpecType::Nomatch, at->format);
    return item->map[(int)comp];
  }
  else
  {
    item->items.push_back(new TreeItem(comp, LangDefSpecType::Nomatch, at->format));
    return item->items.back();
  }
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeItem* LanguageDefinition::GetTree()
{
  return tree;
}
//---------------------------------------------------------------------------
LangDefSpecType LanguageDefinition::Go(SearchIter& item, wchar_t c, bool & lookahead)
{
  wchar_t comp = towupper(c);
beginning:
  if((int)c < (int)0x7F)
  {
    if(item.current->map[(int)comp])
    {
      lookahead = false;
      item.current = item.current->map[(int)comp];
      return item.current->type;
    }
  }
  else
  {
    for (std::list<TreeItem*>::const_iterator itr = item.current->items.begin(); itr != item.current->items.end(); ++itr)
    {
      if((*itr)->thisItem == comp)
      {
        lookahead = false;
        item.current = *itr;
        return item.current->type;
      }
    }
  }
  item.current = item.base;
  if(lookahead)
  {
    //return item->type;
    if((int)c < (int)0x7F)
    {
      if(item.current->map[(int)comp])
      {
        item.current = item.current->map[(int)comp];
        //return item->type == LangDefSpecType::Nomatch ? LangDefSpecType::NoEmpty : item->type;
        lookahead = true;
        return item.current->type;
      }
    }
    else
    {
      for (std::list<TreeItem*>::const_iterator itr = item.current->items.begin(); itr != item.current->items.end(); ++itr)
      {
        if((*itr)->thisItem == comp)
        {
          item.current = *itr;
          //return item->type == LangDefSpecType::Nomatch ? LangDefSpecType::NoEmpty : item->type;
          lookahead = true;
          return item.current->type;
        }
      }
    }
  }
  return item.current->type;
}
//---------------------------------------------------------------------------
LanguageDefinition::SearchIter LanguageDefinition::GetDefSC()
{
  SearchIter st;
  st.current = GetTree();
  st.base = GetTree();
  //mask = 0;
  return st;
}
//---------------------------------------------------------------------------
/*
LanguageDefinition::TreeItem * LanguageDefinition::GetSpecItem(short id)
{

  for (std::list<TreeItem*>::const_iterator itr = specItems.begin(); itr != specItems.end(); ++itr)
  {
    if((*itr)->id == id)
      return *itr;
  }
  return this->tree;
}                                   */
//---------------------------------------------------------------------------
LanguageDefinition::~LanguageDefinition()
{
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
