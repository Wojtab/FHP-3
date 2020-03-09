#include "simulation.h"
#include <bitset>
#include <thread>

const std::array<std::array<uint8_t, 256>, 2> Simulation::collisionLUT = Simulation::generateCollisionLUT();

Simulation::Simulation(int gridWidth, int gridHeight, int numThreads, const std::function<void(Simulation*)>& initialConditionsGenerator) :
    m_gridWidth(gridWidth),
    m_gridHeight(gridHeight),
    m_numThreads(numThreads),
    m_grid()
{
    std::vector<uint8_t> temp;
    temp.assign(m_gridWidth, 0);
    m_grid.assign(m_gridHeight, temp);

    std::random_device rd;
    m_randGen = std::mt19937(rd());

    initialConditionsGenerator(this);
}

int Simulation::countColOccuppancy(int at) const
{
    int count = 0;
    for (const auto& row : m_grid)
    {
        count += std::bitset<8>(row[at] & 0b01111111).count();
    }
    return count;
}

void Simulation::spawnAtX(float concentration, int at, int width)
{
    int occupancy = 0;
    for (int i = at; i < at + width; i++)
    {
        occupancy = countColOccuppancy(i);
        int wallcount = 0;
        for(int y = 0; y < m_gridHeight; y++)
        {
            wallcount += (m_grid[y][i] & 0b10000000) >> 7;
        }
        // 7 possible particles
        if (occupancy >= (m_gridHeight - wallcount) * 7 * concentration) return;

        int toSpawn = (m_gridHeight - wallcount) * 7 * concentration - occupancy;

        std::uniform_int_distribution<> yDist(0, m_gridHeight - 1);
        std::uniform_int_distribution<> dirDist(0, 6);

        while (toSpawn > 0)
        {
            int y = yDist(m_randGen);
            int dir = dirDist(m_randGen);
            if (m_grid[y][i] & (1 << dir) || m_grid[y][i] & 0b10000000) continue;
            m_grid[y][i] |= 1 << dir;
            toSpawn--;
        }
    }
}

std::vector<std::vector<uint8_t>>& Simulation::getGrid()
{
    return m_grid;
}

std::pair<int, int> Simulation::getRegionVelocity(int fromX, int toX, int fromY, int toY)
{
    int vx = 0;
    int vy = 0;
    /*
    1: -1/2, sqrt(3)/2
    2: 1/2, sqrt(3)/2
    3: 1, 0
    4: 1/2, -sqrt(3)/2
    5: -1/2, -sqrt(3)/2
    6: -1, 0
    */
    for (int y = fromY; y < toY; y++)
    {
        for (int x = fromX; x < toX; x++)
        {
            vx = vx - (m_grid[y][x] & 0b00000001) - ((m_grid[y][x] & 0b00010000) >> 4) // -1/2
                + ((m_grid[y][x] & 0b00000010) >> 1) + ((m_grid[y][x] & 0b00001000) >> 3) // 1/2
                + ((m_grid[y][x] & 0b00000100) >> 1) - ((m_grid[y][x] & 0b00100000) >> 4); // +/- 1 (moved to 2nd place)
            vy = vy + (m_grid[y][x] & 0b00000001) + ((m_grid[y][x] & 0b00000010) >> 1) // sqrt(3)/2
                - ((m_grid[y][x] & 0b00001000) >> 3) - ((m_grid[y][x] & 0b00010000) >> 4); // -sqrt(3)/2
        }
    }

    return std::pair<int, int>(vx, vy);
}

