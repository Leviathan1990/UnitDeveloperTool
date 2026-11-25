#include "BoxWriter.h"

#include <QFile>
#include <QDataStream>

BoxWriter::BoxWriter() {}

bool BoxWriter::writeBoxFile(const QString& filepath, const QVector<BoxFile>& files)
{
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly))
    {
        m_lastError = QString("Cannot open file for writing: %1").arg(filepath);
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // 1. File data first, recomputing each entry's real offset/size in
    // THIS output (see header comment - never trust the input entry's own
    // offset/size, they describe a different, possibly no-longer-relevant
    // source file).
    struct WrittenEntry
    {
        QString filename;
        quint32 offset;
        quint32 size;
    };
    QVector<WrittenEntry> written;
    written.reserve(files.size());

    for (const BoxFile& f : files)
    {
        WrittenEntry entry;
        entry.filename = f.filename;
        entry.offset = static_cast<quint32>(file.pos());
        entry.size = static_cast<quint32>(f.data.size());
        stream.writeRawData(f.data.constData(), f.data.size());
        written.append(entry);
    }

    // 2. Directory, right after all file data.
    const quint32 directoryOffset = static_cast<quint32>(file.pos());

    stream << static_cast<quint32>(written.size());
    for (const WrittenEntry& entry : written)
    {
        QByteArray nameBytes = entry.filename.toUtf8();
        stream.writeRawData(nameBytes.constData(), nameBytes.size());
        stream.writeRawData("\0", 1);
        stream << entry.offset << entry.size;
    }

    // 3. Footer: the directory's own offset, as the final 4 bytes.
    stream << directoryOffset;

    file.close();
    return true;
}
