#ifndef BOXWRITER_H
#define BOXWRITER_H

#include "BoxStruct.h"
#include <QString>
#include <QVector>

// Writes a .box archive from an in-memory file list, using the same binary
// layout BoxStruct::loadBoxFile() reads (file data, then a directory at
// dirOffset: count + per-entry null-terminated filename + offset + size,
// then a trailing 4-byte footer holding dirOffset itself). This exact
// layout is confirmed via three independent implementations agreeing:
// OutforceMapEditor's BoxArchive.cpp (proven against the real Content.box
// all session), BoxBuilder's own BoxStruct.cpp reader, and BoxBuilder's
// NewProj::on_button1_clicked()/writeContent() writer (newproj.cpp) - this
// class is a reusable extraction of that last one, generalized to take a
// flat QVector<BoxFile> directly instead of a QStandardItemModel tree (the
// UI tree used for browsing/display, built separately in BoxArchiveWindow,
// has no bearing on the archive's own on-disk layout, which is always a
// flat file list regardless of how a viewer chooses to group names by "/").
//
// Unlike BoxStruct (read side, carried over unmodified from BoxBuilder),
// this write side never existed as a standalone/reusable piece anywhere -
// BoxBuilder itself only ever builds a brand new archive from a freshly
// picked file tree (NewProj), it has no "open an existing archive, then
// re-save it" path. This class fills that gap.
class BoxWriter
{
public:
    BoxWriter();

    // Writes `files` to `filepath` in .box format. Each entry's `data` is
    // written as-is; `offset`/`size` are recomputed here (not read from the
    // input) since they must reflect where each entry actually lands in
    // THIS output file, not wherever it happened to live in a previously
    // loaded archive.
    bool writeBoxFile(const QString& filepath, const QVector<BoxFile>& files);

    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
};

#endif // BOXWRITER_H
