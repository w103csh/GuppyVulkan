#ifndef RANDOM_H
#define RANDOM_H

#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class Random {
   private:
    std::mt19937 generator_;
    std::uniform_real_distribution<float> distr01_;

   public:
    Random() : distr01_(0.0f, 1.0f) {
        std::random_device rd;
        generator_.seed(rd());
    }

    inline float nextFloat() { return distr01_(generator_); }

    template <typename T>
    static void shuffle(std::vector<T>& v) {
        std::random_device rd;
        std::mt19937 g(rd());

        std::shuffle(v.begin(), v.end(), g);
    }

    glm::vec3 uniformHemisphere() {
        glm::vec3 result;
        float x1 = nextFloat(), x2 = nextFloat();
        float s = glm::sqrt(1.0f - x1 * x1);
        result.x = glm::cos(glm::two_pi<float>() * x2) * s;
        result.y = glm::sin(glm::two_pi<float>() * x2) * s;
        result.z = x1;
        return result;
    }

    glm::vec3 uniformCircle() {
        glm::vec3 result(0.0f);
        float x = nextFloat();
        result.x = glm::cos(glm::two_pi<float>() * x);
        result.y = glm::sin(glm::two_pi<float>() * x);
        return result;
    }
};

#endif  // RANDOM_H
