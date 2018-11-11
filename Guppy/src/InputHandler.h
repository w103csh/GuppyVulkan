
#ifndef INPUT_HANLDER_H
#define INPUT_HANDLER_H

#include <glm/glm.hpp>
#include <set>

#include "Helpers.h"
#include "Singleton.h"

class MyShell;

class InputHandler : Singleton {
   public:
    static void init(MyShell* ctx);

    enum class INPUT_TYPE {
        UP,
        DOWN,
        DBLCLK,
    };

    static inline const glm::vec3& getPosDir() { return inst_.posDir_; }
    static inline const glm::vec3& getLookDir() { return inst_.lookDir_; }

    static inline void updateKeyInput(Game::KEY key, INPUT_TYPE type) {
        switch (type) {
            case INPUT_TYPE::UP:
                inst_.currKeyInput_.erase(key);
                break;
            case INPUT_TYPE::DOWN:
                inst_.currKeyInput_.insert(key);
                break;
        }
    }

    static inline void updateMousePosition(float xPos, float yPos, bool is_looking) {
        inst_.currMouseInput_.xPos = xPos;
        inst_.currMouseInput_.yPos = yPos;
        inst_.isLooking_ = is_looking;
    }

    // inline bool hasMouseInput() {
    //    return !helpers::almost_equal(curr_mouse_input_.xPos, 0.0f, 1) && !helpers::almost_equal(curr_mouse_input_.yPos, 0.0f, 1);
    //}

    static inline void mouseLeave() { inst_.hasFocus_ = true; }

    static void updateInput(float elapsed);

   private:
    InputHandler(){};   // Prevent construction
    ~InputHandler(){};  // Prevent destruction
    
    static InputHandler inst_;
    void reset() override;

    void updateKeyInput();
    void updateMouseInput();

    MyShell* sh_;

    std::set<Game::KEY> currKeyInput_;
    /*  This is not really a position direction vector. Its a holder for how much
        the object should move in each direction.

        x-component : left & right
        y-component : forward & back
        z-component : up & down
    */
    glm::vec3 posDir_;

    struct MouseInput {
        float xPos, yPos;
        // std::set<Game::MOUSE> inputs;
        // std::set<Game::MOUSE> type;
    };
    bool isLooking_;
    bool hasFocus_;
    MouseInput currMouseInput_;
    MouseInput prevMouseInput_;
    glm::vec3 lookDir_;
};

#endif  // !INPUT_HANLDER_H