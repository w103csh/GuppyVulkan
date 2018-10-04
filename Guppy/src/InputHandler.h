
#ifndef INPUT_HANLDER_H
#define INPUT_HANDLER_H

//#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <set>

#include "MyShell.h"
#include "Game.h"

// static struct
//{
//    int UP =      GLFW_KEY_E;
//    int DOWN =    GLFW_KEY_Q;
//    int FORWARD = GLFW_KEY_W;
//    int BACK =    GLFW_KEY_S;
//    int LEFT =    GLFW_KEY_A;
//    int RIGHT =   GLFW_KEY_D;
//} GLFWDirectionKeyMap;

#include "Helpers.h"
const bool MY_DEBUG = true;

class InputHandler {
   public:
    static InputHandler& get(MyShell* sh = nullptr) {
        static InputHandler instance(sh);
        return instance;
    }

    InputHandler() = delete;
    InputHandler(InputHandler const&) = delete;             // Copy construct
    InputHandler(InputHandler&&) = delete;                  // Move construct
    InputHandler& operator=(InputHandler const&) = delete;  // Copy assign
    InputHandler& operator=(InputHandler&&) = delete;       // Move assign

    enum class INPUT_TYPE {
        UP,
        DOWN,
        DBLCLK,
    };

    inline const glm::vec3& getPosDir() const { return pos_dir_; }
    inline const glm::vec3& getLookDir() const { return look_dir_; }

    inline void updateKeyInput(Game::KEY key, INPUT_TYPE type) {
        switch (type) {
            case INPUT_TYPE::UP:
                curr_key_input_.erase(key);
                break;
            case INPUT_TYPE::DOWN:
                curr_key_input_.insert(key);
                break;
        }
    }

    inline void updateMousePosition(float xPos, float yPos, bool is_looking) {
        curr_mouse_input_.xPos = xPos;
        curr_mouse_input_.yPos = yPos;
        is_looking_ = is_looking;
    }

    // inline bool hasMouseInput() {
    //    return !helpers::almost_equal(curr_mouse_input_.xPos, 0.0f, 1) && !helpers::almost_equal(curr_mouse_input_.yPos, 0.0f, 1);
    //}

    inline void mouseLeave() { has_focus_ = true; }

    void updateInput();
    void reset();

   private:
    InputHandler(MyShell* sh);

    void updateKeyInput();
    void updateMouseInput();

    MyShell* sh_;

    std::set<Game::KEY> curr_key_input_;
    /*  This is not really a position direction vector. Its a holder for how much
        the object should move in each direction.

        x-component : left & right
        y-component : forward & back
        z-component : up & down
    */
    glm::vec3 pos_dir_;

    struct MouseInput {
        float xPos, yPos;
        // std::set<Game::MOUSE> inputs;
        // std::set<Game::MOUSE> type;
    };
    bool is_looking_;
    bool has_focus_;
    MouseInput curr_mouse_input_;
    MouseInput prev_mouse_input_;
    glm::vec3 look_dir_;
};

#endif  // !INPUT_HANLDER_H