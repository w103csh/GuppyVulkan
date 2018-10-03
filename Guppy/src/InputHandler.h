
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

    inline const glm::vec3& getPosDir() { return pos_dir_; }

    inline const glm::vec3& getLookDir() { return look_dir_; }

    inline void addKeyInput(Game::Key key) { curr_key_input_.insert(key); }

    inline void removeKeyInput(Game::Key key) { curr_key_input_.erase(key); }

    void updateKeyInput();
    void reset();

   private:
    InputHandler(MyShell* sh);

    MyShell* sh_;
    std::set<Game::Key> curr_key_input_;
    glm::vec3 look_dir_;

    /*  This is not really a position direction vector. Its a holder for how much
        the object should move in each direction.

        x-component : left & right
        y-component : forward & back
        z-component : up & down
    */
    glm::vec3 pos_dir_;
};

#endif  // !INPUT_HANLDER_H