#pragma once

#include <cstdlib>  // for rand()
#include <ctime>    // for time()

// Initialize the random seed
inline void initRandom()
{
    static bool initialized = false;
    if (!initialized)
    {
        srand(static_cast<unsigned int>(time(0)));  // Seed the random number generator with the current time
        initialized = true;
    }
}

// Get a random integer within a specified range [min, max]
inline int getRandomInRange(int min, int max)
{
    return rand() % (max - min + 1) + min;
}
