#ifndef RANDOM_H
#define RANDOM_H

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <random>

#include "Singleton.h"

class Random : public Singleton<Random> {
    friend class Singleton<Random>;

   public:
    inline float nextFloatZeroToOne() { return distr_0_1_(mt19937_generator_); }

    template <typename T>
    static void shuffle(std::vector<T>& v) {
        std::random_device rd;
        std::mt19937 g(rd());

        std::shuffle(v.begin(), v.end(), g);
    }

    glm::vec3 uniformHemisphere() {
        glm::vec3 result;
        float x1 = nextFloatZeroToOne(), x2 = nextFloatZeroToOne();
        float s = glm::sqrt(1.0f - x1 * x1);
        result.x = glm::cos(glm::two_pi<float>() * x2) * s;
        result.y = glm::sin(glm::two_pi<float>() * x2) * s;
        result.z = x1;
        return result;
    }

    glm::vec3 uniformSphere() {
        glm::vec3 result;
        float theta = nextFloatZeroToOne() * glm::two_pi<float>();
        float phi = nextFloatZeroToOne() * glm::two_pi<float>();
        result.x = cos(theta) * sin(phi);
        result.y = sin(theta) * sin(phi);
        result.z = cos(phi);
        return result;
    }

    glm::vec3 uniformCircle() {
        glm::vec3 result(0.0f);
        float x = nextFloatZeroToOne();
        result.x = glm::cos(glm::two_pi<float>() * x);
        result.y = glm::sin(glm::two_pi<float>() * x);
        return result;
    }

    // The book called this "jitter".
    float nextFloatNegZeroPoint5ToPosZeroPoint5() { return distr_neg05_pos05_(dre_generator_); }

   private:
    Random() : distr_0_1_(0.0f, 1.0f), distr_neg05_pos05_(-0.5f, 0.5f) {
        std::random_device rd;
        mt19937_generator_.seed(rd());
    }
    ~Random() = default;

    // TODO: make these static in the next functions, and then make
    // the class a namespace, or singleton.
    std::mt19937 mt19937_generator_;
    std::uniform_real_distribution<float> distr_0_1_;

    std::default_random_engine dre_generator_;
    std::uniform_real_distribution<float> distr_neg05_pos05_;
};

#endif  // RANDOM_H
