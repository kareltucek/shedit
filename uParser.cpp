//---------------------------------------------------------------------------
#pragma hdrstop


#include "uParser.h"
#include "uDrawer.h"
#include "uLanguageDefinition.h"
#include "cSHEdit.h"
#include "uSpan.h"
#include "uIter.h"
#include "uMark.h"
#include "uBuffer.h"
#include "uFormat.h"
#include "uStack.h"
#include <string>
#include <stdio.h>
#include <time.h>
#include <list>
#include <windows.h>

using namespace SHEdit;

#ifdef DEBUG
#include <fstream>
std::ofstream myfile;
#endif

#pragma package(smart_init)

#define searchStateStack searchStateStack

//---------------------------------------------------------------------------
  Parser::ParserState::ParserState()
: markupStack(), searchStateStack()
{
  //stateid = 0;
  globalMask = 0;
}
//---------------------------------------------------------------------------
Parser::ParserState::~ParserState()
{
  searchStateStack.Erase();
  markupStack.Erase();
}
//---------------------------------------------------------------------------
void Parser::ParserState::InitBanks(int count)
{

}
//---------------------------------------------------------------------------
SHEdit::Parser::ParserState& Parser::ParserState::operator=(const SHEdit::Parser::ParserState& p)
{
  parseid = p.parseid;
  markupStack = p.markupStack;
  globalMask = p.globalMask;

  searchStateStack = p.searchStateStack;

  return *this;
}
//---------------------------------------------------------------------------
bool Parser::ParserState::operator==(const ParserState& state)
{
    if(!(searchStateStack == state.searchStateStack))
      return false;
  return (this->parseid == state.parseid && globalMask == state.globalMask && markupStack == state.markupStack);
}
//---------------------------------------------------------------------------
bool Parser::ParserState::operator!=(const ParserState& state)
{
    if(!(searchStateStack == state.searchStateStack))
      return true;
  return !(this->parseid == state.parseid && globalMask == state.globalMask && markupStack == state.markupStack);
}
//---------------------------------------------------------------------------
Parser::ParseTask::ParseTask(NSpan * l, int ln)
{
  line = l;
  linenum = ln;
}
//---------------------------------------------------------------------------
Parser::ParseTask::ParseTask()
{
}
//---------------------------------------------------------------------------
bool Parser::ParseTask::operator==(const Parser::ParseTask & pt) const
{
  return this->line == pt.line;
}
//---------------------------------------------------------------------------
bool Parser::ParseTask::operator<(const Parser::ParseTask & pt)  const
{
  return this->linenum < pt.linenum;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall Parser::Parser(TSHEdit * parent, Drawer * drawer)
{
#ifdef DEBUG
  myfile.open("parser.txt", ios::out);
  dbgLogging = false;
#endif
  this->parent = parent;
  this->drawer = drawer;

  recurse = 0;
  upperbound = PARSEINADVANCE;
  onidleset = false;

  //DuplicateHandle(GetCurrentProcess(), bufferChanged, this->Handle, &(this->bufferChanged),  0, false, DUPLICATE_SAME_ACCESS);
}
//---------------------------------------------------------------------------
bool __fastcall Parser::Execute()
{
  //parse lists
  tasklist.sort();
  tasklistprior.sort();
  upperbound = parent->GetActLine() + PARSEINADVANCE;

  bool painted = false;

  int parsed = 0;
  bool endValid = false;
  while(tasklistprior.size() > 0 || (tasklist.size() > 0 && (tasklist.front().linenum < upperbound || tasklist.front().line->parserState.parseid != currentparseid)) )
  {
    ParseTask pt;
    if(tasklistprior.size() > 0)
    {
      pt = tasklistprior.front();
    }
    else
    {
      pt = tasklist.front();
    }
    linenum = parent->GetLineNum(pt.line);
    bool first = true;
    painted = linenum >= 0 | painted;
    Iter * itr = new Iter(pt.line, linenum >= 0 ? parent->itrLine.linenum+linenum : pt.linenum, 0, parent->buffer);
    state = itr->line->parserState;

    if(itr->linenum == 1)
      state.parseid = currentparseid;
    else if(itr->line != NULL && itr->line->prevline != NULL)
      state.parseid = itr->line->prevline->parserState.parseid;

    if(linenum >= 0)
    {
      ReconstructMarkup();
      actIMarkup = itr->ReconstructIMarkFontStyle();
      actMarkupCombined = actMarkup;
      actMarkupCombined += actIMarkup;
      itr->UpdateNextImark();
    }

    if(state.searchStateStack.top == NULL)
      state.searchStateStack.Push(langdef->GetDefSC(0));
    //LanguageDefinition::SearchIter * searchiter = &state.searchStateStack.top->data;  //now passed directly - no need to keep it
    actFormat = *state.searchStateStack.top->data.base->format;

    tasklistprior.remove(pt);
    tasklist.remove(pt);



    while(itr->word->next && (first || itr->line->parserState != this->state || tasklistprior.front().line == itr->line))
    {
      //take care of record from list
      newline = true;
      first = false;
      //take care of line
      if(itr->line->prevline != NULL) //otherwise sets a format to the first line -> changing langdef will never iterate change in buffer
        itr->line->parserState = this->state;

      tasklistprior.remove(pt);
      tasklist.remove(pt);

      // parent->Log(String("parsing line ")+String(itr->linenum));
      ParseLine(itr, &state.searchStateStack.top->data, linenum >= 0);

      endValid = itr->GoChar();
      if(linenum >= 0)
      {
        //FlushAll();
        painted = true;
        linenum = parent->GetLineNum(itr->line);
      }
      else if(parent->IsLineFirstVisible(itr->line))
      {
        itr->linenum = parent->GetActLine();
        ReconstructMarkup();
        actIMarkup = itr->ReconstructIMarkFontStyle();
        actMarkupCombined = actMarkup;
        actMarkupCombined += actIMarkup;
        itr->UpdateNextImark();

        linenum = 0;
      }
      pt.linenum++;
      pt.line = itr->line;
      parsed++;

      //if(!(linenum >= 0 && linenum <= parent->GetVisLineCount() && itr->line->parserState != this->state) && tasklistprior.size() > 0 )
      if(linenum < 0 && (tasklistprior.size() > 0 || itr->linenum > upperbound || parsed > PARSEINONEGO))
      {
        if(itr->word->next != NULL && itr->line->parserState != this->state)
        {
          this->tasklist.push_front(ParseTask(itr->line, itr->linenum));
        }
        itr->line->parserState = this->state;
        break;
      }
    }
    if(!itr->word->next && linenum >= 0)
    {
      if(first)  //means line has not been either parsed or flushed, because iter was already on nextline (empty line) //well I no longer understand purpose of this, but let's consider, that the tail of buffer has to be explicitly flushed to the drawer
        FlushAll();
      SendEof();
    }

    if(itr->word->next == NULL && endValid) //when last line is empty line, then parseline would not get assigned
      itr->line->parserState = this->state;  //beware - you do not want to assign state into wrong line (when iter could go no further

    //parent->Log(String("parsed to line ")+String(itr->linenum));
    delete itr;

    if((painted || parsed > PARSEINONEGO) && tasklistprior.size() == 0)
    {
      if(painted)
      {
        drawer->Paint();
      }
      if(parsed > PARSEINONEGO)
      {
        if(!onidleset)
        {
          oldidle = Application->OnIdle;
          Application->OnIdle = OnIdle;
          onidleset = true;
        }
        return false;
      }
      parsed = 0;
    }
  }

  return true;
}
//---------------------------------------------------------------------------
void __fastcall Parser::OnIdle(TObject * Sender, bool & Done)
{
  if(Execute())
  {
    Application->OnIdle = oldidle;
    onidleset = false;
  }
  Done = false;
}
//---------------------------------------------------------------------------
void Parser::ParseFromLine(NSpan * line, int linenum, int prior)
{
  if(prior > 0)
    tasklistprior.push_back(ParseTask(line, linenum));
  else
    tasklist.push_back(ParseTask(line, linenum));
}

//---------------------------------------------------------------------------
void Parser::ParseFromToLine(NSpan * line, int linenum, int count, int prior)
{
  for(NSpan * l = line; l != NULL && count > 0; l = l->nextline, linenum++, count--)
  {
    if(prior > 0)
      tasklistprior.push_back(ParseTask(l, linenum));
    else
      tasklist.push_back(ParseTask(l, linenum));
  }
}

//---------------------------------------------------------------------------
/*!
 * how did this get here?
 */
void __fastcall Parser::Draw()
{
  drawer->Paint();
}
//---------------------------------------------------------------------------
void Parser::ParseLine(Iter * itr, LanguageDefinition::SearchIter * searchiter, bool paint)
{
#ifdef DEBUG
  //Write(" entering parseline");
#endif
  wchar_t * ptr;
  ParserState lineback;
  bool linetag = false;
  int pos = 0;
  bool lookahead;
  LangDefSpecType type, lasttype;
  wchar_t *& pt = (itr->ptr); //debug
  this->inword = false;
  bool& inword = this->inword; //debug

  while(true)
  {
    CheckMarkup(itr, paint);
    bool whiteskipped = false;
    while(langdef->IsWhite(*(itr->ptr)) && *(itr->ptr) != '\n')
    {
      whiteskipped = true;
      AddChar(itr, pos);
      itr->GoChar();
      CheckMarkup(itr, paint);
    }
    if((whiteskipped && inword && langdef->IsAl(*(itr->ptr))) || type != LangDefSpecType::Nomatch)
    {
      searchiter->current = searchiter->base;
      Flush();
      //resetbase & flush
    }
    if(whiteskipped)
      inword = false;

    //lasttoken = searchiter->nextTree;
    /*
       bool lookahead = true;
       int type;
       if(langdef->IsAl(*(itr->ptr)))
       {
       inword = true;
       type = langdef->Go(searchiter, *(itr->ptr), lookahead);
       if(lookahead && paint)
       Flush();
       lookahead = false;
       while(langdef->IsAlNum(*(itr->ptr)) && (type == LangDefSpecType::Nomatch || (type != LangDefSpecType::Empty && langdef->IsAlNum(itr->GetNextChar()))))
       {
       if(paint)
       AddChar(itr, pos);
       itr->GoChar();
       CheckMarkup(itr,paint);
       type = langdef->Go(searchiter, *(itr->ptr), lookahead);
       }
       if(!langdef->IsAlNum(*(itr->ptr)))  //the search was ended past the word
       {
       if(type != LangDefSpecType::Nomatch && type != LangDefSpecType::Empty && CanGoFurther(*searchiter, itr, true, true))
       {
       while(langdef->IsWhite(*(itr->ptr)) && *(itr->ptr) != '\n')
       {
       inword = false;
       AddChar(itr, pos);
       itr->GoChar();
       CheckMarkup(itr, paint);
       }
       type = LangDefSpecType::Nomatch;
       } /*                             //ths would cut the ended word
       else
       {
       searchiter->current = searchiter->base;
       if(paint)
       actFormat += *(searchiter->current->format);
       Flush();
       type = langdef->Go(searchiter, *(itr->ptr), lookahead);
       if(*(itr->ptr) == '\n')
       break;
       }     */   /*
                     }
                     else if( langdef->IsAlNum(itr->GetNextChar()))  //the search was ended in the middle of word
                     {
                     type = LangDefSpecType::WordTag;
                     searchiter->current = searchiter->base;
                     }
    //else everything is gonna work  //ehm  //the search was ended on the last letter of word
    }
    else
    {
    inword = false;
    type = langdef->Go(searchiter, *(itr->ptr), lookahead);
    if(type != LangDefSpecType::Empty && type != LangDefSpecType::Nomatch && *CanGoFurther(searchiter, itr, false))
    type = LangDefSpecType::Nomatch;
    } */

    lasttype = type;
    type = langdef->Go(searchiter, *(itr->ptr), lookahead = !inword || !langdef->IsAlNum(*(itr->ptr)));

    inword = langdef->IsAl(*(itr->ptr)) || (inword && langdef->IsAlNum(*(itr->ptr)));

    if(type != LangDefSpecType::Empty && type != LangDefSpecType::Nomatch && CanGoFurther(*searchiter, itr, inword, false))
      type = LangDefSpecType::Nomatch;

    if(type != LangDefSpecType::Nomatch && inword && langdef->IsAlNum(itr->GetNextChar()))
    {
      type = LangDefSpecType::WordTag;
      searchiter->current = searchiter->base;
    }

#ifdef DEBUG
    /* int dbgtype = type; if( dbgLogging )  parent->Log( String(*(itr->ptr))+String(" ")+String(type)+String(" ")+String((int)lookahead));        //Write(String(*(itr->ptr))+String(" ")+String(type)+String(" ")+String(searchiter->type));  */
#endif
    if(lookahead || (lasttype == LangDefSpecType::Empty && type != LangDefSpecType::Empty))
      Flush();

proc:
    switch(type)
    {
      case LangDefSpecType::Empty:
        if(paint)
        {
          AddChar(itr, pos);
          //actFormat += *(searchiter->current->format);
          //Flush();
        }
        break;
      case LangDefSpecType::LineTag:
        linetag = true;
        lineback = state;
        searchiter->base = searchiter->current->jumps[0].nextTree;
        searchiter->current = searchiter->base;
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchiter->current->format);
          Flush();
        }
        break;                          /*
      case LangDefSpecType::PushPop:
        if((searchiter->mask | state.globalMask) & searchiter->current->popmask)
        {
          PerformPop(searchiter);
          if(paint)
          {
             actFormat = *searchiter->current->format ;
            AddChar(itr, pos);
            actFormat += *(searchiter->current->format);
            Flush();
          }
          searchiter->current = searchiter->base;
        }
        else
        {
          if(paint)
          {
            AddChar(itr, pos);
            actFormat += *(searchiter->current->format);
            Flush();
            actFormat = *searchiter->current->format ;
          }
          PerformJumpPush(searchiter, true);
          actFormat = *searchiter->current->format ;
        }
        break;    */
      case LangDefSpecType::Jump:
      /*
        if(PerformPop(searchiter))
        {
          if(paint)
          {
            AddChar(itr, pos);
            actFormat += *(searchiter->current->format);
            Flush();
          }
        }
        else
        {
          if(paint)
          {
            AddChar(itr, pos);
            actFormat += *(searchiter->current->format);
            Flush();
          }
          PerformJumpPush(searchiter);
          actFormat = *searchiter->current->format ;
        }                           */
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchiter->current->format);
        }
        if(!PerformPop(searchiter))
          PerformJumpPush(searchiter);
        if(paint)
          Flush();
        searchiter->current = searchiter->base;
        actFormat = *searchiter->current->format ;
        break;
      case LangDefSpecType::Normal:
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchiter->current->format);
          Flush();
        }
        break;
      case LangDefSpecType::Nomatch:
        if(paint)
        {
          AddChar(itr, pos);
          //actFormat += *(searchiter->current->format);//if format remains, flush wont be done
        }
        break;
      case LangDefSpecType::WordTag:
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchiter->current->format);
        }
        itr->GoChar();
        while(langdef->IsAlNum(*(itr->ptr)))
        {
          CheckMarkup(itr,paint);
          if(paint)
            AddChar(itr, pos);
          itr->GoChar();
        }
        CheckMarkup(itr,paint);
        if(paint)
        {
          actFormat += *(searchiter->current->format);
          Flush();
        }
        searchiter->current = searchiter->base;
        itr->RevChar();
        break;
      default:
        break;
    }
    if(*(itr->ptr) == '\n')
      break;

    itr->GoChar();
  }

  CheckMarkup(itr, paint);
  //FlushAll();

  Flush();
  if(linetag)
    state=lineback;
  actFormat = *(state.searchStateStack.top->data.base->format);
  actFormat += actMarkupCombined;
  drawer->DrawEndl(linenum, actFormat);
  //parent->Log(String("mask is ")+String(searchiter->mask)+String(" glmask is ")+String(state.globalMask));

  return;

}
//---------------------------------------------------------------------------
bool Parser::PerformPop(LanguageDefinition::SearchIter *& sit)
{

  //Write(String("going to perform POP with popmask ")+String(popmask)+String(" on ")+actText+String(sit->current->thisItem));
  //DumpStackState();
   for(int i = 0; i < sit->current->recpopcount; i++)
  {
    if(sit->current->pops[i].popmask & (sit->mask | state.globalMask) || sit->current->pops[i].popmask == 0)
    {
      if(sit->current->pops[i].format != NULL)
        actFormat += *(sit->current->pops[i].format);
      LanguageDefinition::TreeNode * dbg = sit->current;
      int popmask = (sit->mask | state.globalMask) & sit->current->pops[i].popmask;
      int newgmask = sit->current->pops[i].newgmask;
      if(sit->current->pops[i].popcount >= 0)
      {
        //Write(String(" manualpopping"));
        for(int j = sit->current->pops[i].popcount ; j > 0; j--)
          state.searchStateStack.Pop();
      }
      else
      {
        //int popmask = sit->current->popmask;
        while(state.searchStateStack.top != NULL && state.searchStateStack.top->data.mask & popmask)
        {
          // Write(String(" autopopping"));
          state.searchStateStack.Pop();
        }
      }
      if(state.searchStateStack.top == NULL)
        state.searchStateStack.Push(langdef->GetDefSC(0));

      state.globalMask = state.globalMask  ^ newgmask;
      sit = &(state.searchStateStack.top->data);
      return true;
    }
  }
  return false;
  //DumpStackState();
}
//---------------------------------------------------------------------------
void Parser::PerformJumpPush(LanguageDefinition::SearchIter *& sit)
{
  /*
     if(sit->base->bankID == sit->current->jumps[0].nextTree->bankID)
     {
     sit->base = sit->current->jumps[0].nextTree;
     }
     else
     {
     state.actBank = sit->current->jumps[0].nextTree->bankID;
     if(state.searchStateStack.top == NULL)
     state.searchStateStack.Push(langdef->GetDefSC(state.actBank));

     sit = &(state.searchStateStack.top->data);
     } */
  for(int i = 0; i < sit->current->jumpcount; i++)
  {
    if((sit->current->jumps[i].pushmask & (sit->mask | state.globalMask)) > 0 || sit->current->jumps[i].pushmask == 0)
    {
      if(sit->current->jumps[i].format != NULL)
        actFormat += *(sit->current->jumps[i].format);

      int newmask = sit->current->jumps[i].newmask;
      int newgmask = sit->current->jumps[i].newgmask;

      LanguageDefinition::TreeNode * newtree = sit->current->jumps[i].nextTree;

      if(sit->current->jumps[i].type != LangDefJumpType::tMask)
      {
        if(sit->current->jumps[i].type == LangDefJumpType::tPush)
        {
          state.searchStateStack.Push(*sit);
          sit = &(state.searchStateStack.top->data);
        }
         sit->base = newtree;
      }

      state.globalMask = state.globalMask  ^ newgmask;
      sit->mask ^= newmask;

      sit->current = sit->base;
      break;
    }
  }
}
//---------------------------------------------------------------------------
bool Parser::CanGoFurther(LanguageDefinition::SearchIter sit, Iter * itr, bool inword, bool checkbefore, bool recursive)
{
  Iter * it2 = itr->Duplicate();
  LangDefSpecType type = LangDefSpecType::Nomatch;
  bool look = false;
  if(checkbefore)
  {
    goto jmp;
    // = langdef->Go(&sit, *(it2->ptr), look);

  }
  while(*(it2->ptr) != '\n' && type == LangDefSpecType::Nomatch)
  {
    look = false;
    it2->GoChar();

jmp:

    bool whiteskipped = false;
    while(langdef->IsWhite(*(it2->ptr)) && *(it2->ptr) != '\n')
    {
      whiteskipped = true;
      it2->GoChar();
    }
    if(whiteskipped && inword && langdef->IsAl(*(it2->ptr)))
    {
      delete it2;
      return false;
    }
    else if(whiteskipped)
      inword = false;

    type = langdef->Go(&sit, *(it2->ptr), look);
    inword = (inword && langdef->IsAlNum(*(it2->ptr))) || langdef->IsAl(*(it2->ptr));
  }
  if(*(it2->ptr) == '\n' || type == LangDefSpecType::Empty || (inword && langdef->IsAlNum(it2->GetNextChar())))
  {
    bool rtn=false;
    if (*(it2->ptr) != '\n' && type != LangDefSpecType::Empty && inword && langdef->IsAlNum(it2->GetNextChar())) //recursivity is not an option here
      rtn = CanGoFurther(sit, it2, inword, false, recursive);
    delete it2;
    return rtn;
  }
  actFormat += *(sit.current->format);
  if(recursive)
    CanGoFurther(sit, it2, inword, inword, true);
  delete it2;
  return true;
}
//---------------------------------------------------------------------------
void Parser::AddChar(Iter * itr, int & pos)
{
  if(*(itr->ptr) == '\n')
    return;
  if(*(itr->ptr) != '\t')
  {
    actText += *(itr->ptr);
    pos++;
    return;
  }
  else
  {
    for(int i = TAB_WIDTH - (pos+1)%TAB_WIDTH; i > 0; i--)
    {
      pos++;
      actText += " ";
    }
    return;
  }
}
//---------------------------------------------------------------------------