std::pair<double, double> Simulation::getRegionAverageVelocity(int fromX, int toX, int fromY, int toY)
{
    int vx = 0;
    int vy = 0;
    int count = 0;

    for (int y = fromY; y < toY; y++)
    {
        for (int x = fromX; x < toX; x++)
        {
            vx = vx - (m_grid[y][x] & 0b00000001) - ((m_grid[y][x] & 0b00010000) >> 4) // -1/2
                + ((m_grid[y][x] & 0b00000010) >> 1) + ((m_grid[y][x] & 0b00001000) >> 3) // 1/2
                + ((m_grid[y][x] & 0b00000100) >> 1) - ((m_grid[y][x] & 0b00100000) >> 4); // +/- 1 (moved to 2nd place)
            vy = vy + (m_grid[y][x] & 0b00000001) + ((m_grid[y][x] & 0b00000010) >> 1) // sqrt(3)/2
                - ((m_grid[y][x] & 0b00001000) >> 3) - ((m_grid[y][x] & 0b00010000) >> 4); // -sqrt(3)/2
            count += std::bitset<8>(m_grid[y][x] & 0b01111111).count();
        }
    }
    return { count != 0 ? (double(vx) / 2.) / count : 0, count != 0 ? (double(vy) * 0.8660254) / count : 0 };
}

std::vector<std::vector<std::pair<double, double>>> Simulation::getVelocityField(int cellSizeX, int cellSizeY)
{
    if (m_gridHeight % cellSizeY != 0 || m_gridWidth % cellSizeX != 0) throw "Simulation::getVelocityField grid size non-divisible by cellSize";

    std::vector<std::vector<std::pair<double, double>>> velField;

    for (int y = 0; y < m_gridHeight / cellSizeY; y++)
    {
        velField.push_back(std::vector<std::pair<double, double>>());
        for (int x = 0; x < m_gridWidth / cellSizeX; x++)
        {
            velField[y].push_back(getRegionAverageVelocity(cellSizeX * x, cellSizeX * (x + 1), cellSizeY * y, cellSizeY * (y + 1)));
        }
    }

    return velField;
}

std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> Simulation::getVelocityMagnitudeAndDensityField(int cellSizeX, int cellSizeY)
{
    std::vector<std::vector<double>> vmField;
    std::vector<std::vector<double>> densityField;
    std::vector<double> t;
    t.assign(m_gridWidth/cellSizeX, 0.);
    vmField.assign(m_gridHeight/cellSizeY, t);
    densityField.assign(m_gridHeight/cellSizeY, t);

    runThreaded([&](int threadNo)
    {
        int n = m_gridHeight / cellSizeY / m_numThreads;
        for (int i = n*threadNo; i < n*(threadNo+1); i++)
        {
            for (int j = 0; j < m_gridWidth / cellSizeX; j++)
            {
                int vx = 0;
                int vy = 0;
                int count = 0;

                for (int y = cellSizeY * i; y < cellSizeY * (i+1); y++)
                {
                    for (int x = cellSizeX * j; x < cellSizeX * (j+1); x++)
                    {
                        vx = vx - (m_grid[y][x] & 0b00000001) - ((m_grid[y][x] & 0b00010000) >> 4) // -1/2
                            + ((m_grid[y][x] & 0b00000010) >> 1) + ((m_grid[y][x] & 0b00001000) >> 3) // 1/2
                            + ((m_grid[y][x] & 0b00000100) >> 1) - ((m_grid[y][x] & 0b00100000) >> 4); // +/- 1 (moved to 2nd place)
                        vy = vy + (m_grid[y][x] & 0b00000001) + ((m_grid[y][x] & 0b00000010) >> 1) // sqrt(3)/2
                            - ((m_grid[y][x] & 0b00001000) >> 3) - ((m_grid[y][x] & 0b00010000) >> 4); // -sqrt(3)/2
                        count += std::bitset<8>(m_grid[y][x] & 0b01111111).count();
                    }
                }
                if(count != 0)
                    vmField[i][j] = sqrt(pow((double(vx) / 2.) / count, 2) +  pow((double(vy) * 0.8660254) / count, 2));
                else
                    vmField[i][j] = 0;
                densityField[i][j] = double(count) / double(cellSizeX*cellSizeY*7);
            }
        }
    });
    return {vmField, densityField};
}

