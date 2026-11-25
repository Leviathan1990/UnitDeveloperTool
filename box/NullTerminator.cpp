#include "NullTerminator.h"
#include <QDebug>

QString NullTerminator::readNullTerminatedString(QDataStream& in)
{
    QByteArray bytes;
    char b;

    while (in.readRawData(&b, 1) == 1 && b != '\0')
    {
        bytes.append(b);
        if (bytes.size() > 256)
        {
            qDebug() << "Filename too long!";
            return QString();
        }
    }
    return QString::fromLatin1(bytes); // ASCII
}
