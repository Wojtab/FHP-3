#ifndef SIMULATION_H
#define SIMULATION_H
#include <array>
#include <functional>
#include <random>
#include <vector>

class Simulation
{
public:
    Simulation(int gridWidth, int gridHeight, int numThreads, const std::function<void(Simulation*)>& initialConditionsGenerator);

    // counts number of particles in a column
    int countColOccuppancy(int at) const;

    // spawns particles at columns [at...at+width] until desired concentration[0...1] is reached
    void spawnAtX(float concentration, int at, int width);

    std::vector<std::vector<uint8_t>>& getGrid();

    // returns pair(total x velocity(multiple of 1/2), total y velocity(multiple of sqrt(3)/2))
    std::pair<int, int> getRegionVelocity(int fromX, int toX, int fromY, int toY);

    std::pair<double, double> getRegionAverageVelocity(int fromX, int toX, int fromY, int toY);

    // returns average velocity in cellSize x cellSize grid
    std::vector<std::vector<std::pair<double, double>>> getVelocityField(int cellSizeX, int cellSizeY);

    std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> getVelocityMagnitudeAndDensityField(int cellSizeX, int cellSizeY);

    std::pair<std::vector<std::vector<std::pair<double, double>>>, std::vector<std::vector<double>>> getVelocityAndDensityField(int cellSizeX, int cellSizeY);

    void moveStep();

    void colissionStep();

    inline static std::pair<double, double> asNormalVelocity(const std::pair<int, int>& v, double normFactorX = 1, double normFactorY = 1)
    {
        return { (double(v.first) / 2)/normFactorX, (double(v.second) * 0.8660254)/normFactorY };
    }

    static const std::array<std::array<uint8_t, 256>, 2> collisionLUT;
    static constexpr std::array<std::array<uint8_t, 256>, 2> generateCollisionLUT();

private:
    int m_gridWidth;
    int m_gridHeight;
    int m_numThreads;
    std::vector<std::vector<uint8_t>> m_grid;
    std::mt19937 m_randGen;

    // update single row
    void moveRow(int y, std::vector<std::vector<uint8_t>>& tempGrid);

    // runs m_numThreads instances of function fn with parameter threadNumber [0...m_numThreads-1]
    void runThreaded(std::function<void(int threadNumber)> fn);
};

#endif // SIMULATION_H
