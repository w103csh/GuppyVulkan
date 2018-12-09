
#ifndef INPUT_HANLDER_H
#define INPUT_HANDLER_H

#include <glm/glm.hpp>
#include <set>

#include "Helpers.h"
#include "Singleton.h"

class Shell;

class InputHandler : Singleton {
   public:
    static void init(Shell* sh);

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

    static inline void updateMousePosition(float xPos, float yPos, float zDelta, bool isLooking) {
        inst_.currMouseInput_.xPos = xPos;
        inst_.currMouseInput_.yPos = yPos;
        inst_.currMouseInput_.zDelta = zDelta;
        inst_.isLooking_ = isLooking;
    }

    static inline void mouseLeave() { inst_.hasFocus_ = true; }

    static void updateInput(float elapsed);

   private:
    InputHandler()
        : sh_(nullptr),
          currKeyInput_(),
          posDir_(),
          isLooking_(false),
          hasFocus_(false),
          currMouseInput_{0.0f, 0.0f, 0.0f},
          prevMouseInput_{0.0f, 0.0f, 0.0f},
          lookDir_(){};  // Prevent construction
    ~InputHandler(){};   // Prevent destruction

    static InputHandler inst_;
    void reset() override;

    void updateKeyInput();
    void updateMouseInput();

    Shell* sh_;

    std::set<Game::KEY> currKeyInput_;
    /*  This is not really a position direction vector. Its a holder for how much
        the object should move in each direction.

        x-component : left & right
        y-component : forward & back
        z-component : up & down
    */
    glm::vec3 posDir_;

    struct MouseInput {
        float xPos, yPos, zDelta;
        // std::set<Game::MOUSE> inputs;
        // std::set<Game::MOUSE> type;
    };
    bool isLooking_;
    bool hasFocus_;
    MouseInput currMouseInput_;
    MouseInput prevMouseInput_;
    glm::vec3 lookDir_;
};

#endif  // !INPUT_HANDLER_H