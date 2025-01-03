#pragma once

#include <cstdlib>  // for rand()
#include <ctime>    // for time()
#include <vector>
#include <random>

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
    if (min > max)
        throw std::invalid_argument("min must be less than or equal to max.");

    return rand() % (max - min + 1) + min;
}

inline int getWeightedRandomInRange(int min, int max)
{
    if (min > max)
        throw std::invalid_argument("min must be less than or equal to max.");

    // Calculate the range size
    int rangeSize = max - min + 1;

    // Define the base weight and the decay factor
    const float decayFactor = 0.5f; // Adjust this to control the rate of decrease

    // Generate weights using a geometric progression
    std::vector<int> weights(rangeSize);
    weights[0] = 100; // Base weight
    for (int i = 1; i < rangeSize; ++i)
        weights[i] = static_cast<int>(weights[i - 1] * decayFactor);

    // Calculate the total weight
    int totalWeight = 0;
    for (int weight : weights)
        totalWeight += weight;

    // Generate a random number in the range [0, totalWeight)
    std::random_device rd; // Seed
    std::mt19937 gen(rd()); // Random number generator
    std::uniform_int_distribution<> dist(0, totalWeight - 1);

    int randomValue = dist(gen);

    // Determine the index based on the random value
    int cumulativeWeight = 0;
    for (int i = 0; i < weights.size(); ++i)
    {
        cumulativeWeight += weights[i];
        if (randomValue < cumulativeWeight)
            return min + i;
    }

    return -1; // Should never reach here
}