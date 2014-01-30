//---------------------------------------------------------------------------
#pragma hdrstop


#include "uParser.h"
#include "uDrawer.h"
#include "uLanguageDefinition.h"
#include "cSQLSyntax.h"
#include "uSpan.h"
#include "uIter.h"
#include "uMark.h"
#include "uFormat.h"
#include <string>
#include <stdio.h>
#include <time.h>
#include <list>

using namespace SHEdit;

#ifdef DEBUG
#include <fstream>
std::ofstream myfile;
#endif

#pragma package(smart_init)
//---------------------------------------------------------------------------
Parser::ParserState::ParserState()
{
  statemask = 0;
  markupStack = NULL;
}
//---------------------------------------------------------------------------
SHEdit::Parser::ParserState& Parser::ParserState::operator=(const SHEdit::Parser::ParserState& p)
{
  parseid = p.parseid;
  statemask = p.statemask;
  while(markupStack != NULL)
  {
    Node *n = markupStack;
    markupStack = markupStack->node;
    delete n;
  }
  if(p.markupStack != NULL)
  {
    Node * m = p.markupStack;
    Node ** n = &markupStack;
    while(m != NULL)
    {
      MarkupPush(n, m->format);
      n = &((*n)->node);
      m = m->node;
    }
  }
  return *this;
}
//---------------------------------------------------------------------------
Parser::Node::Node(SHEdit::Format * format, SHEdit::Parser::Node * node)
{
  this->format = format;
  this->node = node;
}

