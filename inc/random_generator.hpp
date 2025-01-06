#pragma once

#include <random>
#include <stdexcept>
#include <vector>

class RandomGenerator
{
public:
    // Delete copy constructor and assignment operator to enforce singleton
    RandomGenerator(const RandomGenerator&) = delete;
    RandomGenerator& operator=(const RandomGenerator&) = delete;

    // Access the singleton instance
    static RandomGenerator& GetInstance()
    {
        static RandomGenerator instance; // Guaranteed to be lazy-initialized and thread-safe
        return instance;
    }

    // Set a seed for the random number generator
    void SetSeed(unsigned int seed)
    {
        generator.seed(seed);
    }

    // Get a random integer within a specified range [min, max]
    int GetRandomInRange(int min, int max)
    {
        if (min > max)
            throw std::invalid_argument("min must be less than or equal to max.");
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(generator);
    }

    // Get a weighted random integer within a specified range [min, max]
    int GetWeightedRandomInRange(int min, int max)
    {
        if (min > max)
            throw std::invalid_argument("min must be less than or equal to max.");

        // Calculate the range size
        int rangeSize = max - min + 1;

        // Define the base weight and the decay factor
        const float decayFactor = 0.25f; // Adjust this to control the rate of decrease

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
        std::uniform_int_distribution<int> distribution(0, totalWeight - 1);
        int randomValue = distribution(generator);

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

private:
    // Private constructor to enforce singleton
    RandomGenerator()
        : generator(std::random_device{}())
    {}

    std::mt19937 generator; // Mersenne Twister random number generator
};