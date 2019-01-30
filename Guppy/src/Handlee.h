#ifndef HANDLEE_H
#define HANDLEE_H

template <class T>
class Handlee {
   protected:
    Handlee(const T &handler) : handler_(handler) {}
    const T &handler_;
};

#endif  // !HANDLEE_H