//---------------------------------------------------------------------------
bool Parser::ParserState::operator==(const ParserState& state)
{
  return (this->statemask == state.statemask && this->parseid == state.parseid &&  MarkupEquals(markupStack, state.markupStack));
}
//---------------------------------------------------------------------------
bool Parser::ParserState::operator!=(const ParserState& state)
{
  return !(this->statemask == state.statemask && this->parseid == state.parseid && MarkupEquals(markupStack, state.markupStack));
}
//---------------------------------------------------------------------------
void Parser::MarkupPush(SHEdit::Parser::Node ** at, SHEdit::Format * format)
{
  *at = new Node(format, *at);
}
//---------------------------------------------------------------------------
bool Parser::MarkupContains(SHEdit::Parser::Node ** at, SHEdit::Format * format)
{
  if(*at != NULL)
  {
    Node ** m = at;
    while(*m != NULL)
    {
      if((*m)->format == format)
      {
        return true;
      }
      m = &((*m)->node);
    }
  }
  return false;
}
//---------------------------------------------------------------------------
bool Parser::MarkupEquals(SHEdit::Parser::Node * at, SHEdit::Parser::Node * bt)
{
  if(at != NULL && bt != NULL)
  {
    Node * a = at;
    Node * b = bt;
    while(a != NULL && b != NULL)
    {
      if(a->format != b->format)
      {
        return false;
      }
      a = a->node;
      b = b->node;
    }
    return (b == a);
  }
  return (at == NULL && bt == NULL);
}
//---------------------------------------------------------------------------
void Parser::MarkupPop(SHEdit::Parser::Node ** at, SHEdit::Format * format)
{
  if(*at != NULL)
  {
    Node ** m = at;
    while(*m != NULL)
    {
      if((*m)->format == format)
      {
        Node *n = *m;
        *m = (*m)->node;
        delete n;
      }
      else
        m = &((*m)->node);
    }
  }
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
  __fastcall Parser::Parser(TSQLEdit * parent, Drawer * drawer, HANDLE bufferChanged, HANDLE bufferMutex, HANDLE drawerCanvasMutex, HANDLE drawerQueueMutex, HANDLE drawerTaskPending)
{
#ifdef DEBUG
  myfile.open("parser.txt", ios::out);
  dbgLogging = false;
#endif
  this->parent = parent;
  this->drawer = drawer;

  processAll = true;
  //DuplicateHandle(GetCurrentProcess(), bufferChanged, this->Handle, &(this->bufferChanged),  0, false, DUPLICATE_SAME_ACCESS);

  DuplicateHandle(GetCurrentProcess(), bufferChanged, GetCurrentProcess(), &(this->bufferChanged),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), bufferMutex, GetCurrentProcess(), &(this->bufferMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerQueueMutex, GetCurrentProcess(), &(this->drawerQueueMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerCanvasMutex, GetCurrentProcess(), &(this->drawerCanvasMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerTaskPending, GetCurrentProcess(), &(this->drawerTaskPending),  0, false, DUPLICATE_SAME_ACCESS);
}
//---------------------------------------------------------------------------
void __fastcall Parser::Execute()
{
    NSpan* line=NULL;
    //parse lists
    tasklist.sort();
    tasklistprior.sort();

    while(tasklist.size() > 0 || tasklistprior.size() > 0)
    {
      ParseTask pt;
      if(tasklistprior.size() > 0)
       {
        pt = tasklistprior.front();
        }
        else
        {
        pt = tasklist.front(); //TODO handle linenum
        }
      line = pt.line;
      linenum = parent->GetLineNum(line);
      bool first = true;
      bool painted = linenum >= 0;
      Iter * itr = new Iter(line);
      state = itr->line->parserState;
      LanguageDefinition::TreeItem * searchtoken = langdef->GetSpecItem(state.statemask);

        tasklistprior.remove(pt);
        tasklist.remove(pt);

      while(itr->word->next && (first || itr->line->parserState != this->state))
      {
      //take care of record from list
        newline = true;
        first = false;
        //take care of line
        itr->line->parserState = this->state;

        tasklistprior.remove(pt);
        tasklist.remove(pt);

        ParseLine(itr, searchtoken, linenum >= 0);

        if(linenum >= 0)
        {
          FlushAll();
        }
        itr->GoChar();
        if(linenum >= 0)
        {
          painted = true;
          linenum = parent->GetLineNum(itr->line);
        }
        else if(parent->GetLineFirst(itr->line))
        {
          painted = true;
          linenum = 0;
        }
        pt.linenum++;

        this->state.parseid = currentparseid;

        #ifdef DEBUG
        //if(itr->line->prevline != NULL)
        //  parent->Log("parsing line " + String(itr->line->prevline->next->string));
          #endif

        //if(!(linenum >= 0 && linenum <= parent->GetVisLineCount() && itr->line->parserState != this->state) && tasklistprior.size() > 0 )
        if(linenum < 0 && tasklistprior.size() > 0 )
        {
          if(itr->word->next != NULL && itr->line->parserState != this->state)
            this->tasklist.push_back(ParseTask(itr->line, linenum));
          itr->line->parserState = this->state;
          break;
        }
        if(linenum < 0 && painted && itr->line->parserState != this->state)
        {
          itr->line->parserState = this->state;
          itr->line->parserState.parseid--;
          break;
        }
      }
      if(!itr->word->next && linenum >= 0)
      {
        if(first)  //means line has not been either parsed or flushed, because iter was already on nextline (empty line)
          FlushAll();
        SendEof();
      }

      //itr->line->parserState = this->state;

      delete itr;

      if(painted && tasklistprior.size() == 0)
      {
        drawer->Paint();
        if(processAll)
          Application->ProcessMessages();
      }
    }
    drawer->Paint();
}
//---------------------------------------------------------------------------
void Parser::ParseFromLine(NSpan * line, int linenum, int prior)
{
#ifdef DEBUG
    //Write("pushing line");
#endif
  if(prior > 0)
  {
    tasklistprior.push_back(ParseTask(line, linenum));
  }
  else
  {
    tasklist.push_back(ParseTask(line, linenum));
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
/*
 *lines are parsed recursively - every time special tag is encountered, another level of recursion is entered
 */
void Parser::ParseLine(Iter * itr, LanguageDefinition::TreeItem *& searchtoken, bool paint)
{
#ifdef DEBUG
    //Write(" entering parseline");
#endif
  wchar_t * ptr;
  LanguageDefinition::TreeItem * lineback;
  LanguageDefinition::TreeItem * lasttoken;
  bool linetag = false;
  int pos = 0;

      while(*(itr->ptr) != '\n')
      {
        if(itr->word->mark)
          CheckMarkup(itr, paint);

        lasttoken = searchtoken->nextTree;
        bool lookahead = true;
        int type;
        if(langdef->IsAl(*(itr->ptr)))
        {
          Flush();
          type = langdef->Go(searchtoken, *(itr->ptr), lookahead);
          lookahead = false;
          while(langdef->IsAlNum(*(itr->ptr)) && type == LangDefSpecType::Nomatch)
          {
            if(paint)
                AddChar(itr, pos);
            itr->GoChar();
            if(itr->word->mark)
             CheckMarkup(itr,paint);
            type = langdef->Go(searchtoken, *(itr->ptr), lookahead);
          }
          if(!langdef->IsAlNum(*(itr->ptr)))
          {
            searchtoken = lasttoken;
            if(paint)
              actFormat = *(searchtoken->format);
            Flush();
            type = langdef->Go(searchtoken, *(itr->ptr), lookahead);
            if(*(itr->ptr) == '\n')
              break;
          }
          else if( langdef->IsAlNum(itr->GetNextChar()))
          {
            type = LangDefSpecType::WordTag;
            searchtoken = lasttoken;
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
              actFormat = *(searchtoken->format);
              Flush();
            }
            break;         
          case LangDefSpecType::LineTag:
            linetag = true;
            lineback = lasttoken;
            goto paint;
          case LangDefSpecType::PairTag:
            //Flush();
            state.statemask = searchtoken->mask;
          case LangDefSpecType::Normal:
paint:
            if(paint)
            {
                AddChar(itr, pos);
              actFormat = *(searchtoken->format);
              Flush();
            }
            break;
          case LangDefSpecType::Nomatch:
            if(paint)
            {
                AddChar(itr, pos);
              actFormat = *(searchtoken->format);//if format remains, flush wont be done
#ifdef DEBUG
    //Write(String("   nomatch - outtask is ")+String((*outTask->text))+String(" acttask is ") + String(*actTask->text));
#endif
            }
            break;
          case LangDefSpecType::WordTag:
            if(paint)
            {
                AddChar(itr, pos);
              actFormat = *(searchtoken->format);
            }
            itr->GoChar();
            while(langdef->IsAlNum(*(itr->ptr)))
            {
              if(itr->word->mark)
                CheckMarkup(itr,paint);
              if(paint)
                AddChar(itr, pos);
              itr->GoChar();
            }
            if(itr->word->mark)
              CheckMarkup(itr,paint);
            if(paint)
            {
              actFormat = *(searchtoken->format);
              Flush();
            }
            searchtoken = searchtoken->nextTree;
            itr->RevChar();
            break;
          default:
          break;
        }
        itr->GoChar();
      }

      if(itr->word->mark)
        CheckMarkup(itr, paint);
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
  Mark ** m = &(itr->word->mark);
  while(*m != NULL)
  {
    if((*m)->pos == itr->offset)
    {
      if(paint)
        Flush();
      if((*m)->begin)
      {
        MarkupPush(&(state.markupStack), (*m)->format);
      }
      else
      {
        //actTask->format = *(langdef->GetTree()->format);
        Flush();
        MarkupPop(&(state.markupStack), (*m)->format);
      }
    }
    m = &((*m)->mark);
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
    if(state.markupStack != NULL)
      actFormat = *(state.markupStack->format);
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



