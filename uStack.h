//---------------------------------------------------------------------------

#ifndef uStackH
#define uStackH

/*!
 * Stack 
 * -----
 * Stack (working rather on principle of a forward list, but written for purpose of having a stack) template was specially reimplemented due to need of a small dynamic container, that would consume as little memory as possible (at least when empty :-) ), and anyway so that it's behaviour is always absolutely predictable.
 *
 * Iterating through Stack is meant to be done using the Stack<T_data>::Node* pointer as an iterator. I.e. like:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 *  Stack<int> stack = Stack<int>();
 *  ... fill the stack...
 *
 *  for(Stack<int>::Node * n = stack.top; n != NULL; n = n->next)
 *    if(n->data % 2 == 0)
 *      ...do something with even numbers contained in stack...
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    
 * */

template <class T_data> class Stack
{
  public:
    struct Node
    {
      Node(T_data _data) : data(_data) { };

      T_data data;
      Node * next;
      Node ** prev;

      Node* Remove();                                                         /*!< Returns pointer to the next node, for easier removal of elements while iterating through. Any item of Stack can be removed directly from outside of stack by this. Removed node is directly deleted. */
    };

    Node * top;

    Stack();
    ~Stack();
    Node* Push(const T_data& d);
    void Pop();
    void Erase();
    void Remove(T_data d);
    bool Contains(T_data d);
    Stack<T_data>& operator=(const Stack<T_data>& stack);
    bool operator==(const Stack<T_data>& stack);
};


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
typename Stack<T_data>::Node* Stack<T_data>::Push(const T_data& d)
{
  Node * n = new Node(d);
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
  if(top != NULL)
  {
    Node *n = top;
    top = top->next;
    if(top != NULL)
      top->prev = &top;
    delete n;
  }
}
//---------------------------------------------------------------------------
  template <class T_data>
typename Stack<T_data>::Node* Stack<T_data>::Node::Remove()
{
  Node * ret = next;
  *prev = next;
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
      n = n->next;
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
  top = NULL;
}
//---------------------------------------------------------------------------
template <class T_data>
Stack<T_data>& Stack<T_data>::operator=(const Stack<T_data>& stack)
{
    if(&stack == this)
        return *this;

    Erase();
    Node * n = stack.top;
    Node ** m = &top;
    while(n != NULL)
    {
        *m = new Node(n->data);
        (*m)->next = NULL;
        (*m)->prev = m;

        m = &((*m)->next);
        n = n->next;
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
bool Stack<T_data>::Contains(T_data d)
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
//---------------------------------------------------------------------------


#endif

