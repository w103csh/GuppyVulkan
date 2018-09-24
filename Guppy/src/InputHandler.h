//#pragma once
//
//#include <GLFW/glfw3.h>
//#include <glm/glm.hpp>
//
//static struct
//{
//    int UP =      GLFW_KEY_E;
//    int DOWN =    GLFW_KEY_Q;
//    int FORWARD = GLFW_KEY_W;
//    int BACK =    GLFW_KEY_S;
//    int LEFT =    GLFW_KEY_A;
//    int RIGHT =   GLFW_KEY_D;
//} GLFWDirectionKeyMap;
//
//// Singleton
//class InputHandler
//{
//public:
//    static InputHandler& get()
//    {
//        static InputHandler instance;
//        return instance;
//    }
//
//    InputHandler(InputHandler const&) = delete;             // Copy construct
//    InputHandler(InputHandler&&) = delete;                  // Move construct
//    InputHandler& operator=(InputHandler const&) = delete;  // Copy assign
//    InputHandler& operator=(InputHandler &&) = delete;      // Move assign
//
//    void reset();
//    void handleKeyInput(int key, int action);
//
//    inline const glm::vec3& getPosDir()
//    {
//        return m_posDir;
//    }
//
//    inline const glm::vec3& getLookDir()
//    {
//        return m_lookDir;
//    }
//
//
//private:
//    InputHandler() {};
//
//    glm::vec3 m_lookDir;
//    glm::vec3 m_posDir;
//};