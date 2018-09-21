
#include <glm/glm.hpp>
#include <iostream>
#include "InputHandler.h"

void InputHandler::reset()
{
    m_lookDir = glm::vec3();
    m_posDir = glm::vec3();
}

void InputHandler::handleKeyInput(int key, int action)
{
    float X_AXIS_MOVEMENT = 0.05f;
    float Y_AXIS_MOVEMENT = 0.05f;
    float Z_AXIS_MOVEMENT = 0.05f;

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        std::cout << "pressed: " << key << std::endl;

        // TODO: I'm sure there is faster math here somehow. 

        if (key == GLFWDirectionKeyMap.UP)
        {
            m_posDir.y += Y_AXIS_MOVEMENT;
        }
        else if (key == GLFWDirectionKeyMap.DOWN)
        {
            m_posDir.y += Y_AXIS_MOVEMENT * -1;
        }
        else if (key == GLFWDirectionKeyMap.FORWARD)
        {
            m_posDir.z += Z_AXIS_MOVEMENT;
        }
        else if (key == GLFWDirectionKeyMap.BACK)
        {
            m_posDir.z += Z_AXIS_MOVEMENT * -1;
        }
        else if (key == GLFWDirectionKeyMap.RIGHT)
        {
            m_posDir.x += X_AXIS_MOVEMENT;
        }
        else if (key == GLFWDirectionKeyMap.LEFT)
        {
            m_posDir.x += X_AXIS_MOVEMENT * -1;
        }
    }

    std::cout << "input direction: ("
        << m_posDir.x << ", "
        << m_posDir.y << ", "
        << m_posDir.z << ")"
        << std::endl;
}