#ifndef NULLTERMINATOR_H
#define NULLTERMINATOR_H

#include <QDataStream>
#include <QString>

class NullTerminator {
public:
    static QString readNullTerminatedString(QDataStream& in);
};

#endif // NULLTERMINATOR_H
