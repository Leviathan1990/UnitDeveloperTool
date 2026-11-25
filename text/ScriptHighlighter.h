#ifndef SCRIPTHIGHLIGHTER_H
#define SCRIPTHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

// Highlights the two real Outforce text-script syntaxes (see
// ScriptVocabulary.h for how the keyword lists were ground-truthed).
class ScriptHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    enum class Mode
    {
        CallSyntax, // .cfg, .oms - FunctionName(args...); with "strings", // comments
        Tabular     // .ai - Name<TAB>Value<TAB>Variance, // or ; comments
    };

    explicit ScriptHighlighter(Mode mode, QTextDocument* parent = nullptr);

    // Picks CallSyntax for .cfg/.oms, Tabular for .ai, defaulting to
    // CallSyntax for anything else (.txt and similar) since it's the more
    // common/general of the two grammars.
    static Mode modeForFileName(const QString& fileName);

protected:
    void highlightBlock(const QString& text) override;

private:
    Mode m_mode;

    struct Rule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<Rule> m_rules;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_commentFormat;

    void setupFormats();
    void setupRules();
};

#endif // SCRIPTHIGHLIGHTER_H
