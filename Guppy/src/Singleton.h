#ifndef SINGLETON_H
#define SINGLETON_H

template <class T>
class Singleton {
   public:
    Singleton(Singleton const &) = delete;             // Copy construct
    Singleton(Singleton &&) = delete;                  // Move construct
    Singleton &operator=(Singleton const &) = delete;  // Copy assign
    Singleton &operator=(Singleton &&) = delete;       // Move assign

    static T &inst() {
        if (pInst_ == nullptr) pInst_ = new T;
        return *pInst_;
    }

   protected:
    Singleton() = default;   // Prevent construction
    ~Singleton() = default;  // Prevent destruction

   private:
    static inline T *pInst_ = nullptr;
};

#endif  // !SINGLETON_H