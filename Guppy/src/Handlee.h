#ifndef HANDLEE_H
#define HANDLEE_H

template <class T>
class Handlee {
   protected:
    Handlee(T &handler) : handler_(handler) {}
    inline T &handler() { return handler_; }

   private:
    T &handler_;
};

#endif  // !HANDLEE_H