#include "BoxStruct.h"
#include "NullTerminator.h"

#include <QFile>
#include <QDataStream>
#include <QDebug>

BoxStruct::BoxStruct() : numberOfFiles(0), directoryOffset(0) {}

bool BoxStruct::loadBoxFile(const QString& filepath)
{
    QFile file(filepath);

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Cannot open archive:" << filepath;
        return false;
    }

    // Teljes fájl beolvasása
    fileDataBuffer = file.readAll();
    qint64 fileSize = file.size();
    qDebug() << "File size:" << fileSize << "bytes";

    if (fileSize < 4)
    {
        qDebug() << "File too small to contain directory offset!";
        file.close();
        return false;
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    // Directory offset beolvasása a fájl végéről
    in.device()->seek(fileSize - 4);
    in >> directoryOffset;
    qDebug() << "Directory offset:" << directoryOffset;

    if (directoryOffset >= fileSize - 4)
    {
        qDebug() << "Invalid directory offset:" << directoryOffset;
        file.close();
        return false;
    }

    // TOC kezdetére ugrás
    in.device()->seek(directoryOffset);

    if (in.atEnd())
    {
        qDebug() << "Unexpected end of file at directory offset!";
        file.close();
        return false;
    }

    // Fájlok számának beolvasása
    in >> numberOfFiles;
    qDebug() << "Number of files:" << numberOfFiles;

    if (numberOfFiles > 10000 || numberOfFiles == 0)
    {
        qDebug() << "Invalid number of files:" << numberOfFiles;
        file.close();
        return false;
    }

    files.clear();

    // Fájlok adatainak beolvasása
    for (uint32_t i = 0; i < numberOfFiles; ++i)
    {
        BoxFile boxFile;

        qDebug() << "Reading file" << i + 1 << "at position" << file.pos();

        // Fájlnév beolvasása
        boxFile.filename = NullTerminator::readNullTerminatedString(in);

        if (boxFile.filename.isEmpty() && in.atEnd())
        {
            qDebug() << "File" << i + 1 << ": Unexpected end of file while reading filename!";
            file.close();
            return false;
        }

        qDebug() << "Filename:" << boxFile.filename;

        // Offset és méret beolvasása
        if (file.pos() + 8 > fileSize)
        {
            qDebug() << "File" << i + 1 << ": Not enough data for offset and size!";
            file.close();
            return false;
        }

        in >> boxFile.offset;
        qDebug() << "Offset:" << boxFile.offset;
        in >> boxFile.size;
        qDebug() << "Size:" << boxFile.size;

        // Fájl adat kinyerése
        if (boxFile.offset + boxFile.size <= fileSize)
        {
            boxFile.data = fileDataBuffer.mid(boxFile.offset, boxFile.size);
        }

        else
        {
            qDebug() << "File" << i + 1 << ": Invalid offset or size!";
            file.close();
            return false;
        }

        files.append(boxFile);
    }

    file.close();
    qDebug() << "Successfully loaded" << numberOfFiles << "files from" << filepath;
    return true;
}

QVector<BoxFile> BoxStruct::getFiles() const
{
    return files;
}
