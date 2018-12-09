#ifndef SINGLETON_H
#define SINGLETON_H

#include "Shell.h"

class Singleton {
   public:
    Singleton(Singleton const &) = delete;             // Copy construct
    Singleton(Singleton &&) = delete;                  // Move construct
    Singleton &operator=(Singleton const &) = delete;  // Copy assign
    Singleton &operator=(Singleton &&) = delete;       // Move assign

   protected:
    Singleton(){};   // Prevent construction
    ~Singleton(){};  // Prevent destruction
    
    virtual void reset() = 0;
};

#endif  // !SINGLETON_H