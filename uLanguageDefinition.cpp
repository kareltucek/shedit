//---------------------------------------------------------------------------
#pragma hdrstop


#include "uLanguageDefinition.h"
#include "uFormat.h"
#include <string>
#include <stdio.h>

using namespace SHEdit;

//---------------------------------------------------------------------------
#pragma package(smart_init)

LanguageDefinition::TreeNode::TreeNode(wchar_t c, LangDefSpecType type, SHEdit::FontStyle * format, bool caseSensitive)
{
  this->thisItem = c;
  this->type = type;
  this->format = format;
  this->jumpcount = 0;
  this->jumps = NULL;
  this->popmask = 0;
  this->bankID = 0;
  this->caseSensitive = caseSensitive;
  for(int i = 0; i < 128; i++)
    map[i] = NULL;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeNode::TreeNode(TreeNode * tree, SHEdit::FontStyle * format)
{
  this->thisItem = tree->thisItem;
  this->type = tree->type;
  this->format = format;
  this->jumpcount = tree->jumpcount;
  this->popmask = tree->popmask;
  this->jumps = new Jump[jumpcount];
  this->bankID = tree->bankID;
  this->caseSensitive = tree->caseSensitive;
  for(int i = 0; i < jumpcount; i++)
    this->jumps[i] = tree->jumps[i];
  for(int i = 0; i < 128; i++)
    map[i] = tree->map[i];
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
  LanguageDefinition::Jump::Jump(short _pushmask, short _newmask, short _freemask, TreeNode * _next)
: pushmask(_pushmask), newmask(_newmask), freemask(_freemask), nextTree(_next)
{
}

//---------------------------------------------------------------------------
  LanguageDefinition::Jump::Jump()
: pushmask(0), newmask(0), freemask(0), nextTree(NULL)
{
}

//---------------------------------------------------------------------------
  LanguageDefinition::SearchIt::SearchIt()
: current(NULL), base(NULL), mask(0)
{
}
//---------------------------------------------------------------------------
void LanguageDefinition::TreeNode::AddJump(short pushmask, short newmask, short freemask, TreeNode * to, bool begin)
{
  Jump * newarray = new Jump[jumpcount+1];
  for(int i = 0; i < jumpcount; i++)
    newarray[i+(begin ? 1 : 0)] = jumps[i];
  newarray[begin ? 0 : jumpcount] = Jump(pushmask, newmask, freemask, to);
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
  caseSensitive = false;

  tree = AddNewTree(defFormat);
  tree->format = defFormat;

  allowWhiteSkipping = true;
  bankIdsNum = 1;
  bankMasks = new short[1];
  bankMasks[0] = ~((short)0)-1;
  bankBases = new TreeNode*[1];
  bankBases[0] = tree;
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
  return allowWhiteSkipping && (int)c < 33 && c != '\n';
}
//---------------------------------------------------------------------------
void LanguageDefinition::SetDefaultColor(TColor * defColor)
{
  this->defFormat->foreground = defColor;
}
//---------------------------------------------------------------------------
//at may be NULL
void LanguageDefinition::AddKeywords(wchar_t * string, FontStyle * format, TreeNode * at)
{
  if(at == NULL)
    at = tree;
  FontStyle * newformat = format;
  //FontStyle * newformat = new FontStyle();
  //*newformat = *(at->format);
  //*newformat = *format;
  wchar_t * ptrend = string + wcslen(string); 
  while(string != ptrend)
  {
    while(*string == ' ')
      string++;
    wchar_t * ptr = string;
    TreeNode * item = at;
    if(*ptr == '\0')  //else would change the tree base instead
      return;
    while(*ptr != ' ' && *ptr != '\0')
    {
      item = FindOrCreateItem(item, *ptr, at);
      ptr++;
    }
    item->type = LangDefSpecType::Normal;
    if(newformat != NULL)
      item->format = newformat;
    if(newformat == NULL && item->format == NULL && at->format != NULL)
      item->format = at->format;
    string = ptr;
  }
}
//---------------------------------------------------------------------------
void LanguageDefinition::SetCaseSensitive(bool casesensitive)
{
  this->caseSensitive = casesensitive;
}
//---------------------------------------------------------------------------
void LanguageDefinition::SetTreeCaseSensitive(TreeNode * item, bool casesensitive)
{
  item->caseSensitive = casesensitive;
}
//---------------------------------------------------------------------------
void LanguageDefinition::SetAllowWhiteSkipping(bool allow)
{
  this->allowWhiteSkipping = allow;
}
//---------------------------------------------------------------------------

LanguageDefinition::TreeNode* LanguageDefinition::AddLineStrong(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to)
{
  if(at == NULL)
    at = tree;
  if(to == NULL)
    to = AddNewTree(format);

  AddJump(string, format, LangDefSpecType::LineTag, at, to);
  return to;
}
//---------------------------------------------------------------------------

LanguageDefinition::TreeNode* LanguageDefinition::AddLine(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to)
{
  if(at == NULL)
    at = tree;
  if(to == NULL)
    to = AddNewTree(format);

  return AddPair(string, L"\n", format, at, to);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddWord(wchar_t * string, FontStyle * format, TreeNode * at)
{
  wchar_t * ptr = string;
  if(at == NULL)
    at = tree;
  TreeNode * item = at;
  while(*ptr != ' ' && *ptr != '\0')
  {
    item = FindOrCreateItem(item, *ptr, at);
    ptr++;
  }
  item->type = LangDefSpecType::WordTag;
  if(format != NULL)
    item->format = format;
  if(format == NULL && item->format == NULL && at->format != NULL)
    item->format = at->format;
  //item->format = new FontStyle();
  //*(item->format) = *(at->format);
  //*(item->format) = *format;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeNode* LanguageDefinition::AddPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeNode * at, TreeNode * to)
{
  if(at == NULL)
    at = tree;
  if(to == NULL)
    to = AddNewTree(format);

  AddJump(opening, format, LangDefSpecType::Jump, at, to);
  AddJump(closing, format, LangDefSpecType::Jump, to, at);

  return to;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeNode* LanguageDefinition::AddPushPopPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeNode * at, TreeNode * to, short mask)
{
  if(at == NULL)
    at = tree;
  if(to == NULL)
    to = AddNewTree(format);

  AddPush(opening, format, at, to, PUSH_ALWAYS, mask);
  AddPop(closing, format, to, mask, POP_AUTO);

  return to;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeNode * LanguageDefinition::AddNewTree(FontStyle * format)
{
  return AddNewTree(format, this->caseSensitive);
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeNode * LanguageDefinition::AddNewTree(FontStyle * format, bool caseSensit)
{
  TreeNode * newTree = new TreeNode('\0', LangDefSpecType::Empty, format, caseSensit);
  newTree->format = format;
  //newTree->id = idsUsed;
  //idsUsed++;
  //specItems.push_back(newTree);
  return newTree;
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeNode * LanguageDefinition::AddDupTree(LanguageDefinition::TreeNode * tree, FontStyle * format)
{
  TreeNode * newTree = new TreeNode(tree, format == NULL ? tree->format : format);
  return newTree;
}
//---------------------------------------------------------------------------
void LanguageDefinition::_AddJump(bool begin, wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask, short newmask, short freemask)
{
  if(at == NULL)
    at = tree;
  if(to == NULL)
    to = tree;
  wchar_t * ptr = string;

  while(*ptr != '\0')
  {
    while(*ptr == ' ')
      ptr++;
    if(*ptr == '\0')
      break;
    
      TreeNode * item = at;
    while(*ptr != ' ' && *ptr != '\0')
    {
      item = FindOrCreateItem(item, *ptr, at);
      ptr++;
    }
    item->type = type;
    if(format != NULL)
      item->format = format;
    item->AddJump(jumpmask, newmask, freemask, to, begin);
    //item->id = to->id;
  }
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddJump(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask, short newmask, short freemask)
{
  _AddJump(false, string, format, type, at, to, jumpmask, newmask, freemask);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddJumps(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask, short newmask, short freemask)
{
  _AddJump(false, string, format, type, at, to, jumpmask, newmask, freemask);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddJumpFront(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask, short newmask, short freemask)
{
  _AddJump(true, string, format, type, at, to, jumpmask, newmask, freemask);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddJumpsFront(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask, short newmask, short freemask)
{
  _AddJump(true, string, format, type, at, to, jumpmask, newmask, freemask);
}
//---------------------------------------------------------------------------
void LanguageDefinition::_AddPush(bool begin, wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask)
{
  if(at == NULL)
    at = tree;
  if(to == NULL)
    to = tree;
  wchar_t * ptr = string;
  while(*ptr != '\0')
  {
    while(*ptr == ' ')
      ptr++;
    if(*ptr == '\0')
      break;


    TreeNode * item = at;
    while(*ptr != ' ' && *ptr != '\0')
    {
      item = FindOrCreateItem(item, *ptr, at);
      ptr++;
    }
    item->type = LangDefSpecType::PushPop;
    if(format != NULL)
      item->format = format;
    item->AddJump(pushmask, newmask, 0, to, begin);
    //item->id = to->id;
  }
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddPush(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask)
{
  _AddPush(false, string, format, at, to, pushmask, newmask);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddPushFront(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask)
{
  _AddPush(true, string, format, at, to, pushmask, newmask);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddPushes(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask)
{
  _AddPush(false, string, format, at, to, pushmask, newmask);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddPushesFront(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask)
{
  _AddPush(true, string, format, at, to, pushmask, newmask);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddPops(wchar_t * string, FontStyle * format, TreeNode * at, short popmask, short popcount)
{
  AddPop(string, format, at, popmask, popcount);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddPop(wchar_t * string, FontStyle * format, TreeNode * at, short popmask, short popcount)
{
  if(at == NULL)
    at = tree;
  wchar_t * ptr = string;
  while(*ptr != '\0')
  {
    while(*ptr == ' ')
      ptr++;
    if(*ptr == '\0')
      break;

    TreeNode * item = at;
    while(*ptr != ' ' && *ptr != '\0')
    {
      item = FindOrCreateItem(item, *ptr, at);
      ptr++;
    }
    if(item->type != LangDefSpecType::Jump)
      item->type = LangDefSpecType::PushPop;
    if(format != NULL)
      item->format = format;
    item->popmask = item->popmask | popmask;
    if(popcount >= -1)
      item->popcount = popcount;
    //item->id = to->id;
  }
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeNode* LanguageDefinition::FindOrCreateItem(TreeNode * item, wchar_t comp, TreeNode * at)
{
  if(!at->caseSensitive)
    comp = towupper(comp);
  //find
  if((int)comp < (int)0xEF && item->map[(int)comp] != NULL)
    return item->map[(int)comp];
  for (std::list<TreeNode*>::const_iterator itr = item->items.begin(); itr != item->items.end(); ++itr)
  {
    if((*itr)->thisItem == comp)
      return *itr;
  }
  //create
  if((int)comp < (int)0xEF)
  {
    item->map[(int)comp] = new TreeNode(comp, LangDefSpecType::Nomatch, at->format, at->caseSensitive);
    return item->map[(int)comp];
  }
  else
  {
    item->items.push_back(new TreeNode(comp, LangDefSpecType::Nomatch, at->format, at->caseSensitive));
    return item->items.back();
  }
}
//---------------------------------------------------------------------------
LanguageDefinition::TreeNode* LanguageDefinition::GetTree()
{
  return tree;
}
//---------------------------------------------------------------------------
LangDefSpecType LanguageDefinition::Go(SearchIter * item, wchar_t c, bool & lookahead)
{
  if(!item->base->caseSensitive)
    c = towupper(c);
beginning:
  if((int)c < (int)0x7F)
  {
    if(item->current->map[(int)c])
    {
      lookahead = false;
      item->current = item->current->map[(int)c];
      return item->current->type;
    }
  }
  else
  {
    for (std::list<TreeNode*>::const_iterator itr = item->current->items.begin(); itr != item->current->items.end(); ++itr)
    {
      if((*itr)->thisItem == c)
      {
        lookahead = false;
        item->current = *itr;
        return item->current->type;
      }
    }
  }
  item->current = item->base;
  if(lookahead)
  {
    //return item->type;
    if((int)c < (int)0x7F)
    {
      if(item->current->map[(int)c])
      {
        item->current = item->current->map[(int)c];
        //return item->type == LangDefSpecType::Nomatch ? LangDefSpecType::NoEmpty : item->type;
        lookahead = true;
        return item->current->type;
      }
    }
    else
    {
      for (std::list<TreeNode*>::const_iterator itr = item->current->items.begin(); itr != item->current->items.end(); ++itr)
      {
        if((*itr)->thisItem == c)
        {
          item->current = *itr;
          //return item->type == LangDefSpecType::Nomatch ? LangDefSpecType::NoEmpty : item->type;
          lookahead = true;
          return item->current->type;
        }
      }
    }
  }
  return item->current->type;
}
//---------------------------------------------------------------------------
LanguageDefinition::SearchIter LanguageDefinition::GetDefSC(short id)
{
  SearchIter st;
  st.current = bankBases[id];
  st.base = bankBases[id];
  //mask = 0;
  return st;
}
//---------------------------------------------------------------------------
short LanguageDefinition::GetBankIdCount()
{
  return  bankIdsNum;
}
//---------------------------------------------------------------------------
short LanguageDefinition::GetNewBankID()
{
  short * newarray = new short[bankIdsNum+1];
  for(int i = 0; i < bankIdsNum; i++)
    newarray[i] = bankMasks[i];
  bankMasks[bankIdsNum] = ~((short)0);
  delete [] bankMasks;
  bankMasks = newarray;

  TreeNode ** newtarray = new TreeNode*[bankIdsNum+1];
  for(int i = 0; i < bankIdsNum; i++)
    newtarray[i] = bankBases[i];
  bankBases[bankIdsNum] = NULL;
  delete [] bankBases;
  bankBases = newtarray;

  bankIdsNum++;
  return bankIdsNum-1;
}
//---------------------------------------------------------------------------
void LanguageDefinition::SetBankMask(int bank, short mask)
{
  bankMasks[bank] = mask;
}
//---------------------------------------------------------------------------
short LanguageDefinition::GetBankMask(int bank)
{
  return bankMasks[bank];
}
//---------------------------------------------------------------------------
void LanguageDefinition::SetBankBase(int bank, TreeNode * item)
{
    bankBases[bank] = item;
}
//---------------------------------------------------------------------------
short LanguageDefinition::GetBankIdByMask(int mask)
{
  for(int i = 0; i < bankIdsNum; i++)
    if(bankMasks[i] & mask)
      return i;
  return 0;
}
//---------------------------------------------------------------------------
/*
   LanguageDefinition::TreeNode * LanguageDefinition::GetSpecItem(short id)
   {

   for (std::list<TreeNode*>::const_iterator itr = specItems.begin(); itr != specItems.end(); ++itr)
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
