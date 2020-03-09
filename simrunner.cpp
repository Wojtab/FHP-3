#include "simrunner.h"
#include <fstream>
#include <QDebug>
#include "simulation.h"

SimRunner::SimRunner(QObject *parent) : QObject(parent)
{

}

std::vector<std::vector<double> > SimRunner::density()
{
    std::scoped_lock l(m_densityMutex);
    return m_density;
}

std::vector<std::vector<double> > SimRunner::velMagnitude()
{
    std::scoped_lock l(m_velMagnitudeMutex);
    return m_velMagnitude;
}

SimRunner::~SimRunner()
{
    if(m_simThread.joinable())
    {
        m_stopThread = true;
        m_simThread.join();
    }
}

void SimRunner::start()
{
    if(m_simThread.joinable())
    {
        m_stopThread = true;
        m_simThread.join();
    }
    m_stopThread = false;
    m_simThread = std::thread([this](){plate(4000, 1000, 50, 100000, 400, 700);});
}

void SimRunner::stop()
{
    m_stopThread = true;
}

std::pair<double, double> SimRunner::average(const std::pair<double, double> &prev, int sampleCount, const std::pair<double, double> &sample)
{
    return { (prev.first * sampleCount + sample.first) / (sampleCount + 1), (prev.second * sampleCount + sample.second) / (sampleCount + 1) };
}

std::vector<std::vector<std::pair<double, double> > > SimRunner::average(const std::vector<std::vector<std::pair<double, double> > > &prev, int sampleCount, const std::vector<std::vector<std::pair<double, double> > > &sample)
{
    std::vector<std::vector<std::pair<double, double>>> avg;
    avg.reserve(prev.size());

    for (int i = 0; i < prev.size(); i++)
    {
        avg.push_back(std::vector<std::pair<double, double>>());
        avg[i].reserve(prev[i].size());

        for (int j = 0; j < prev[i].size(); j++)
        {
            avg[i].push_back(average(prev[i][j], sampleCount, sample[i][j]));
        }
    }
    return avg;
}

std::vector<std::vector<double> > SimRunner::average(const std::vector<std::vector<double> > &prev, int sampleCount, const std::vector<std::vector<double> > &sample)
{
    std::vector<std::vector<double>> avg;
    avg.reserve(prev.size());

    for (int i = 0; i < prev.size(); i++)
    {
        avg.push_back(std::vector<double>());
        avg[i].reserve(prev[i].size());

        for (int j = 0; j < prev[i].size(); j++)
        {
            avg[i].push_back((prev[i][j] * sampleCount + sample[i][j]) / (sampleCount + 1));
        }
    }
    return avg;
}

void SimRunner::saveVelToFile(std::string name, const std::vector<std::vector<std::pair<double, double> > > &velField)
{
    std::ofstream f(name, std::ios::out);
    for (int y = 0; y < velField.size(); y++)
    {
        for (int x = 0; x < velField[y].size(); x++)
        {
            double magnitude = sqrt(velField[y][x].first * velField[y][x].first + velField[y][x].second * velField[y][x].second);
            f << x << "\t" << y << "\t" << velField[y][x].first / magnitude << "\t" << velField[y][x].second / magnitude << "\t" << magnitude << "\n";
        }
    }
    f.close();
}

/* to run just the solver for Poisseule flow:
Simulation sim = Simulation(w, h, 10, [&](Simulation* sim)
{
    sim->getGrid()[0].assign(w, 0b10000000); // top wall
    sim->getGrid()[h - 1].assign(w, 0b10000000); // bottom wall
    sim->spawnAtX(0.2, 0, w); // fill with particles
    sim->spawnAtX(0.4, 0, reserveWidth); // left side constant density
}
for (int i = 0; i < steps; i++)
{
    sim.moveStep(); // move step
    sim.colissionStep(); // collision step
    sim.spawnAtX(0.4, 0, reserveWidth); // left side constant density
    sim.spawnAtX(0.2, w - reserveWidth, reserveWidth); // right side constant density
}

everything else is just thread management or data generation/saving/averaging etc.
 */

