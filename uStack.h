//---------------------------------------------------------------------------

#ifndef uStackH
#define uStackH
//---------------------------------------------------------------------------
  template <class T_data> class Stack
  {
  public:
    class Node
    {
    public:

      T_data data;
      Node * next;
      Node ** prev;

      Node* Remove();
    };

    Node * top;

    Stack();
    ~Stack();
    Node* Push(const T_data& d);
    void Pop();
    void Erase();
    void Contains(T_data d);
    void Remove(T_data d);
    Stack<T_data>& operator=(const Stack<T_data>& stack);
    bool operator==(const Stack<T_data>& stack);
  };
//---------------------------------------------------------------------------

#include "uStack.cpp"

#endif
