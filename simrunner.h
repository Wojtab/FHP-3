#ifndef SIMRUNNER_H
#define SIMRUNNER_H

#include <QObject>
#include <thread>
#include <atomic>
#include <mutex>

class SimRunner : public QObject
{
    Q_OBJECT
public:
    explicit SimRunner(QObject *parent = nullptr);

    std::vector<std::vector<double>> density();
    std::vector<std::vector<double>> velMagnitude();

    ~SimRunner();

public slots:
    void start();
    void stop();

signals:

private:
    static inline std::pair<double, double> average(const std::pair<double, double>& prev, int sampleCount, const std::pair<double, double>& sample);

    static inline std::vector<std::vector<std::pair<double, double>>> average(const std::vector<std::vector<std::pair<double, double>>>& prev, int sampleCount, const std::vector<std::vector<std::pair<double, double>>>& sample);

    static inline std::vector<std::vector<double>> average(const std::vector<std::vector<double>>& prev, int sampleCount, const std::vector<std::vector<double>>& sample);

    static void saveVelToFile(std::string name, const std::vector<std::vector<std::pair<double, double>>>& velField);

    void plate(int w, int h, int reserveWidth, int steps, int barrierHeight, int barrierPos);

    void wave(int w, int h, int originX, int originY, int radius);

    std::thread m_simThread;
    std::atomic_bool m_stopThread = false;

    std::vector<std::vector<double>> m_density;
    std::vector<std::vector<double>> m_velMagnitude;
    std::mutex m_densityMutex;
    std::mutex m_velMagnitudeMutex;
};

#endif // SIMRUNNER_H