void SimRunner::plate(int w, int h, int reserveWidth, int steps, int barrierHeight, int barrierPos)
{
    int imageSampleWH = 4;

    Simulation sim = Simulation(w, h, 10, [&](Simulation* sim)
    {
        sim->getGrid()[0].assign(w, 0b10000000);
        sim->getGrid()[h - 1].assign(w, 0b10000000);
        sim->spawnAtX(0.2, 0, w);
        sim->spawnAtX(0.4, 0, reserveWidth);
        // "porous" media
        /*for(int i = 0; i < 1000; i++)
        {
            int posx = rand() % (w-10);
            int posy = rand() % (h-10);
            for(int y = posy; y < posy+10; y++)
            {
                for(int x = posx; x < posx+10; x++)
                {
                    sim->getGrid()[y][x] = 0b10000000;
                }
            }
        }*/
        // circle
        /*for (int i = h / 2 - barrierHeight / 2; i < h / 2 + barrierHeight / 2; i++)
        {
            for(int j = barrierPos - barrierHeight / 2; j < barrierPos + barrierHeight / 2; j++)
            {
                if((i-h/2)*(i-h/2) + (j - barrierPos)*(j - barrierPos) < barrierHeight * barrierHeight/4)
                    sim->getGrid()[i][j] = 0b10000000;
            }
        }*/
        // block
        for (int i = h / 2 - barrierHeight / 2; i < h / 2 + barrierHeight / 2; i++)
        {
            for(int j = barrierPos - barrierHeight / 4; j < barrierPos + barrierHeight / 4; j++)
            {
                sim->getGrid()[i][j] = 0b10000000;
            }
        }
    });

    std::vector<std::vector<std::pair<double, double>>> velField;
    auto t = std::chrono::system_clock::now();
    std::vector<std::vector<double>> dField;
    std::vector<std::vector<double>> vmField;
    velField = sim.getVelocityField(imageSampleWH, imageSampleWH);
    auto info = sim.getVelocityMagnitudeAndDensityField(imageSampleWH, imageSampleWH);
    vmField = info.first;
    dField = info.second;

    std::vector<std::pair<double, double>> parts;
    for(int i = 0; i < 50; i++)
    {
        for(int j = 0; j < 50; j++)
        {
            parts.push_back({w/50 * j + 1, h/50 * i + 1});
        }
    }

    for (int i = 0; i < steps; i++)
    {
        sim.moveStep();
        sim.colissionStep();
        sim.spawnAtX(0.4, 0, reserveWidth);
        sim.spawnAtX(0.2, w - reserveWidth, reserveWidth);

        if (i % 10 == 0) qDebug() << i;

        // velocity field saving
/*
        if (i == (steps - 5000) + 1) saveVelToFile("out1.dat", velField);
        if (i == (steps - 5000) + 2) saveVelToFile("out2.dat", velField);
        if (i == (steps - 5000) + 5) saveVelToFile("out5.dat", velField);
        if (i == (steps - 5000) + 10) saveVelToFile("out10.dat", velField);
        if (i == (steps - 5000) + 20) saveVelToFile("out20.dat", velField);
        if (i == (steps - 5000) + 50) saveVelToFile("out50.dat", velField);
        if (i == (steps - 5000) + 100) saveVelToFile("out100.dat", velField);
        if (i == (steps - 5000) + 200) saveVelToFile("out200.dat", velField);
        if (i == (steps - 5000) + 500) saveVelToFile("out500.dat", velField);
        if (i == (steps - 5000) + 1000) saveVelToFile("out1000.dat", velField);
        if (i == (steps - 5000) + 2000) saveVelToFile("out2000.dat", velField);

        if (i >= steps - 5000)
        {
            if (i == steps - 5000) velField = sim.getVelocityField(4, 4);
            else velField = average(velField, i - (steps - 5000), sim.getVelocityField(4, 4));
        }
        if (i == 1000)
        {
            qDebug() << double(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - t).count()) / 1000000.;
        }
*/

        // flow lines + density field data generation
        // save flow line image to velocity magnitude data just for simplicity
        if((i < 30000 && i%200 <= 10) || (i >= 30000 && i%100 <= 10))
        {
            // generation of data only after 10 temporal samples
            if(i%100==10)
            {
                parts.clear();
                for(int k = 0; k < 30; k++)
                {
                    for(int j = 0; j < 100; j++)
                    {
                        parts.push_back({w/100 * j + 1, h/30 * k + 1});
                    }
                }

                for(auto& v : vmField)
                {
                    v.assign(v.size(), 0);
                }

                for(int i = 0; i < 500; i++)
                {
                    for(int j = 0; j < parts.size(); j++)
                    {
                        auto& p = parts[j];
                        if(p.first > w-1 || p.first < 0 || p.second > h-1 || p.second < 0)continue;
                        auto v = velField[p.second/imageSampleWH][p.first/imageSampleWH];
                        auto m = sqrt(pow(v.first, 2.) + pow(v.second, 2.));
                        if(m<0.000001)
                        {
                            continue;
                        }
                        else
                        {
                            vmField[p.second/imageSampleWH][p.first/imageSampleWH] = fmin(m, 1);
                            p.first = p.first + (v.first / m);
                            p.second = p.second + (v.second /m);
                        }
                    }
                }
                std::unique_lock<std::mutex> l1(m_velMagnitudeMutex);
                m_velMagnitude = vmField;
                l1.unlock();
                std::unique_lock<std::mutex> l2(m_densityMutex);
                m_density = dField;
                l2.unlock();
            }
            else
            {
                auto info = sim.getVelocityAndDensityField(imageSampleWH, imageSampleWH);
                dField = average(dField, i%10, info.second);
                velField = average(velField, i%10, info.first);
            }
        }

        if(m_stopThread)
        {
            m_stopThread = false;
            return;
        }
    }

    //std::unique_lock<std::mutex> l1(m_velMagnitudeMutex);
    //m_velMagnitude = vmField;
    //l1.unlock();

    //saveVelToFile("out.dat", velField);
}