void Parser::CheckMarkup(Iter * itr, bool paint)
{
#ifdef DEBUG
  //Write(" markup check");
#endif
  if(itr->word->marks.top != NULL)
  {
    Stack<Mark>::Node * m = itr->word->marks.top;
    while(m != NULL)
    {
      if(m->data.pos == itr->offset)
      {
        if(m->data.begin)
        {
          state.markupStack.Push(m->data.format);
          if(paint)
          {
            CanGoFurther(state.searchStateStack.top->data, itr, this->inword, true, FORCELOOKAHEAD ); //its side effect is correction of format
            Flush();
            actMarkup += *(m->data.format);
            actMarkupCombined = actMarkup;
            actMarkupCombined += actIMarkup;
          }
        }
        else
        {
          //actTask->format = *(langdef->GetTree()->format);
          // Flush(); //it should not be here - should it?
          //MarkupPop(&(state.markupStack), (*m)->format);
          state.markupStack.Remove(m->data.format);
          if(paint)
          {
            CanGoFurther(state.searchStateStack.top->data, itr, this->inword, true, FORCELOOKAHEAD ); //its side effect is correction of format
            Flush();
            ReconstructMarkup();
            actMarkupCombined = actMarkup;
            actMarkupCombined += actIMarkup;
          }
        }
      }
      m = m->next;
    }
  }
  if(itr->pos == itr->nextimark && itr->linenum == itr->nextimarkln && paint)
  {
    CanGoFurther(state.searchStateStack.top->data, itr, this->inword, true, FORCELOOKAHEAD ); //its side effect is correction of format
    Flush();
    actIMarkup = itr->ReconstructIMarkFontStyle();
    actMarkupCombined = actMarkup;
    actMarkupCombined += actIMarkup;
    itr->UpdateNextImark();
  }
}
//---------------------------------------------------------------------------
void Parser::ReconstructMarkup()
{
  Stack<Format*> formats;
  Stack<Format*>::Node * m = state.markupStack.top;
  actMarkup = FontStyle();

  while(m != NULL)
  {
    formats.Push(m->data);
    m = m->next;
  }

  m = formats.top;
  while(m != NULL)
  {
    actMarkup += *(m->data);
    m = m->next;
  }
}
//---------------------------------------------------------------------------
void Parser::SetLangDef(LanguageDefinition * langdef)
{
  this->langdef = langdef;
}
//---------------------------------------------------------------------------
void Parser::Flush()
{
  if(!actText.IsEmpty())
  {
    actFormat += actMarkupCombined;
    drawer->DrawText(actText, newline, linenum, actFormat);
    newline = false;
    actText = "";
    actFormat = *(state.searchStateStack.top->data.base->format);
  }
}
//---------------------------------------------------------------------------
void Parser::FlushAll()
{
  Flush();
  actFormat = *(state.searchStateStack.top->data.base->format);
  actFormat += actMarkupCombined;
  drawer->DrawEndl(linenum, actFormat);
}
//---------------------------------------------------------------------------
void Parser::SendEof()
{
  drawer->DrawEof(linenum+1);
}
//---------------------------------------------------------------------------
void Parser::InvalidateAll()
{
  currentparseid++;
}
//---------------------------------------------------------------------------
__fastcall Parser::~Parser()
{
}
//---------------------------------------------------------------------------
#ifdef DEBUG
void Parser::DumpStackState()
{
#ifdef DEBUG_LOGGING
/*
  Write(String("  current bank is ")+String(state.actBank));
  for(int i = 0; i < langdef->GetBankIdCount(); i++)
  {
    Write(String("  stack of bank ")+String(i)+String(" is "));
    for(Stack<LanguageDefinition::SearchIter>::Node * n = state.searchStateBank[i].top; n != NULL; n = n->next)
      Write(String("    ")+String(n->data.base->Name)+String(" - ")+String(n->data.mask));
  }     */
#endif
}
//---------------------------------------------------------------------------
void Parser::Write(AnsiString message)
{
#ifdef DEBUG_LOGGING
  if(dbgLogging)
    myfile << message.c_str() << std::endl;
#endif
}
#endif
//---------------------------------------------------------------------------



