///////////////////////////////////////////////////////////////////////////////
///
/// Author: Andrea Ellinger
/// Copyright 2017, DigiPen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef LockFreeQueue_H
#define LockFreeQueue_H

namespace Audio
{
#define Type32Bit s32

  //-------------------------------------------------------------------------------- Lock Free Queue

  template <typename T>
  class LockFreeQueue
  {
  private:
    struct Node
    {
      Node(T value) : Value(value), Next(nullptr) {}

      T Value;
      Node* Next;
    };

  public:
    // Create the dummy node
    LockFreeQueue() { First = Last = new Node(T()); }

    ~LockFreeQueue()
    {
      // Delete anything remaining on the queue
      while (First)
      {
        Node* temp = First;
        First = temp->Next;
        delete temp;
      }
    }

    void Write(const T& object)
    {
      // Create the new object
      Last->Next = new Node(object);
      // Set the Last pointer
      Zero::AtomicStore((void**)&Last, Last->Next);
    }

    bool Read(T& result)
    {
      // Check if there is anything on the queue
      if (!Zero::AtomicCompareExchangeBool((void**)&First, (void*)Last, (void*)Last))
      {
        // Store the pointers
        Node* firstNode = First;
        Node* nextNode = First->Next;
        // Get the value from the next node
        result = nextNode->Value;
        // Move the First pointer forward
        First = First->Next;
        // Delete the old First node
        delete firstNode;

        return true;
      }

      return false;
    }

  private:
    // Only touched by reader
    Node* First;
    // Shared by writer and reader
    Node* Last;
  };

  //-------------------------------------------------------------------------- Multiple Writer Queue

  template <typename T>
  class MultipleWriterQueue
  {
  private:
    struct Node
    {
      Node(T value) : Value(value), Next(nullptr) {}

      T Value;
      Node* Next;
    };

  public:
    // Create the dummy node
    MultipleWriterQueue() : WriteLock(0) { First = Last = new Node(T()); }

    ~MultipleWriterQueue()
    {
      // Delete anything remaining on the queue
      while (First)
      {
        Node* temp = First;
        First = temp->Next;
        delete temp;
      }
    }

    void Write(const T& object)
    {
      // Create the new object
      Node* temp = new Node(object);
      // Get access (wait until WriteLock is 0 and then set it to 1)
      while (Zero::AtomicCompareExchangeBool(&WriteLock, 1, 0))
      { }
      // Set pointer on last object
      Last->Next = temp;
      // Set the Last pointer (shared with reader)
      Zero::AtomicStore((void**)&Last, temp);
      // Release access
      Zero::AtomicStore(&WriteLock, 0);
    }

    bool Read(T& result)
    {
      // Check if there is anything on the queue
      if (!Zero::AtomicCompareExchangeBool((void**)&First, (void*)Last, (void*)Last))
      {
        // Store the pointers
        Node* firstNode = First;
        Node* nextNode = First->Next;
        // Get the value from the next node
        result = nextNode->Value;
        // Move the First pointer forward
        First = First->Next;
        // Delete the old First node
        delete firstNode;

        return true;
      }

      return false;
    }

  private:
    // Only touched by reader
    Node* First;
    // Shared by writers and reader
    Node* Last;
    // Shared by writers
    Type32Bit WriteLock;
  };

}

#endif