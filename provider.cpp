#include "provider.h"

Provider::Provider(QSharedPointer<SimRunner> simRunner) : QQuickImageProvider(QQmlImageProviderBase::Image),
    m_simRunner(simRunner)
{
}

void deleter(void* toDelete)
{
    delete toDelete;
}

QImage Provider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    auto simdata = id[0]=="d" ? m_simRunner->density() : m_simRunner->velMagnitude();

    if(simdata.size() == 0)
    {
        if(requestedSize.width() > 0 && requestedSize.height() > 0)
        {
            QImage im(requestedSize.width(), requestedSize.height(), QImage::Format_Grayscale8);
            im.fill(128);
            *size = requestedSize;
            return im;
        }
        else
        {
            QImage im(100, 100, QImage::Format_Grayscale8);
            im.fill(128);
            *size = QSize(100, 100);
            return im;
        }
    }

    uchar* data = new uchar[simdata.size()*simdata[0].size()];
    for(int i = 0; i < simdata.size(); i++)
    {
        for(int j = 0; j < simdata[i].size(); j++)
        {
            data[i*simdata[i].size() + j] = id[0]=="d" ? 511*fmin(simdata[i][j], 0.5) : 255*fmin(simdata[i][j], 1.);
        }
    }

    QImage img(data, simdata[0].size(), simdata.size(), QImage::Format_Grayscale8, deleter, data);

    *size = QSize(simdata[0].size(), simdata.size());

    if((requestedSize.width() != simdata[0].size() || requestedSize.height() != simdata.size()) && requestedSize.width() > 0 && requestedSize.height() > 0)
    {
        img = img.scaled(requestedSize.width(), requestedSize.height());
    }

    return img;
}
