#ifndef UTILS_RANDOM_HPP
#define UTILS_RANDOM_HPP

#include "../common/prereqs.hpp"

namespace decisiontree {

/**
 * @brief
*/
class RandomState {
protected:
    // random eigine 
    std::mt19937  engine_;

public:
    // Create and initialize random number generator with the current system time
    RandomState(): engine_(static_cast<unsigned long>(time(nullptr))) {};

    /**
     * Create and initialize random number generator with a seed
     * @param seed
    */
    RandomState(unsigned long  seed): engine_(seed) {};

    /**
     * Provide a double random number from a uniform distribution between [low, high).
     * @param low included lower bound for random number.
     * @param high excluded upper bound for random number.
     * @return random number.
    */
    double uniform_real(double low,
                        double high) {
        std::uniform_real_distribution<double> dist(low, high);
        return dist(engine_);
    };

    /**
     * Provide a long random number from a uniform distribution between [low, high).
     * @param low  included lower bound for random number.
     * @param high excluded upper bound for random number.
     * @return random number.
    */
    long uniform_int(long low,
                     long high) {
        
        std::uniform_int_distribution<long> dist(low, high - 1);
        return dist(engine_);          
    };
};

} //namespace

#endif // UTILS_RANDOM_HPP