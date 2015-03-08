//---------------------------------------------------------------------------
#pragma hdrstop


#include "uLanguageDefinition2.h"
#include "uFormat.h"
#include <string>
#include <stdio.h>

using namespace SHEdit;

//---------------------------------------------------------------------------
LanguageDefinition::LanguageDefinition(const std::locale& l = std::locale()) : loc(l), tokenizer(), caseSensitive(true), ids(1)
{
}
//---------------------------------------------------------------------------
LanguageDefinition::~LanguageDefinition()
{
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddTerm(const std::wstring& name,FontStyle* fs,const std::wstring& rgx, int id, bool caseSens, bool getst, bool confirm)
{
  Term* t = new Term( name,fs, rgx,  i == Auto ? ++ids : id,  getst, confirm);
  tokenizer.AddToken(rgx,id,caseSens);
  terms.push_back(t);
  index[name] = Rec(t);
}
//---------------------------------------------------------------------------
void LanguageDefinition::AddNonTerm(const std::wstring& name, FontStyle* fs, int id, bool gather, bool call)
{
  NTerm* t = new NTerm(name, fs, id == Auto ? ++ids : id, gather, call);
  nonterms.push_back(t);
  index[name] = Rec(t);
}
//---------------------------------------------------------------------------
void LanguageDefinition::Finalize()
{
  tokenizer.AddTokensSubmit();
  for(std::set<Node*>::iterator itr = nodes.begin(); itr != nodes.end(); ++itr)
  {
    itr->Finalize();
  }
  nodes.clear();
}
//---------------------------------------------------------------------------
void LanguageDefinition::Node::ExpandLambda(std::map<int, Node*>& index, Node* next)
{
  switch(type)
  {
    case ntTerm:
      next = next == NULL ? this : next;
      std::map<int,Node*>::iterator it = index.find(t->tokid);
      if(it == index.end())
      {
        //ok
        index[t->tokid] = next;
      }
      else
      {
        if(it->second != next)
          throw std::wstring("lambda closure conflict on : ").append(r.GetName()).c_str();
      }
      break;
    case ntNTerm: break;
      next = next == NULL ? this : next;
      r.nt->node->ExpandLambda(recidx, next); //first nonterminal or nonlambda node will take place
    case ntLambda: 
      for(std::vector<Node*>::iterator itr = nextnodes.begin(); itr != nextnodes.end(); ++itr)
        itr->ExpandLambda(index, next);
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
        itr->ExpandLambda(lftidx, *itr);
      break;
    case ntNTerm: break;
      for(std::vector<Node*>::iterator itr = nextnodes.begin(); itr != nextnodes.end(); ++itr)
        itr->ExpandLambda(lftidx, *itr);
      r.nt->node->ExpandLambda(recidx, NULL); //first nonterminal or nonlambda node will take place
    case ntLambda: 
    case ntEnd: 

           break;
    case ntTailrec: 
                   /*TODO in case we want tail recursion support*/
           break;
  }
}
//---------------------------------------------------------------------------
LanguageDefinition::NTerm::NTerm(const std::wstring& n, FontStyle* f, int i, bool g, bool c) : ruleid(i), name(n), fs(f), gather(g), call(c), node(new Node(ntLambda)) 
{
}
//---------------------------------------------------------------------------
LanguageDefinition::Term::Term(const std::wstring& n, FontStyle* fs_, int i, bool g, bool c) : name(n), fs(fs_), id(i), getstyle(g), call(c) {} ;
//---------------------------------------------------------------------------
void LanguageDefinition::AddRule(const std::wstring& name, std::wstring rule)
{
  std::vector<Node*> endnodes;
  NTerm* nt = index[name].nt; 
  if(nt == NULL)
    throw std::runtime_error( "non existing non terminal encountered" );
  endnodes.push_back(nt);
  Construct(rule, endnodes);
  for(std::vector<Node*>::iterator itr = endnodes.begin(); itr != endnodes.end(); ++itr)
    itr->Add(Node(ntEnd));
}
//---------------------------------------------------------------------------
void LanguageDefinition::Construct(std::wstring& rule, std::vector<Node*>& endnodes)
{
  std::wstring buff;
  std::vector<Node*> begins(endnodes);
  std::vector<Node*> ends();

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
        std::vector<Node*> tmp(begins);
        ParseString(rule, tmp);
        ends.insert(e.end(), tmp.begin(), tmp.end());
        laststring = true;
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
        Rec r = index(buff);
        Node n;
        if(r.term) //(two different constructors)
          n = Node(r.t);
        else
          n = Node(r.nt);

        for(std::vector<Node*>::iterator itr = endnodes.begin(); itr != endnodes.end(); ++itr)
        {
          *itr = itr->Add(n);
          nodes.insert(*itr);
        }

        GetToken(rule, buff, true);
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
  throw std::wstring( "comparing two Nodes by == not allowed! Use Eq or Cq" ).c_str();
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
    if(itr->Eq(n))
    {
      if( ! itr->Cq(n))
        throw std::wstring("equal but not congruent nodes conflict: ").append(r.GetName()).c_str();
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
      str = str.substring(1);
      return tC;
    default:
      int i = 0;
      while(str.length() > i && str[i] != ' ' && str[i] != '|')
        ++i;
      val = str.substring(0,i);
      str = str.substring(i);
      return i > 0 ? tS : tE;
  }
}




#pragma package(smart_init)
