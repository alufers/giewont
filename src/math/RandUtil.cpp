#include "RandUtil.h"
#include <random>
using namespace giewont;

float giewont::rand_float(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}