std::pair<std::vector<std::vector<std::pair<double, double> > >, std::vector<std::vector<double> > > Simulation::getVelocityAndDensityField(int cellSizeX, int cellSizeY)
{
    std::vector<std::vector<std::pair<double, double>>> vField;
    std::vector<std::vector<double>> densityField;
    std::vector<double> t;
    t.assign(m_gridWidth/cellSizeX, 0.);
    densityField.assign(m_gridHeight/cellSizeY, t);
    std::vector<std::pair<double, double>> t2;
    t2.assign(m_gridWidth/cellSizeX, {0., 0.});
    vField.assign(m_gridHeight/cellSizeY, t2);

    runThreaded([&](int threadNo)
    {
        int n = m_gridHeight / cellSizeY / m_numThreads;
        for (int i = n*threadNo; i < n*(threadNo+1); i++)
        {
            for (int j = 0; j < m_gridWidth / cellSizeX; j++)
            {
                int vx = 0;
                int vy = 0;
                int count = 0;

                for (int y = cellSizeY * i; y < cellSizeY * (i+1); y++)
                {
                    for (int x = cellSizeX * j; x < cellSizeX * (j+1); x++)
                    {
                        vx = vx - (m_grid[y][x] & 0b00000001) - ((m_grid[y][x] & 0b00010000) >> 4) // -1/2
                            + ((m_grid[y][x] & 0b00000010) >> 1) + ((m_grid[y][x] & 0b00001000) >> 3) // 1/2
                            + ((m_grid[y][x] & 0b00000100) >> 1) - ((m_grid[y][x] & 0b00100000) >> 4); // +/- 1 (moved to 2nd place)
                        vy = vy + (m_grid[y][x] & 0b00000001) + ((m_grid[y][x] & 0b00000010) >> 1) // sqrt(3)/2
                            - ((m_grid[y][x] & 0b00001000) >> 3) - ((m_grid[y][x] & 0b00010000) >> 4); // -sqrt(3)/2
                        count += std::bitset<8>(m_grid[y][x] & 0b01111111).count();
                    }
                }
                vField[i][j] = { count != 0 ? (double(vx) / 2.) / count : 0, count != 0 ? (double(vy) * 0.8660254) / count : 0 };
                densityField[i][j] = double(count) / double(cellSizeX*cellSizeY*7);
            }
        }
    });
    return {vField, densityField};
}

void Simulation::moveStep()
{
    std::vector<std::vector<uint8_t>> tempGrid;
    std::vector<uint8_t> temp;
    temp.assign(m_gridWidth, 0);
    tempGrid.assign(m_gridHeight, temp);

    runThreaded([this, &tempGrid](int i) {
        int to = i != m_numThreads - 1 ? (m_gridHeight / m_numThreads) * (i + 1) : m_gridHeight;
        for (int y = (m_gridHeight / m_numThreads) * i; y < to; y++)
        {
            moveRow(y, tempGrid);
        }
    });

    m_grid.swap(tempGrid);
}

void Simulation::colissionStep()
{
    std::uniform_int_distribution<> which(0, 1);

    runThreaded([this, &which](int i) {
        int to = i != m_numThreads - 1 ? (m_gridHeight / m_numThreads) * (i + 1) : m_gridHeight;
        for (int y = (m_gridHeight / m_numThreads) * i; y < to; y++)
        {
            for (auto& value : m_grid[y])
            {
                // don't generate random if not needed, it is way faster
                value = collisionLUT[collisionLUT[0][value] == collisionLUT[1][value] ? 0 : which(m_randGen)][value];
            }
        }
    });
}

