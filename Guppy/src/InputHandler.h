
#ifndef INPUT_HANLDER_H
#define INPUT_HANDLER_H

#include <glm/glm.hpp>
#include <set>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Singleton.h"
#include "Shell.h"

class InputHandler : public Singleton {
   public:
    static void init(Shell* sh);

    static inline const glm::vec3& getPosDir() { return inst_.posDir_; }
    static inline const glm::vec3& getLookDir() { return inst_.lookDir_; }

    static inline void updateKeyInput(GAME_KEY key, INPUT_TYPE type) {
        switch (type) {
            case INPUT_TYPE::UP:
                inst_.currKeyInput_.erase(key);
                break;
            case INPUT_TYPE::DOWN:
                inst_.currKeyInput_.insert(key);
                break;
        }
    }

    static inline void updateMousePosition(float xPos, float yPos, float zDelta, bool isLooking, bool move = false,
                                           bool primary = false, bool secondary = false) {
        inst_.currMouseInput_.xPos = xPos;
        inst_.currMouseInput_.yPos = yPos;
        inst_.currMouseInput_.zDelta = zDelta;
        inst_.currMouseInput_.moving = move;
        inst_.currMouseInput_.primary = primary;
        inst_.currMouseInput_.secondary = secondary;
        inst_.isLooking_ = isLooking;
    }

    static inline void mouseLeave() { inst_.hasFocus_ = true; }
    static void updateInput(float elapsed);
    static void clear() { inst_.reset(); }

    // TODO: all this stuff is garbage
    static const MouseInput& getMouseInput() { return inst_.currMouseInput_; }

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