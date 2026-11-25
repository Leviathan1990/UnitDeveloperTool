#ifndef BOXSTRUCT_H
#define BOXSTRUCT_H

#include "QMetaType"
#include <QString>
#include <QVector>
#include <cstdint>

struct BoxFile {
    QString filename;
    uint32_t offset;
    uint32_t size;
    QByteArray data;
};
Q_DECLARE_METATYPE(BoxFile);

class BoxStruct {
public:
    BoxStruct();
    bool loadBoxFile(const QString& filepath);
    QVector<BoxFile> getFiles() const;

private:
    uint32_t numberOfFiles;
    uint32_t directoryOffset;
    QVector<BoxFile> files;
    QByteArray fileDataBuffer;
};

#endif // BOXSTRUCT_H