/* optimized to eliminate almost all bound checks(it's faster)
we are looking at the incoming, instead of outgoing so that 1 run only acceses 1 row,
so it can be parallelised
basically it does:

if (y % 2 == 0)
    for (int x = 0; x < m_gridWidth; x++)
        tempGrid[y][x] = m_grid[y][x] & 0b11000000;
        tempGrid[y][x] |= m_grid[y + 1][x + 1] & 0b00000001;
        tempGrid[y][x] |= m_grid[y + 1][x] & 0b00000010;
        tempGrid[y][x] |= m_grid[y][x - 1] & 0b00000100;
        tempGrid[y][x] |= m_grid[y - 1][x] & 0b00001000;
        tempGrid[y][x] |= m_grid[y - 1][x + 1] & 0b00010000;
        tempGrid[y][x] |= m_grid[y][x + 1] & 0b00100000;
else
    for (int x = 0; x < m_gridWidth; x++)
        tempGrid[y][x] = m_grid[y][x] & 0b11000000;
        tempGrid[y][x] |= m_grid[y + 1][x] & 0b00000001;
        tempGrid[y][x] |= m_grid[y + 1][x - 1] & 0b00000010;
        tempGrid[y][x] |= m_grid[y][x - 1] & 0b00000100;
        tempGrid[y][x] |= m_grid[y - 1][x - 1] & 0b00001000;
        tempGrid[y][x] |= m_grid[y - 1][x] & 0b00010000;
        tempGrid[y][x] |= m_grid[y][x + 1] & 0b00100000;

+ bounds checking
*/
inline void Simulation::moveRow(int y, std::vector<std::vector<uint8_t>>& tempGrid)
{
    if (y % 2 == 0)
    {
        for (int x = 1; x < m_gridWidth - 1; x++)
        {
            tempGrid[y][x] = m_grid[y][x] & 0b11000000;
            tempGrid[y][x] |= m_grid[y][x - 1] & 0b00000100;
            tempGrid[y][x] |= m_grid[y][x + 1] & 0b00100000;
        }
        tempGrid[y][0] = m_grid[y][0] & 0b11000000;
        tempGrid[y][0] |= m_grid[y][1] & 0b00100000;
        tempGrid[y][m_gridWidth - 1] = m_grid[y][m_gridWidth - 1] & 0b11000000;
        tempGrid[y][m_gridWidth - 1] |= m_grid[y][m_gridWidth - 2] & 0b00000100;
        if (y - 1 >= 0)
        {
            for (int x = 0; x < m_gridWidth - 1; x++)
            {
                tempGrid[y][x] |= m_grid[y - 1][x] & 0b00001000;
                tempGrid[y][x] |= m_grid[y - 1][x + 1] & 0b00010000;
            }
            tempGrid[y][m_gridWidth - 1] |= m_grid[y - 1][m_gridWidth - 1] & 0b00001000;
        }
        if (y + 1 < m_gridHeight)
        {
            for (int x = 0; x < m_gridWidth - 1; x++)
            {
                tempGrid[y][x] |= m_grid[y + 1][x + 1] & 0b00000001;
                tempGrid[y][x] |= m_grid[y + 1][x] & 0b00000010;
            }
            tempGrid[y][m_gridWidth - 1] |= m_grid[y + 1][m_gridWidth - 1] & 0b00000010;
        }
    }
    else
    {
        for (int x = 1; x < m_gridWidth - 1; x++)
        {
            tempGrid[y][x] = m_grid[y][x] & 0b11000000;
            tempGrid[y][x] |= m_grid[y][x - 1] & 0b00000100;
            tempGrid[y][x] |= m_grid[y][x + 1] & 0b00100000;
        }
        tempGrid[y][0] = m_grid[y][0] & 0b11000000;
        tempGrid[y][0] |= m_grid[y][1] & 0b00100000;
        tempGrid[y][m_gridWidth - 1] = m_grid[y][m_gridWidth - 1] & 0b11000000;
        tempGrid[y][m_gridWidth - 1] |= m_grid[y][m_gridWidth - 2] & 0b00000100;
        if (y - 1 >= 0)
        {
            for (int x = 1; x < m_gridWidth; x++)
            {
                tempGrid[y][x] |= m_grid[y - 1][x - 1] & 0b00001000;
                tempGrid[y][x] |= m_grid[y - 1][x] & 0b00010000;
            }
            tempGrid[y][0] |= m_grid[y - 1][0] & 0b00010000;
        }
        if (y + 1 < m_gridHeight)
        {
            for (int x = 1; x < m_gridWidth; x++)
            {
                tempGrid[y][x] |= m_grid[y + 1][x] & 0b00000001;
                tempGrid[y][x] |= m_grid[y + 1][x - 1] & 0b00000010;
            }
            tempGrid[y][0] |= m_grid[y + 1][0] & 0b00000001;
        }
    }
}

