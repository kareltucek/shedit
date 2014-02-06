//---------------------------------------------------------------------------

#ifndef uStackH
#define uStackH
//---------------------------------------------------------------------------
  template <class T_data> class Stack
  {
  public:
    class Node
    {
      T_data data;
      Node * next;
      Node ** prev;

      void Remove();
    };

    Node * top;

    Stack();
    Node* Push(T_data d);
    void Pop();
    void Remove(T_data d);
    void Erase();
    void Contains(T_data d);
    Stack<T_data>& operator=(const Stack<T_data>& stack);
    bool operator==(const Stack<T_data>& stack);
  };
//---------------------------------------------------------------------------
#endif
