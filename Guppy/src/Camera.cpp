//
//#include <glm/gtc/matrix_transform.hpp>
//
//#include "Camera.h"
//#include "Constants.h"
//
//void Camera::defaultOrthographic(float width, float height)
//{
//    // TODO
//}
//
//void Camera::defaultPerspective(float width, float height)
//{
//    //static auto startTime = std::chrono::high_resolution_clock::now();
//
//    //auto currentTime = std::chrono::high_resolution_clock::now();
//    //float time = std::chrono::duration<float, std::ratio<3, 1>>(currentTime - startTime).count();
//
//    // INITIAL VALUES
//
//    model = glm::mat4(1.0f);
//    //ubo.model = glm::rotate(
//    //    glm::mat4(1.0f),            // matrix
//    //    time * glm::radians(90.0f), // angle 
//    //    glm::vec3(0.0f, 0.0f, 1.0f) // vector
//    //);
//
//    view = glm::lookAt(
//        glm::vec3(0.0f, 2.0f, -2.0f), // eye
//        glm::vec3(0.0f, 0.0f, 0.0f),  // center
//        UP_VECTOR
//    );
//
//    proj = glm::perspective(
//        glm::radians(45.0f),  // fov
//        width / height,       // aspect
//        0.1f,                 // near
//        10.0f                 // far
//    );
//
//    // TODO: Having to invert the y-axis here makes it pretty obvious that this type of transform is
//    //       not the most efficient. I would imagine camera space is inverted in Vulkan for a reason.
//    //proj[1][1] *= -1;
//}
//
//glm::vec3 Camera::getDirection()
//{
//    glm::vec4 out(0.0f, 0.0f, -1.0f, 1.0f);
//    if (UP_VECTOR.z == 1)
//    {
//        // I did not test this.
//        out.z = 0.0f;
//        out.y = 1.0f;
//    }
//    glm::vec3 dir = view * out;
//    return glm::normalize(dir);
//}
//
//glm::vec3 Camera::getPosition()
//{
//    // never looked at this. got it off the web
//    {
//        glm::mat3 mat3(model);
//        glm::vec3 d(model[3]);
//
//        glm::vec3 retVec = -d * mat3;
//        return retVec;
//    }
//}
//
//void Camera::updatePosistion(const glm::vec3& dir)
//{
//    if (!(dir.x == 0 && dir.y == 0 && dir.z == 0))
//    {
//        auto forward = getDirection();
//        auto right = glm::normalize(glm::cross(forward, UP_VECTOR)); // TODO: handle looking straight up
//        auto up = glm::normalize(glm::cross(right, forward));
//        
//        auto d  = forward * dir.z;
//             d += right * dir.x;
//             d += up * dir.y;
//
//        view *= glm::translate(glm::mat4(1.0f), d);
//    }
//}