void Simulation::runThreaded(std::function<void(int threadNumber)> fn)
{
    std::vector<std::thread> threads;
    for (int i = 0; i < m_numThreads; i++)
    {
        threads.push_back(std::thread(fn, i));
    }
    for (auto& t : threads) t.join();
}

constexpr std::array<std::array<uint8_t, 256>, 2> Simulation::generateCollisionLUT()
{
    std::array<std::array<uint8_t, 256>, 2> table = std::array<std::array<uint8_t, 256>, 2>();
    table[0][0b00000000] = 0b00000000;
    table[0][0b00000001] = 0b00000001;
    table[0][0b00000010] = 0b00000010;
    table[0][0b00000011] = 0b00000011;
    table[0][0b00000100] = 0b00000100;
    table[0][0b00000101] = 0b01000010;
    table[0][0b00000110] = 0b00000110;
    table[0][0b00000111] = 0b00000111;
    table[0][0b00001000] = 0b00001000;
    table[0][0b00001001] = 0b00100100;
    table[0][0b00001010] = 0b01000100;
    table[0][0b00001011] = 0b00100110;
    table[0][0b00001100] = 0b00001100;
    table[0][0b00001101] = 0b01001010;
    table[0][0b00001110] = 0b00001110;
    table[0][0b00001111] = 0b00001111;
    table[0][0b00010000] = 0b00010000;
    table[0][0b00010001] = 0b01100000;
    table[0][0b00010010] = 0b00001001;
    table[0][0b00010011] = 0b01100010;
    table[0][0b00010100] = 0b01001000;
    table[0][0b00010101] = 0b00101010;
    table[0][0b00010110] = 0b00001101;
    table[0][0b00010111] = 0b01100110;
    table[0][0b00011000] = 0b00011000;
    table[0][0b00011001] = 0b00110100;
    table[0][0b00011010] = 0b01010100;
    table[0][0b00011011] = 0b00101101;
    table[0][0b00011100] = 0b00011100;
    table[0][0b00011101] = 0b01011010;
    table[0][0b00011110] = 0b00011110;
    table[0][0b00011111] = 0b01101110;
    table[0][0b00100000] = 0b00100000;
    table[0][0b00100001] = 0b00100001;
    table[0][0b00100010] = 0b01000001;
    table[0][0b00100011] = 0b00100011;
    table[0][0b00100100] = 0b00010010;
    table[0][0b00100101] = 0b00010011;
    table[0][0b00100110] = 0b01000101;
    table[0][0b00100111] = 0b00100111;
    table[0][0b00101000] = 0b01010000;
    table[0][0b00101001] = 0b01010001;
    table[0][0b00101010] = 0b00010101;
    table[0][0b00101011] = 0b01010011;
    table[0][0b00101100] = 0b00011010;
    table[0][0b00101101] = 0b00110110;
    table[0][0b00101110] = 0b01001101;
    table[0][0b00101111] = 0b01010111;
    table[0][0b00110000] = 0b00110000;
    table[0][0b00110001] = 0b00110001;
    table[0][0b00110010] = 0b00101001;
    table[0][0b00110011] = 0b00110011;
    table[0][0b00110100] = 0b01101000;
    table[0][0b00110101] = 0b01101001;
    table[0][0b00110110] = 0b00011011;
    table[0][0b00110111] = 0b01101011;
    table[0][0b00111000] = 0b00111000;
    table[0][0b00111001] = 0b00111001;
    table[0][0b00111010] = 0b01110100;
    table[0][0b00111011] = 0b01110101;
    table[0][0b00111100] = 0b00111100;
    table[0][0b00111101] = 0b01111010;
    table[0][0b00111110] = 0b01011101;
    table[0][0b00111111] = 0b00111111;
    table[0][0b01000000] = 0b01000000;
    table[0][0b01000001] = 0b00100010;
    table[0][0b01000010] = 0b00000101;
    table[0][0b01000011] = 0b01000011;
    table[0][0b01000100] = 0b00001010;
    table[0][0b01000101] = 0b00001011;
    table[0][0b01000110] = 0b01000110;
    table[0][0b01000111] = 0b01000111;
    table[0][0b01001000] = 0b00010100;
    table[0][0b01001001] = 0b01100100;
    table[0][0b01001010] = 0b00010110;
    table[0][0b01001011] = 0b00010111;
    table[0][0b01001100] = 0b01001100;
    table[0][0b01001101] = 0b01010110;
    table[0][0b01001110] = 0b01001110;
    table[0][0b01001111] = 0b01001111;
    table[0][0b01010000] = 0b00101000;
    table[0][0b01010001] = 0b00110010;
    table[0][0b01010010] = 0b01001001;
    table[0][0b01010011] = 0b01100101;
    table[0][0b01010100] = 0b00101100;
    table[0][0b01010101] = 0b01101010;
    table[0][0b01010110] = 0b00101110;
    table[0][0b01010111] = 0b00101111;
    table[0][0b01011000] = 0b01011000;
    table[0][0b01011001] = 0b00111010;
    table[0][0b01011010] = 0b01101100;
    table[0][0b01011011] = 0b01101101;
    table[0][0b01011100] = 0b01011100;
    table[0][0b01011101] = 0b00111110;
    table[0][0b01011110] = 0b01011110;
    table[0][0b01011111] = 0b01011111;
    table[0][0b01100000] = 0b00010001;
    table[0][0b01100001] = 0b01100001;
    table[0][0b01100010] = 0b00100101;
    table[0][0b01100011] = 0b01100011;
    table[0][0b01100100] = 0b01010010;
    table[0][0b01100101] = 0b00101011;
    table[0][0b01100110] = 0b01001011;
    table[0][0b01100111] = 0b01100111;
    table[0][0b01101000] = 0b00011001;
    table[0][0b01101001] = 0b01110010;
    table[0][0b01101010] = 0b01010101;
    table[0][0b01101011] = 0b00110111;
    table[0][0b01101100] = 0b00011101;
    table[0][0b01101101] = 0b01110110;
    table[0][0b01101110] = 0b00011111;
    table[0][0b01101111] = 0b01101111;
    table[0][0b01110000] = 0b01110000;
    table[0][0b01110001] = 0b01110001;
    table[0][0b01110010] = 0b00110101;
    table[0][0b01110011] = 0b01110011;
    table[0][0b01110100] = 0b01011001;
    table[0][0b01110101] = 0b00111011;
    table[0][0b01110110] = 0b01011011;
    table[0][0b01110111] = 0b01110111;
    table[0][0b01111000] = 0b01111000;
    table[0][0b01111001] = 0b01111001;
    table[0][0b01111010] = 0b00111101;
    table[0][0b01111011] = 0b01111011;
    table[0][0b01111100] = 0b01111100;
    table[0][0b01111101] = 0b01111101;
    table[0][0b01111110] = 0b01111110;
    table[0][0b01111111] = 0b01111111;
    table[0][0b10000000] = 0b10000000;
    table[0][0b10000001] = 0b10001000;
    table[0][0b10000010] = 0b10010000;
    table[0][0b10000011] = 0b10011000;
    table[0][0b10000100] = 0b10100000;
    table[0][0b10000101] = 0b10101000;
    table[0][0b10000110] = 0b10110000;
    table[0][0b10000111] = 0b10111000;
    table[0][0b10001000] = 0b10000001;
    table[0][0b10001001] = 0b10001001;
    table[0][0b10001010] = 0b10010001;
    table[0][0b10001011] = 0b10011001;
    table[0][0b10001100] = 0b10100001;
    table[0][0b10001101] = 0b10101001;
    table[0][0b10001110] = 0b10110001;
    table[0][0b10001111] = 0b10111001;
    table[0][0b10010000] = 0b10000010;
    table[0][0b10010001] = 0b10001010;
    table[0][0b10010010] = 0b10010010;
    table[0][0b10010011] = 0b10011010;
    table[0][0b10010100] = 0b10100010;
    table[0][0b10010101] = 0b10101010;
    table[0][0b10010110] = 0b10110010;
    table[0][0b10010111] = 0b10111010;
    table[0][0b10011000] = 0b10000011;
    table[0][0b10011001] = 0b10001011;
    table[0][0b10011010] = 0b10010011;
    table[0][0b10011011] = 0b10011011;
    table[0][0b10011100] = 0b10100011;
    table[0][0b10011101] = 0b10101011;
    table[0][0b10011110] = 0b10110011;
    table[0][0b10011111] = 0b10111011;
    table[0][0b10100000] = 0b10000100;
    table[0][0b10100001] = 0b10001100;
    table[0][0b10100010] = 0b10010100;
    table[0][0b10100011] = 0b10011100;
    table[0][0b10100100] = 0b10100100;
    table[0][0b10100101] = 0b10101100;
    table[0][0b10100110] = 0b10110100;
    table[0][0b10100111] = 0b10111100;
    table[0][0b10101000] = 0b10000101;
    table[0][0b10101001] = 0b10001101;
    table[0][0b10101010] = 0b10010101;
    table[0][0b10101011] = 0b10011101;
    table[0][0b10101100] = 0b10100101;
    table[0][0b10101101] = 0b10101101;
    table[0][0b10101110] = 0b10110101;
    table[0][0b10101111] = 0b10111101;
    table[0][0b10110000] = 0b10000110;
    table[0][0b10110001] = 0b10001110;
    table[0][0b10110010] = 0b10010110;
    table[0][0b10110011] = 0b10011110;
    table[0][0b10110100] = 0b10100110;
    table[0][0b10110101] = 0b10101110;
    table[0][0b10110110] = 0b10110110;
    table[0][0b10110111] = 0b10111110;
    table[0][0b10111000] = 0b10000111;
    table[0][0b10111001] = 0b10001111;
    table[0][0b10111010] = 0b10010111;
    table[0][0b10111011] = 0b10011111;
    table[0][0b10111100] = 0b10100111;
    table[0][0b10111101] = 0b10101111;
    table[0][0b10111110] = 0b10110111;
    table[0][0b10111111] = 0b10111111;
    table[0][0b11000000] = 0b11000000;
    table[0][0b11000001] = 0b11001000;
    table[0][0b11000010] = 0b11010000;
    table[0][0b11000011] = 0b11011000;
    table[0][0b11000100] = 0b11100000;
    table[0][0b11000101] = 0b11101000;
    table[0][0b11000110] = 0b11110000;
    table[0][0b11000111] = 0b11111000;
    table[0][0b11001000] = 0b11000001;
    table[0][0b11001001] = 0b11001001;
    table[0][0b11001010] = 0b11010001;
    table[0][0b11001011] = 0b11011001;
    table[0][0b11001100] = 0b11100001;
    table[0][0b11001101] = 0b11101001;
    table[0][0b11001110] = 0b11110001;
    table[0][0b11001111] = 0b11111001;
    table[0][0b11010000] = 0b11000010;
    table[0][0b11010001] = 0b11001010;
    table[0][0b11010010] = 0b11010010;
    table[0][0b11010011] = 0b11011010;
    table[0][0b11010100] = 0b11100010;
    table[0][0b11010101] = 0b11101010;
    table[0][0b11010110] = 0b11110010;
    table[0][0b11010111] = 0b11111010;
    table[0][0b11011000] = 0b11000011;
    table[0][0b11011001] = 0b11001011;
    table[0][0b11011010] = 0b11010011;
    table[0][0b11011011] = 0b11011011;
    table[0][0b11011100] = 0b11100011;
    table[0][0b11011101] = 0b11101011;
    table[0][0b11011110] = 0b11110011;
    table[0][0b11011111] = 0b11111011;
    table[0][0b11100000] = 0b11000100;
    table[0][0b11100001] = 0b11001100;
    table[0][0b11100010] = 0b11010100;
    table[0][0b11100011] = 0b11011100;
    table[0][0b11100100] = 0b11100100;
    table[0][0b11100101] = 0b11101100;
    table[0][0b11100110] = 0b11110100;
    table[0][0b11100111] = 0b11111100;
    table[0][0b11101000] = 0b11000101;
    table[0][0b11101001] = 0b11001101;
    table[0][0b11101010] = 0b11010101;
    table[0][0b11101011] = 0b11011101;
    table[0][0b11101100] = 0b11100101;
    table[0][0b11101101] = 0b11101101;
    table[0][0b11101110] = 0b11110101;
    table[0][0b11101111] = 0b11111101;
    table[0][0b11110000] = 0b11000110;
    table[0][0b11110001] = 0b11001110;
    table[0][0b11110010] = 0b11010110;
    table[0][0b11110011] = 0b11011110;
    table[0][0b11110100] = 0b11100110;
    table[0][0b11110101] = 0b11101110;
    table[0][0b11110110] = 0b11110110;
    table[0][0b11110111] = 0b11111110;
    table[0][0b11111000] = 0b11000111;
    table[0][0b11111001] = 0b11001111;
    table[0][0b11111010] = 0b11010111;
    table[0][0b11111011] = 0b11011111;
    table[0][0b11111100] = 0b11100111;
    table[0][0b11111101] = 0b11101111;
    table[0][0b11111110] = 0b11110111;
    table[0][0b11111111] = 0b11111111;

    table[1] = table[0];

    table[1][0b00001001] = 0b00010010;
    table[1][0b00001011] = 0b01000101;
    table[1][0b00001101] = 0b00010110;
    table[1][0b00010010] = 0b00100100;
    table[1][0b00010011] = 0b00100101;
    table[1][0b00010110] = 0b01001010;
    table[1][0b00010111] = 0b01001011;
    table[1][0b00011001] = 0b01101000;
    table[1][0b00011010] = 0b00101100;
    table[1][0b00011011] = 0b00110110;
    table[1][0b00011101] = 0b01101100;
    table[1][0b00100100] = 0b00001001;
    table[1][0b00100101] = 0b01100010;
    table[1][0b00100110] = 0b00001011;
    table[1][0b00101001] = 0b00110010;
    table[1][0b00101011] = 0b01100101;
    table[1][0b00101100] = 0b01010100;
    table[1][0b00101101] = 0b00011011;
    table[1][0b00101110] = 0b01010110;
    table[1][0b00110010] = 0b01010001;
    table[1][0b00110100] = 0b00011001;
    table[1][0b00110101] = 0b01110010;
    table[1][0b00110110] = 0b00101101;
    table[1][0b00111010] = 0b01011001;
    table[1][0b01000101] = 0b00100110;
    table[1][0b01001001] = 0b01010010;
    table[1][0b01001010] = 0b00001101;
    table[1][0b01001011] = 0b01100110;
    table[1][0b01001101] = 0b00101110;
    table[1][0b01010001] = 0b00101001;
    table[1][0b01010010] = 0b01100100;
    table[1][0b01010011] = 0b00101011;
    table[1][0b01010100] = 0b00011010;
    table[1][0b01010110] = 0b01001101;
    table[1][0b01011001] = 0b01110100;
    table[1][0b01011010] = 0b00011101;
    table[1][0b01011011] = 0b01110110;
    table[1][0b01100010] = 0b00010011;
    table[1][0b01100100] = 0b01001001;
    table[1][0b01100101] = 0b01010011;
    table[1][0b01100110] = 0b00010111;
    table[1][0b01101000] = 0b00110100;
    table[1][0b01101001] = 0b00110101;
    table[1][0b01101100] = 0b01011010;
    table[1][0b01101101] = 0b01011011;
    table[1][0b01110010] = 0b01101001;
    table[1][0b01110100] = 0b00111010;
    table[1][0b01110110] = 0b01101101;

    return table;
}
