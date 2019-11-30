/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef HANDLEE_H
#define HANDLEE_H

template <class T>
class Handlee {
   protected:
    Handlee(T &handler) : handler_(handler) {}
    inline T &handler() const { return handler_; }

   private:
    T &handler_;
};

#endif  // !HANDLEE_H