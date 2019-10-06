
#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <glm/glm.hpp>
#include <set>

#include "Singleton.h"
#include "Shell.h"

class InputHandler : public Singleton<InputHandler> {
    friend class Singleton<InputHandler>;

   public:
    void init(Shell* sh);

    constexpr const glm::vec3& getPosDir() { return posDir_; }
    constexpr const glm::vec3& getLookDir() { return lookDir_; }

    inline void updateKeyInput(GAME_KEY key, INPUT_ACTION type) {
        switch (type) {
            case INPUT_ACTION::UP:
                currKeyInput_.erase(key);
                break;
            case INPUT_ACTION::DOWN:
                currKeyInput_.insert(key);
                break;
            default:;
        }
    }

    inline void updateMousePosition(float xPos, float yPos, float zDelta, bool isLooking, bool move = false,
                                    bool primary = false, bool secondary = false) {
        currMouseInput_.xPos = xPos;
        currMouseInput_.yPos = yPos;
        currMouseInput_.zDelta = zDelta;
        currMouseInput_.moving = move;
        currMouseInput_.primary = primary;
        currMouseInput_.secondary = secondary;
        isLooking_ = isLooking;
    }

    constexpr void mouseLeave() { hasFocus_ = true; }
    void updateInput(float elapsed);
    void clear() { reset(); }

    // TODO: all this stuff is garbage
    constexpr const MouseInput& getMouseInput() const { return currMouseInput_; }

   private:
    InputHandler()
        : sh_(nullptr),
          currKeyInput_(),
          posDir_(),
          isLooking_(false),
          hasFocus_(false),
          currMouseInput_{0.0f, 0.0f, 0.0f},
          prevMouseInput_{0.0f, 0.0f, 0.0f},
          lookDir_() {}         // Prevent construction
    ~InputHandler() = default;  // Prevent destruction

    void reset();

    void updateKeyInput();
    void updateMouseInput();

    Shell* sh_;

    std::set<GAME_KEY> currKeyInput_;
    /*  This is not really a position direction vector. Its a holder for how much
        the object should move in each direction.

        x-component : left & right
        y-component : forward & back
        z-component : up & down
    */
    glm::vec3 posDir_;

    bool isLooking_;
    bool hasFocus_;
    MouseInput currMouseInput_;
    MouseInput prevMouseInput_;
    glm::vec3 lookDir_;
};

#endif  // !INPUT_HANDLER_H