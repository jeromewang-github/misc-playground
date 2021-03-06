// ============================================================================
// Copyright (c) 2009-2013 Faustino Frechilla
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
/// @file safe_queue.h
/// @brief Definition of a thread-safe queue based on c++11 std calls
/// It internally contains a std::queue which is protected from concurrent
/// access by std mutexes and conditional variables
///
/// Your compiler must have support for c++11. This is an example of how to 
/// compile an application that makes use of this queue with gcc 4.8:
///   $ g++ -g -O0 -Wall -std=c++11 -D_REENTRANT -c app.cpp
///   $ g++ app.o -o app
///
/// @author Faustino Frechilla
/// @history
/// Ref  Who                 When         What
///      Faustino Frechilla 04-May-2009 Original development (based on pthreads)
///      Faustino Frechilla 19-May-2010 Ported to glib. Removed pthread dependency
///      Faustino Frechilla 06-Jun-2013 Ported to c++11. Removed glib dependency
///      Faustino Frechilla 19-Mar-2014 Copy/move constructor, operator= and move assignment
/// @endhistory
///
// ============================================================================

#ifndef _SAFEQUEUE_H_
#define _SAFEQUEUE_H_

#include <queue>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <limits> // std::numeric_limits<>::max

#define SAFE_QUEUE_DEFAULT_MAX_SIZE std::numeric_limits<std::size_t >::max()

/// @brief thread-safe queue
/// It uses a mutex+condition variables to protect the internal queue
/// implementation. Inserting or reading elements use the same mutex
template <typename T>
class SafeQueue
{
public:
    /// @brief constructor
    /// @param a_maxSize optional parameter with the maximum size of the queue
    SafeQueue(std::size_t a_maxSize = SAFE_QUEUE_DEFAULT_MAX_SIZE);
    
    /// @brief destructor
    ~SafeQueue();
    
    /// @brief copy contructor
    /// WARNING: Use with great care, this function call can take a long time
    /// and block other threads from pushing/popping elements into the source
    /// queue
    SafeQueue(const SafeQueue<T>& a_src);
    
    /// @brief operator= overloading
    /// This function blocks the a_src and "this" SafeQueues and copies the
    /// contents of a_src into "this" SafeQueue. 
    /// WARNING: Use with great care, this function call can take a long time
    /// and block other threads from pushing/popping elements into the queues
    /// @param a_src the "right" side of the operator=
    /// @return a const reference to this object
    const SafeQueue<T>& operator=(const SafeQueue<T> &a_src);
    
    /// @brief move contructor
    SafeQueue(SafeQueue<T>&& a_src);
    
    /// @brief move assignment
    SafeQueue<T>& operator=(SafeQueue<T>&& a_src);

    /// @brief Check if the queue is empty
    /// This call can block if another thread owns the lock that protects the
    /// queue
    /// @return true if the queue is empty. False otherwise
    bool IsEmpty() const;

    /// @brief inserts an element into queue queue
    /// This call can block if another thread owns the lock that protects the
    /// queue. If the queue is full The thread will be blocked in this queue
    /// until someone else gets an element from the queue
    /// @param element to insert into the queue
    void Push(const T &a_elem);

    /// @brief inserts an element into queue queue
    /// This call can block if another thread owns the lock that protects the
    /// queue. If the queue is full The call will return false and the element
    /// won't be inserted
    /// @param element to insert into the queue
    /// @return True if the elem was successfully inserted into the queue.
    ///         False otherwise
    bool TryPush(const T &a_elem);

    /// @brief extracts an element from the queue (and deletes it from the q)
    /// If the queue is empty this call will block the thread until there is
    /// something in the queue to be extracted
    /// @param a reference where the element from the queue will be saved to
    void Pop(T &out_data);

    /// @brief extracts an element from the queue (and deletes it from the q)
    /// This call gets the block that protects the queue. It will extract the
    /// element from the queue only if there are elements in it
    /// @param reference to the variable where the result will be saved
    /// @return True if the element was retrieved from the queue.
    ///         False if the queue was empty
    bool TryPop(T &out_data);

    /// @brief extracts an element from the queue (and deletes it from the q)
    /// If the queue is empty this call will block the thread until there
    /// is something in the queue to be extracted or until the timer
    /// (2nd parameter) expires
    /// @param reference to the variable where the result will be saved
    /// @param duration to wait before returning if the queue was empty
    ///        you may also pass into this a std::seconds or std::milliseconds
    ///        (defined in std::chrono)
    /// @return True if the element was retrieved from the queue.
    ///         False if the timeout was hit and nothing could be extracted
    ///         from the queue
    bool TimedWaitPop(T &data, std::chrono::microseconds a_microsecs);

protected:
    /// the actual queue data structure protected by this SafeQueue wrapper
    std::queue<T> m_theQueue;
    /// maximum number of elements for the queue
    std::size_t m_maximumSize;
    /// Mutex to protect the queue
    mutable std::mutex m_mutex;
    /// Conditional variable to wake up threads
    mutable std::condition_variable m_cond;
    
    /// @brief calculate if copying a_src into this instance will need to 
    ///        wake up potential threads waiting to perform push or pop ops.
    /// WARNING: It assumes the caller holds all mutexes required to access
    /// to the data of this and a_src SafeQueues
    /// @param a_src const reference to the SafeQueue that will be copied 
    ///        into this object
    /// @return true if threads will need to be waken up. False otherwise
    inline bool WakeUpSignalNeeded(const SafeQueue<T> &a_src) const;
};

// include the implementation file
#include "safe_queue_impl.h"

#endif /* _SAFEQUEUE_H_ */

