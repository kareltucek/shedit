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
  parsed = p.parsed;
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
  return (this->statemask == state.statemask && this->parsed == state.parsed &&  MarkupEquals(markupStack, state.markupStack));
}
//---------------------------------------------------------------------------
bool Parser::ParserState::operator!=(const ParserState& state)
{
  return !(this->statemask == state.statemask && this->parsed == state.parsed && MarkupEquals(markupStack, state.markupStack));
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
  __fastcall Parser::Parser(TSQLEdit * parent, Drawer * drawer, HANDLE bufferChanged, HANDLE bufferMutex, HANDLE drawerCanvasMutex, HANDLE drawerQueueMutex, HANDLE drawerTaskPending)
: TThread(true)
{
#ifdef DEBUG
  myfile.open("parser.txt", ios::out);
#endif
  this->parent = parent;
  this->drawer = drawer;
  //DuplicateHandle(GetCurrentProcess(), bufferChanged, this->Handle, &(this->bufferChanged),  0, false, DUPLICATE_SAME_ACCESS);

  DuplicateHandle(GetCurrentProcess(), bufferChanged, GetCurrentProcess(), &(this->bufferChanged),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), bufferMutex, GetCurrentProcess(), &(this->bufferMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerQueueMutex, GetCurrentProcess(), &(this->drawerQueueMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerCanvasMutex, GetCurrentProcess(), &(this->drawerCanvasMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerTaskPending, GetCurrentProcess(), &(this->drawerTaskPending),  0, false, DUPLICATE_SAME_ACCESS);

  this->actTask = new DrawTaskText();
  this->outTask = new DrawTaskText();

}
//---------------------------------------------------------------------------
void __fastcall Parser::Execute()
{
  this->Priority=tpHigher;
  while(!Terminated)
  {
    for(int i = 0; langdef == NULL; i++)
    {
      WaitForSingleObject(bufferChanged, WAIT_TIMEOUT_TIME);
      ResetEvent(bufferChanged);
    }
    int dbg = WaitForSingleObject(bufferChanged, WAIT_TIMEOUT_TIME);

    NSpan* line=NULL;
    //parse lists
#ifdef DEBUG
    Write(String("queue size ")+String((int)(tasklist.size()+tasklistprior.size())));
    WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
    clock_t timer = clock();
    int dbglines = 0;
#endif
    while(tasklist.size() > 0 || tasklistprior.size() > 0)
    {
      //take care of record from list
#ifndef DEBUG
      WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
#endif

      line = tasklistprior.size() > 0 ? tasklistprior.front() : tasklist.front();
      linenum = parent->GetLineNum(line);
      bool first = true;
      Iter * itr = new Iter(line);
      state = itr->line->parserState;
      LanguageDefinition::TreeItem * searchtoken = langdef->GetSpecItem(state.statemask);
      tasklistprior.remove(line);  //has to be there! (cycle wont end if anything is invalid and task remains)
      tasklist.remove(line);

      while(itr->word->next && (first || itr->line->parserState != this->state))
      {
      //take care of record from list
#ifdef DEBUG
      dbglines++;
#endif
        newline = true;
        first = false;
        //take care of line
        itr->line->parserState = this->state;
        tasklistprior.remove(itr->line);
        tasklist.remove(itr->line);
#ifdef DEBUG
    Write("going into parseline");
#endif
        ParseLine(itr, searchtoken, linenum >= 0);
#ifdef DEBUG
    Write("going out of parseline");
#endif
        if(linenum >= 0)
        {
          FlushAll();
        }
        itr->GoChar();
        if(linenum >= 0)
          linenum = parent->GetLineNum(itr->line);
        else if(parent->GetLineFirst(itr->line))
          linenum = 0;

        ReleaseMutex(bufferMutex); //let buffer to push new task

        this->state.parsed = true;

        WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);

        //if(!(linenum >= 0 && linenum <= parent->GetVisLineCount() && itr->line->parserState != this->state) && tasklistprior.size() > 0 )
        if(linenum < 0 && tasklistprior.size() > 0)
        {
          if(itr->word->next)
            this->tasklist.push_back(itr->line);
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

      delete itr;
#ifndef DEBUG
      ReleaseMutex(bufferMutex);
#endif
    }
#ifdef DEBUG
      ReleaseMutex(bufferMutex);
    double time = (clock()-timer)/((double)CLOCKS_PER_SEC);
    Write(String("parsed ")+String(dbglines)+String(" in ")+String(time)+String(" queuesize is ")+String((int)(tasklist.size()+tasklistprior.size())));
#endif
    ResetEvent(bufferChanged);
  }
}
//---------------------------------------------------------------------------
void Parser::ParseFromLine(NSpan * line, int prior)
{
  if(prior > 0)
  {
    tasklistprior.remove(line);
    tasklistprior.push_back(line);
  }
  else
  {
    tasklist.remove(line);
    tasklist.push_back(line);
  }
}

//---------------------------------------------------------------------------
/*
 *lines are parsed recursively - every time special tag is encountered, another level of recursion is called
 */
void Parser::ParseLine(Iter * itr, LanguageDefinition::TreeItem *& searchtoken, bool paint)
{
#ifdef DEBUG
    Write(" entering parseline");
#endif
  wchar_t * ptr;
  LanguageDefinition::TreeItem * lineback;
  LanguageDefinition::TreeItem * lasttoken;
  bool linetag = false;
  switch(searchtoken->type)
  {
    case LangDefSpecType::Normal:
    case LangDefSpecType::Nomatch:
    case LangDefSpecType::Empty:
    case LangDefSpecType::NoEmpty:
    case LangDefSpecType::PairTag:
    case LangDefSpecType::LineTag:
#ifdef DEBUG
    Write(" normal switch");
#endif
      while(*(itr->ptr) != '\n')
      {

#ifdef DEBUG
    Write("   while");
#endif
        if(itr->word->mark)
          CheckMarkup(itr, paint);

#ifdef DEBUG
    Write("   markup checked");
#endif
        lasttoken = searchtoken;
        switch(langdef->Go(searchtoken, *(itr->ptr)))
        {
          case LangDefSpecType::Empty:
#ifdef DEBUG
    Write("   1");
#endif
            if(paint)
            {
              *(actTask->text) += *(itr->ptr);
              actTask->format = *(searchtoken->format);
              Flush();
            }
            break;
          case LangDefSpecType::NoEmpty:
#ifdef DEBUG
    Write("   2");
#endif
            if(paint)
            {
              Flush();
              *(actTask->text) += *(itr->ptr);
              actTask->format = *(searchtoken->format);
            }
#ifdef DEBUG
    Write("   2e");
#endif
            break;
          case LangDefSpecType::LineTag:
#ifdef DEBUG
    Write("   3");
#endif
            linetag = true;
            lineback = lasttoken;
            goto paint;
          case LangDefSpecType::PairTag:
#ifdef DEBUG
    Write("   4");
#endif
            Flush();
            state.statemask = searchtoken->mask;
          case LangDefSpecType::Normal:
#ifdef DEBUG
    Write("   5");
#endif
paint:
            if(paint)
            {
              *(actTask->text) += *(itr->ptr);
              actTask->format = *(searchtoken->format);
              Flush();
            }
            break;
          case LangDefSpecType::Nomatch:
#ifdef DEBUG
    Write("   6");
#endif
            if(paint)
              *(actTask->text) += *(itr->ptr);
            break;
          case LangDefSpecType::WordTag:
#ifdef DEBUG
    Write("   7");
#endif
            if(paint)
              *(actTask->text) += *(itr->ptr);
            itr->GoChar();
            ParseLine(itr, searchtoken, paint);
            return;
          default:
#ifdef DEBUG
    Write("   muhaha");
#endif
          break;
        }

#ifdef DEBUG
    Write("   gotta gochar");
#endif
        itr->GoChar();
#ifdef DEBUG
    Write("   wentchar");
#endif
      }

#ifdef DEBUG
    Write(" got through");
#endif
      if(itr->word->mark)
        CheckMarkup(itr, paint);
      if(linetag)
        searchtoken = lineback;
#ifdef DEBUG
    Write(" returning");
#endif
      return;
      break;
    case LangDefSpecType::WordTag:
#ifdef DEBUG
    Write(" wordtag switch");
#endif
      actTask->format = *(searchtoken->format);
      while(langdef->IsAlNum(*(itr->ptr))) //IsAlNum should return true for \n //seems like accidental comment
      {
        if(itr->word->mark)
          CheckMarkup(itr,paint);
        if(paint)
          *(actTask->text) += *(itr->ptr);
        itr->GoChar();
      }
      if(itr->word->mark)
        CheckMarkup(itr,paint);
      if(paint)
        Flush();
      searchtoken = searchtoken->nextTree;
      ParseLine(itr, searchtoken, paint);
      return;
  }
}
//---------------------------------------------------------------------------

void Parser::CheckMarkup(Iter * itr, bool paint)
{
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
  if(state.markupStack != NULL)
  {
    actTask->format = *(state.markupStack->format);
  }

  if(actTask->format != outTask->format)
  {
    SendString();
  }
  else
  {
    *(outTask->text) += *(actTask->text);
    actTask->text = new String("");
  }
}
//---------------------------------------------------------------------------
void Parser::FlushAll()
{
  Flush();
  SendString();
  DrawTaskEndline * task = new DrawTaskEndline();
  task->linenum = this->linenum;
  //WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
  drawer->Draw(task);
  //ReleaseMutex(drawerQueueMutex);
}
//---------------------------------------------------------------------------
void Parser::SendString()
{
  if(!outTask->text->IsEmpty())
  {
    outTask->newline = newline;
    if(newline)
      newline = false;
    outTask->linenum = this->linenum;
    //WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
    drawer->Draw(outTask);
   // ReleaseMutex(drawerQueueMutex);
  }
  else
  {
    delete outTask;
  }
  outTask = actTask;
  actTask = new DrawTaskText();
  actTask->format = *(langdef->GetTree()->format);
}
//---------------------------------------------------------------------------
void Parser::SendEof()
{
  DrawTaskEof * task = new DrawTaskEof();
  task->linenum = this->linenum+1;
  //WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
  drawer->Draw(task);
  //ReleaseMutex(drawerQueueMutex);
}
//---------------------------------------------------------------------------
__fastcall Parser::~Parser()
{
}
//---------------------------------------------------------------------------
#ifdef DEBUG
bool Parser::Write(AnsiString message)
{
  myfile << message.c_str() << std::endl;
  return true;
}
#endif
//---------------------------------------------------------------------------



