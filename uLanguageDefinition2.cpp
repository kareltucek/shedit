//---------------------------------------------------------------------------
#pragma hdrstop


#include "uLanguageDefinition2.h"
#include <string>
#include <stdio.h>
#include <stdexcept>

class FontStyle;
using namespace SHEdit;

//---------------------------------------------------------------------------
LanguageDefinition::LanguageDefinition(const std::locale& l) : loc(l), tokenizer(), ids(1)
{
  NTerm * t = new NTerm(L"", NULL, 0, false, false);
  nonterms.push_back(t);
  global = new Node(t);
  nodes.insert(global);
  entering = NULL;
  
}
//---------------------------------------------------------------------------
LanguageDefinition::~LanguageDefinition()
{
  delete global;
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddTerm(const std::wstring& name,FontStyle* fs,const std::wstring& rgx, int id, int flags)
{
  Term* t = new Term( name,fs, id == Auto ? ++ids : id,  flags & tfGetStyle , flags & tfCall );
  tokenizer.AddToken(rgx,id, flags & tfCaseSens);
  terms.push_back(t);
  index[name] = Rec(t);

  if(flags & tfGlobal)
    nodes.insert(global->Add(Node(t)));
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddNonTerm(const std::wstring& name, FontStyle* fs, int id, int flags)
{
  NTerm* t = new NTerm(name, fs, id == Auto ? ++ids : id, flags & tfGather, flags & tfCall);
  nonterms.push_back(t);
  index[name] = Rec(t);

  if(flags & tfGlobal)
    nodes.insert(global->Add(Node(t)));
  if(flags & tfEntering)
  {
    entering = new Node(t);
    nodes.insert(entering);
    nodes.insert(t->node);
  }
}
//---------------------------------------------------------------------------
void LanguageDefinition::Finalize()
{
  tokenizer.AddTokensSubmit();
  for(std::set<Node*>::iterator itr = nodes.begin(); itr != nodes.end(); ++itr)
  {
    (*itr)->Finalize();
  }
  nodes.clear();
}
//---------------------------------------------------------------------------
void LanguageDefinition::Node::ExpandLambda(std::map<int, Node*>& index, Node* next)
{
  switch(type)
  {
    case ntTerm:
      {
      next = next == NULL ? this : next;
      std::map<int,Node*>::iterator it = index.find(r.t->tokid);
      if(it == index.end())
      {
        //ok
        index[r.t->tokid] = next;
      }
      else
      {
        if(it->second != next)
          throw std::wstring(L"lambda closure conflict on : ").append(r.GetName()).c_str();
      }
      }
      break;
    case ntNTerm: 
      next = next == NULL ? this : next;
      r.nt->node->ExpandLambda(recidx, next); //first nonterminal or nonlambda node will take place
      break;
    case ntLambda: 
      for(std::vector<Node*>::iterator itr = nextnodes.begin(); itr != nextnodes.end(); ++itr)
        (*itr)->ExpandLambda(index, next);
    case ntEnd: 
           break;
    case ntTailrec: 
                   /*TODO in case we want tail recursion support*/
           break;
  }
}
//---------------------------------------------------------------------------
void LanguageDefinition::Node::Finalize()
{
  switch(type)
  {
    case ntTerm:
      for(std::vector<Node*>::iterator itr = nextnodes.begin(); itr != nextnodes.end(); ++itr)
        (*itr)->ExpandLambda(lftidx, *itr);
      break;
    case ntNTerm: 
      for(std::vector<Node*>::iterator itr = nextnodes.begin(); itr != nextnodes.end(); ++itr)
        (*itr)->ExpandLambda(lftidx, *itr);
      r.nt->node->ExpandLambda(recidx, NULL); //first nonterminal or nonlambda node will take place
      break;
    case ntLambda: 
    case ntEnd: 

           break;
    case ntTailrec: 
                   /*TODO in case we want tail recursion support*/
           break;
  }
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddRule(const std::wstring& name, std::wstring rule)
{
  std::vector<Node*> endnodes;
  NTerm* nt = index[name].nt; 
  if(nt == NULL)
    throw std::runtime_error( "non existing non terminal encountered" );
  endnodes.push_back(nt->node);
  Construct(rule, endnodes);
  //for(std::vector<Node*>::iterator itr = endnodes.begin(); itr != endnodes.end(); ++itr)
  //  itr->Add(Node(ntEnd));
}
//---------------------------------------------------------------------------
void LanguageDefinition::Construct(std::wstring& rule, std::vector<Node*>& endnodes)
{
  std::wstring buff;
  std::vector<Node*> begins(endnodes);
  std::vector<Node*> ends;

  bool laststring = false;
  bool cont_emp = false;
  while(true)
  {
    TokType t = GetToken(rule, buff, false);
    switch(t)
    {
      case tC:
      case tE:
        if(!laststring)
          cont_emp = true;
        laststring = false;
        if(t == tE)
        {
          if(cont_emp)
            endnodes.insert(endnodes.end(), ends.begin(), ends.end());
          else
            endnodes == ends;
          return;
        }
        else
        {
          GetToken(rule, buff, true);
        }
      default:
        {
        std::vector<Node*> tmp(begins);
        ParseString(rule, tmp);
        ends.insert(ends.end(), tmp.begin(), tmp.end());
        laststring = true;
        }
        break;
    }
  }
}
//---------------------------------------------------------------------------
void LanguageDefinition::ParseString(std::wstring& rule, std::vector<Node*>& endnodes)
{
  std::wstring buff;
  while(true)
  {
    TokType t = GetToken(rule, buff, false);
    switch(t)
    {
      case tS:
        {
        Rec r = index[buff];
        Node n;
        if(r.term) //(two different constructors)
          n = Node(r.t);
        else
          n = Node(r.nt);

        for(std::vector<Node*>::iterator itr = endnodes.begin(); itr != endnodes.end(); ++itr)
        {
          *itr = (*itr)->Add(n);
          nodes.insert(*itr);
        }

        GetToken(rule, buff, true);
    }
        break;
      case tE:
      case tC:
        return;
    }
  }
}
//---------------------------------------------------------------------------
bool LanguageDefinition::Node::operator==(const Node& n)
{
  throw std::wstring( L"comparing two Nodes by == not allowed! Use Eq or Cq" ).c_str();
}
//---------------------------------------------------------------------------
bool LanguageDefinition::Node::Eq(const Node& n)
{
  if( type != n.type )
    return false;
  return r.term ? r.t->Eq (*n.r.t) : r.nt->Eq( *n.r.nt);
}
//---------------------------------------------------------------------------
bool LanguageDefinition::Node::Cq(const Node& n)
{
  if( type != n.type )
    return false;
  return r.term ? r.t->Cq (*n.r.t) : r.nt->Cq( *n.r.nt);
}
//---------------------------------------------------------------------------
LanguageDefinition::Node* LanguageDefinition::Node::Add(const Node& n)
{
  for(std::vector<Node*>::iterator itr = nextnodes.begin(); itr != nextnodes.end(); ++itr)
  {
    if((*itr)->Eq(n))
    {
      if( ! (*itr)->Cq(n))
        throw std::wstring(L"equal but not congruent nodes conflict: ").append(r.GetName()).c_str();
      return *itr;
    }
  }
  nextnodes.push_back(new Node(n));
  return nextnodes.back();

}
//---------------------------------------------------------------------------
LanguageDefinition::TokType LanguageDefinition::GetToken(std::wstring& str, std::wstring& val, bool eat)
{
begin:
  if(str.length() == 0)
    return tE;

  switch(str[0])
  {
    case ' ':
      goto begin;
    case '|':
      str = str.substr(1);
      return tC;
    default:
      int i = 0;
      while(str.length() > i && str[i] != ' ' && str[i] != '|')
        ++i;
      val = str.substr(0,i);
      str = str.substr(i);
      return i > 0 ? tS : tE;
  }
}

//---------------------------------------------------------------------------
template<class IT>
void LanguageDefinition::Parse(IT& from, const IT& to, PState& s, bool& stylechanged, FontStyle*&fs)
{
  if(s.st.empty())
    s.st.push(StackItem(entering->r.nt->node));

  IT a(from);
  int tokid;
  tokenizer.NextToken(from, to, tokid);

  bool goingup = false;

  while(true)
  {
    std::map<int, Node*>& ref = s.st.top().ptr->type == ntNTerm && !goingup ? s.st.top().ptr->recidx: s.st.top().ptr->lftidx;
    std::map<int, Node*>::iterator itr = ref.search(tokid);
    if(itr == ref.end())
    {
      goingup = true;
      Pop(s);

      if(s.st.empty())
      {
        s.st.push(StackItem(entering->r.nt->node));
        return;
      }
      else
      {
        continue;
      }
    }
    else
    {
      if(itr->second->type == ntNTerm)
      {
        Push(s, itr->second);
        if(itr->second->nt->fs != NULL)
          stylechanged = true;
        continue;
      }
      else
      {
        s.st.top().ptr = itr->second;
        if(itr->second->n->getstyle)
          //...
        return;
      }
    }
  }   
}



#pragma package(smart_init)
