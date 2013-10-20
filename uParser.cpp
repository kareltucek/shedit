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
  parentheses = 0;
  braces = 0;
  brackets = 0;
  statemask = 0;
  markupStack = NULL;
}
//---------------------------------------------------------------------------
SHEdit::Parser::ParserState& Parser::ParserState::operator=(const SHEdit::Parser::ParserState& p)
{
  parentheses = p.parentheses;
  braces = p.braces;
  brackets = p.brackets;
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
  return (this->statemask == state.statemask && this->braces == state.braces && this->brackets == state.brackets && this->parentheses == state.parentheses && MarkupEquals(markupStack, state.markupStack));
}
//---------------------------------------------------------------------------
bool Parser::ParserState::operator!=(const ParserState& state)
{
  return !(this->statemask == state.statemask && this->braces == state.braces && this->brackets == state.brackets && this->parentheses == state.parentheses && MarkupEquals(markupStack, state.markupStack));
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
    while(a != NULL)
    {
      if(a->format != b->format)
        return false;
      a = a->node;
      b = b->node;
    }
    return (b == NULL);
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
  while(!Terminated)
  {
#ifdef DEBUG
    Write("beginning new cycle");
#endif
    for(int i = 0; langdef == NULL; i++)
    {
      WaitForSingleObject(bufferChanged, WAIT_TIMEOUT_TIME);
      ResetEvent(bufferChanged);
    }
#ifdef DEBUG
    Write("waiting for buffer changed");
#endif
    int dbg = WaitForSingleObject(bufferChanged, WAIT_TIMEOUT_TIME);
#ifdef DEBUG
    Write(String("BufferChangedEvent") + String(dbg) + " error code " + String(GetLastError()));
#endif
    NSpan* line=NULL;
    //parse lists
    while(tasklist.size() > 0 || tasklistprior.size() > 0)
    {
      //take care of record from list
      WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
      line = tasklistprior.size() > 0 ? tasklistprior.front() : tasklist.front();
      linenum = parent->GetLineNum(line);
      bool first = true;
      Iter * itr = new Iter(line);
      state = itr->line->parserState;
      LanguageDefinition::TreeItem * searchtoken = langdef->GetSpecItem(state.statemask);
      tasklistprior.remove(itr->line);  //has to be there! (cycle wont end if anything is invalid and task remains)
      tasklist.remove(itr->line);
      while(itr->word->next && (first || itr->line->parserState != this->state))
      {
        first = false;
        //take care of line
        itr->line->parserState = this->state;
        tasklistprior.remove(itr->line);
        tasklist.remove(itr->line);
        ParseLine(itr, searchtoken, linenum >= 0);
        if(linenum >= 0);
        {
          FlushAll();
        }
        itr->GoChar();
        if(linenum >= 0)
          linenum = parent->GetLineNum(itr->line);
        else if(parent->GetLineFirst(itr->line))
          linenum = 0;
        ReleaseMutex(bufferMutex); //let buffer to push new task

        this->state.statemask = this->state.statemask | MASK_PARSED;

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
      ReleaseMutex(bufferMutex);

      delete itr;
#ifdef DEBUG
      Write("synced");
#endif
    }
    ResetEvent(bufferChanged);
#ifdef DEBUG
    Write("synced after all tasks");
#endif
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
  Write("entering ParseLine function");
#endif
  wchar_t * ptr;
  switch(searchtoken->type)
  {
    case LangDefSpecType::Normal:
    case LangDefSpecType::Nomatch:
    case LangDefSpecType::Empty:
      while(*(itr->ptr) != '\n')
      {
        if(itr->word->mark)
          CheckMarkup(itr, paint);
        switch(langdef->Go(searchtoken, *(itr->ptr)))
        {
          case LangDefSpecType::Empty:
            switch(*(itr->ptr))
            {
              case '(':
                state.parentheses++;
                break;
              case '[':
                state.brackets++;
                break;
              case '{':
                state.braces++;
                break;
              case ')':
                state.parentheses--;
                break;
              case ']':
                state.brackets--;
                break;
              case '}':
                state.braces--;
                break;
            }
            if(paint)
            {
              *(actTask->text) += *(itr->ptr);
              actTask->format = *(searchtoken->format);
              Flush();
            }
            break;
          case LangDefSpecType::Normal:
            if(paint)
            {
              *(actTask->text) += *(itr->ptr);
              actTask->format = *(searchtoken->format);
              Flush();
            }
            break;
          case LangDefSpecType::Nomatch:
            if(paint)
              *(actTask->text) += *(itr->ptr);
            break;
          case LangDefSpecType::PairTag:
          case LangDefSpecType::WordTag:
          case LangDefSpecType::LineTag:
            if(paint)
              *(actTask->text) += *(itr->ptr);
            itr->GoChar();
            ParseLine(itr, searchtoken, paint);
            return;
        }
        itr->GoChar();
      }
      if(itr->word->mark)
        CheckMarkup(itr, paint);
      return;
      break;
    case LangDefSpecType::PairTag:
      ptr = searchtoken->pair;
      actTask->format = *(searchtoken->format);
      while(*(itr->ptr) != '\n')
      {
        if(itr->word->mark)
          CheckMarkup(itr, paint);
        if(paint)
          *(actTask->text) += *(itr->ptr);
        if(*(itr->ptr) == *ptr)
          ptr++;
        else
          ptr = searchtoken->pair;
        itr->GoChar();
        if(*ptr == '\0')
        {
          if(paint)
            Flush();
          searchtoken = langdef->GetTree();
          actTask->format = *(searchtoken->format);
          ParseLine(itr, searchtoken, paint);
        }
      }
      if(itr->word->mark)
        CheckMarkup(itr,paint);
      return;
    case LangDefSpecType::WordTag:
      actTask->format = *(searchtoken->format);
      while(langdef->IsAlNum(*(itr->ptr))) //IsAlNum should return true for \n //seems like accidental comment
      {
        if(itr->word->mark)
          CheckMarkup(itr,paint);
        if(paint)
          *(actTask->text) += *(itr->ptr);
        ++itr;      
      }
      if(itr->word->mark)
        CheckMarkup(itr,paint);
      if(paint)
        Flush();
      searchtoken = langdef->GetTree();
      ParseLine(itr, searchtoken, paint);
      return;
    case LangDefSpecType::LineTag:
      actTask->format = *(searchtoken->format);
      while(*(itr->ptr) != '\n')
      {
        if(itr->word->mark)
          CheckMarkup(itr,paint);
        if(paint)
          *(actTask->text) += *(itr->ptr);
        ++itr;      
      }
      if(itr->word->mark)
        CheckMarkup(itr,paint);
      if(paint)
        Flush();
      searchtoken = langdef->GetTree();
      return;
  }
}
//---------------------------------------------------------------------------

void Parser::CheckMarkup(Iter * itr, bool paint)
{
  Mark ** m = &(itr->word->mark);
#ifdef DEBUG
  Write("checking marks");
#endif
  while(*m != NULL)
  {
    if((*m)->pos == itr->offset)
    {
      if(paint)
        Flush();
      if((*m)->begin)
      {

#ifdef DEBUG
  Write(" begin");
#endif
        MarkupPush(&(state.markupStack), (*m)->format);
      }
      else
      {

#ifdef DEBUG
  Write(" end");
#endif
        MarkupPop(&(state.markupStack), (*m)->format);
      }
    }
    m = &((*m)->mark);
  }
#ifdef DEBUG
  Write("leaving marks");
#endif
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
  WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
  drawer->Draw(task);
  ReleaseMutex(drawerQueueMutex);
}
//---------------------------------------------------------------------------
void Parser::SendString()
{
  if(!outTask->text->IsEmpty())
  {
    outTask->newline = newline;
    outTask->linenum = this->linenum;
    WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
    drawer->Draw(outTask);
    ReleaseMutex(drawerQueueMutex);
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
    WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
  drawer->Draw(task);
  ReleaseMutex(drawerQueueMutex);
}
//---------------------------------------------------------------------------
__fastcall Parser::~Parser()
{
}
//---------------------------------------------------------------------------
#ifdef DEBUG
void Parser::Write(AnsiString message)
{
  myfile << message.c_str() << std::endl;
}
#endif
//---------------------------------------------------------------------------


