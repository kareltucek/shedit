//---------------------------------------------------------------------------


#pragma hdrstop

//include "uStack.h"

//---------------------------------------------------------------------------
template <class T_data>
Stack<T_data>::Stack()
{
  top = NULL;
}
//---------------------------------------------------------------------------
template <class T_data>
Stack<T_data>::~Stack()
{
  Erase();
}
//---------------------------------------------------------------------------
template <class T_data>
Stack<T_data>::Node* Stack<T_data>::Push(const T_data& d)
{
  Node * n = new Node();
  n->data = d;
  n->next = top;
  n->prev = &top;
  top = n;
  if(n->next != NULL)
    n->next->prev = &(n->next);

  return n;
}
//---------------------------------------------------------------------------
template <class T_data>
void Stack<T_data>::Pop()
{
  Node *n = top;
  top = top->next;
  if(top != NULL)
    top->prev = &top;
  delete n;
}
//---------------------------------------------------------------------------
template <class T_data>
Stack<T_data>::Node* Stack<T_data>::Node::Remove()
{
  Node * ret = next;
  (*prev)->next = next;
  if(next != NULL)
    next->prev = prev;

  delete this;
  return ret;
}
//---------------------------------------------------------------------------
template <class T_data>
void Stack<T_data>::Remove(T_data d)
{
  Node *n = top;
  while(n != NULL)
  {
    if(n->data == d)
      n = n->Remove();
    else
      n = (*n)->next;
  }
}
//---------------------------------------------------------------------------
template <class T_data>
void Stack<T_data>::Erase()
{
  Node *n = top;
  Node *m = top;
  while(n != NULL)
  {
    n = n->next;
    delete m;
    m = n;
  }
}
//---------------------------------------------------------------------------
template <class T_data>
Stack<T_data>& Stack<T_data>::operator=(const Stack<T_data>& stack)
{
  if(&stack == this)
    return *this;

  Erase();
  Node * n = stack.top;
  while(n != NULL)
  {
    Push(n->data);
  }
  return *this;
}
//---------------------------------------------------------------------------
template <class T_data>
bool Stack<T_data>::operator==(const Stack<T_data>& stack)
{
  Node * n = top;
  Node * m = stack.top;
  while(n != NULL && m != NULL)
  {
    if(n->data != m->data)
      return false;
    n = n->next;
    m = m->next;
  }
  return n == m;
}
//---------------------------------------------------------------------------
template <class T_data>
void Stack<T_data>::Contains(T_data d)
{
  Node *m = top;
  while(m != NULL)
  {
    if(m->data == d)
      return true;
    m = m->next;
  }
  return false;
}

#pragma package(smart_init)
