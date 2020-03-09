#ifndef PROVIDER_H
#define PROVIDER_H

#include <QQuickImageProvider>
#include "simrunner.h"

class Provider : public QQuickImageProvider
{
public:
    Provider(QSharedPointer<SimRunner> simRunner);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);

private:
    QSharedPointer<SimRunner> m_simRunner;
};

#endif // PROVIDER_H
