//---------------------------------------------------------------------------
#pragma hdrstop


#include "uParser.h"
#include "uDrawer.h"
#include "uLanguageDefinition.h"
#include "cSQLSyntax.h"
#include "uSpan.h"
#include "uIter.h"
#include "uMark.h"
#include "uBuffer.h"
#include "uFormat.h"
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
//---------------------------------------------------------------------------
Parser::ParserState::ParserState()
  : markupStack(), searchStateStack()
{
  //stateid = 0;
}
//---------------------------------------------------------------------------
Parser::ParserState::~ParserState()
{
  markupStack.Erase();
  searchStateStack.Erase();
}
//---------------------------------------------------------------------------
SHEdit::Parser::ParserState& Parser::ParserState::operator=(const SHEdit::Parser::ParserState& p)
{
  parseid = p.parseid;
  searchStateStack = p.searchStateStack;
  markupStack = p.markupStack;

  return *this;
}
//---------------------------------------------------------------------------
bool Parser::ParserState::operator==(const ParserState& state)
{
  return (this->searchStateStack == state.searchStateStack && this->parseid == state.parseid &&  markupStack == state.markupStack);
}
//---------------------------------------------------------------------------
bool Parser::ParserState::operator!=(const ParserState& state)
{
  return !(this->searchStateStack == state.searchStateStack && this->parseid == state.parseid && markupStack == state.markupStack);
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
  return this->linenum == pt.linenum;
}
//---------------------------------------------------------------------------
bool Parser::ParseTask::operator<(const Parser::ParseTask & pt)  const
{
  return this->linenum < pt.linenum;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall Parser::Parser(TSQLEdit * parent, Drawer * drawer)
{
#ifdef DEBUG
  myfile.open("main.txt", ios::out);
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
  while(tasklistprior.size() > 0 || (tasklist.size() > 0 && tasklist.front().linenum < upperbound))
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
    Iter * itr = new Iter(pt.line, linenum >= 0 ? parent->itrLine->linenum+linenum : pt.linenum, 0, parent->buffer);
    state = itr->line->parserState;
    state.parseid = currentparseid;

    if(linenum >= 0)
    {
      ReconstructMarkup();
      actIMarkup = itr->ReconstructIMarkFontStyle();
      actMarkupCombined = actMarkup;
      actMarkupCombined += actIMarkup;
      itr->UpdateNextImark();
    }

    if(state.searchStateStack.top == NULL)
      state.searchStateStack.Push(langdef->GetDefSC());
    LanguageDefinition::SearchIter& searchtoken = state.searchStateStack.top->data;

    tasklistprior.remove(pt);
    tasklist.remove(pt);

    while(itr->word->next && (first || itr->line->parserState != this->state || tasklistprior.front().line == itr->line))
    {
      //take care of record from list
      newline = true;
      first = false;
      //take care of line
      itr->line->parserState = this->state;

      tasklistprior.remove(pt);
      tasklist.remove(pt);

      //parent->Log(String("parsing line ")+String(itr->linenum));
      ParseLine(itr, searchtoken, linenum >= 0);

      itr->GoChar();
      if(linenum >= 0)
      {
        //FlushAll();
        painted = true;
        linenum = parent->GetLineNum(itr->line);
      }
      else if(parent->GetLineFirst(itr->line))
      {
        itr->linenum = parent->itrLine->linenum;
        ReconstructMarkup();
        actIMarkup = itr->ReconstructIMarkFontStyle();
        actMarkupCombined = actMarkup;
        actMarkupCombined += actIMarkup;
        itr->UpdateNextImark();

        linenum = 0;
      }
      pt.linenum++;
      parsed++;

      //if(!(linenum >= 0 && linenum <= parent->GetVisLineCount() && itr->line->parserState != this->state) && tasklistprior.size() > 0 )
      if(linenum < 0 && (tasklistprior.size() > 0 || itr->linenum > upperbound || parsed > PARSEINONEGO))
      {
        if(itr->word->next != NULL && itr->line->parserState != this->state)
        {
          this->tasklist.push_front(ParseTask(itr->line, itr->linenum));
        }
        //itr->line->parserState = this->state; //will be done after breaak
        break;
      }
    }
    if(!itr->word->next && linenum >= 0)
    {
      if(first)  //means line has not been either parsed or flushed, because iter was already on nextline (empty line)
        FlushAll();
      SendEof();
    }

    itr->line->parserState = this->state;

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
void Parser::ParseLine(Iter * itr, LanguageDefinition::SearchIter& searchtoken, bool paint)
{
#ifdef DEBUG
  //Write(" entering parseline");
#endif
  wchar_t * ptr;
  LanguageDefinition::SearchIter lineback;
  bool linetag = false;
  int pos = 0;

  while(*(itr->ptr) != '\n')
  {
      CheckMarkup(itr, paint);

    //lasttoken = searchtoken->nextTree;
    bool lookahead = true;
    int type;
    if(langdef->IsAl(*(itr->ptr)))
    {
      Flush();
      type = langdef->Go(searchtoken, *(itr->ptr), lookahead);
      lookahead = false;
      while(langdef->IsAlNum(*(itr->ptr)) && (type == LangDefSpecType::Nomatch || (type != LangDefSpecType::Empty && langdef->IsAlNum(itr->GetNextChar()))))
      {
        if(paint)
          AddChar(itr, pos);
        itr->GoChar();
          CheckMarkup(itr,paint);
        type = langdef->Go(searchtoken, *(itr->ptr), lookahead);
      }
      if(!langdef->IsAlNum(*(itr->ptr)))
      {
        searchtoken.current = searchtoken.base;
        if(paint)
          actFormat += *(searchtoken.current->format);
        Flush();
        type = langdef->Go(searchtoken, *(itr->ptr), lookahead);
        if(*(itr->ptr) == '\n')
          break;
      }
      else if( langdef->IsAlNum(itr->GetNextChar()))
      {
        type = LangDefSpecType::WordTag;
        searchtoken.current = searchtoken.base;
      }
      //else everything is gonna work
    }
    else
    {
      type = langdef->Go(searchtoken, *(itr->ptr), lookahead);
    }
    #ifdef DEBUG

        int dbgtype = type;
        if( dbgLogging )
          parent->Log( String(*(itr->ptr))+String(" ")+String(type)+String(" ")+String((int)lookahead));
        //Write(String(*(itr->ptr))+String(" ")+String(type)+String(" ")+String(searchtoken->type));

    #endif
    if(lookahead)
      Flush();
    switch(type)
    {
      case LangDefSpecType::Empty:
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchtoken.current->format);
          Flush();
        }
        break;         
      case LangDefSpecType::LineTag:
        linetag = true;
        lineback = searchtoken;
        searchtoken.base = searchtoken.current->jumps[0].nextTree;
        searchtoken.current = searchtoken.base;
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchtoken.current->format);
          Flush();
        }
        break;
      case LangDefSpecType::PushPop:
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchtoken.current->format);
          Flush();
        }
        if(searchtoken.mask & searchtoken.current->popmask)
        {
          for(int i = searchtoken.current->popcount; i > 0; i--)
            state.searchStateStack.Pop();
          if(state.searchStateStack.top == NULL)
            state.searchStateStack.Push(langdef->GetDefSC());
          searchtoken = state.searchStateStack.top->data;
        }
        else
        {
          for(int i = 0; i < searchtoken.current->jumpcount; i++)
          {
            if(searchtoken.current->jumps[i].pushmask & searchtoken.mask)
            {
              state.searchStateStack.Push(searchtoken);
              searchtoken = state.searchStateStack.top->data;
              searchtoken.base = searchtoken.current->jumps[i].nextTree;
              searchtoken.current = searchtoken.base;
              break;
            }
          }
        }
        break;
      case LangDefSpecType::Jump:
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchtoken.current->format);
          Flush();
        }
        searchtoken.base = searchtoken.current->jumps[0].nextTree;
        searchtoken.current = searchtoken.base;
        break;
      case LangDefSpecType::Normal:
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchtoken.current->format);
          Flush();
        }
        break;
      case LangDefSpecType::Nomatch:
        if(paint)
        {
          AddChar(itr, pos);
          actFormat += *(searchtoken.current->format);//if format remains, flush wont be done
        }
        break;
      case LangDefSpecType::WordTag:
        if(paint)
        {
          AddChar(itr, pos);
          actFormat = *(searchtoken.current->format);
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
          actFormat += *(searchtoken.current->format);
          Flush();
        }
        searchtoken.current = searchtoken.base;
        itr->RevChar();
        break;
      default:
        break;
    }
    itr->GoChar();
  }

  CheckMarkup(itr, paint);
  FlushAll();
  if(linetag)
    searchtoken = lineback;
  return;

}
//---------------------------------------------------------------------------
void Parser::AddChar(Iter * itr, int & pos)
{
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
    actFormat = *(langdef->GetTree()->format);
  }
}
//---------------------------------------------------------------------------
void Parser::FlushAll()
{
  Flush();
  drawer->DrawEndl(linenum);
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
void Parser::Write(AnsiString message)
{
#ifdef DEBUG_LOGGING
  myfile << message.c_str() << std::endl;
#endif
}
#endif
//---------------------------------------------------------------------------



