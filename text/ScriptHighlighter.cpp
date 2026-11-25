#include "ScriptHighlighter.h"
#include "ScriptVocabulary.h"

#include <QFileInfo>

ScriptHighlighter::ScriptHighlighter(Mode mode, QTextDocument* parent)
    : QSyntaxHighlighter(parent), m_mode(mode)
{
    setupFormats();
    setupRules();
}

ScriptHighlighter::Mode ScriptHighlighter::modeForFileName(const QString& fileName)
{
    const QString ext = QFileInfo(fileName).suffix().toLower();
    if (ext == "ai")
    {
        return Mode::Tabular;
    }
    return Mode::CallSyntax; // .cfg, .oms, .txt and anything else
}

void ScriptHighlighter::setupFormats()
{
    m_keywordFormat.setForeground(QColor(0, 0, 220));
    m_keywordFormat.setFontWeight(QFont::Bold);

    m_stringFormat.setForeground(QColor(0, 120, 0));

    m_numberFormat.setForeground(QColor(160, 80, 0));

    m_commentFormat.setForeground(QColor(120, 120, 120));
    m_commentFormat.setFontItalic(true);
}

void ScriptHighlighter::setupRules()
{
    m_rules.clear();

    // Keywords - one alternation regex over the mode-appropriate vocabulary,
    // whole-word matched. Longer names first isn't required for correctness
    // here (all names are distinct, none is a prefix of another that would
    // cause a shorter match to win first), so the ScriptVocabulary order is
    // used as-is.
    const QStringList& words = (m_mode == Mode::CallSyntax)
        ? ScriptVocabulary::callSyntaxFunctions()
        : ScriptVocabulary::tabularKeys();

    if (!words.isEmpty())
    {
        QStringList escaped;
        escaped.reserve(words.size());
        for (const QString& w : words)
        {
            escaped << QRegularExpression::escape(w);
        }
        Rule keywordRule;
        keywordRule.pattern = QRegularExpression(QStringLiteral("\\b(?:%1)\\b").arg(escaped.join('|')));
        keywordRule.format = m_keywordFormat;
        m_rules.append(keywordRule);
    }

    // Numeric literals (both modes use them - Create() call params, and
    // Value/Variance columns in .ai files).
    Rule numberRule;
    numberRule.pattern = QRegularExpression(QStringLiteral("-?\\b\\d+\\.?\\d*\\b"));
    numberRule.format = m_numberFormat;
    m_rules.append(numberRule);

    if (m_mode == Mode::CallSyntax)
    {
        // String literals only occur in the call-syntax family
        // ("Human_Explosion_01" style name/material/sound arguments).
        Rule stringRule;
        stringRule.pattern = QRegularExpression(QStringLiteral("\"[^\"]*\""));
        stringRule.format = m_stringFormat;
        m_rules.append(stringRule);
    }

    // Comments are applied last (in highlightBlock, after these m_rules)
    // so they take priority over keyword/string/number coloring within the
    // commented-out portion of a line.
}

void ScriptHighlighter::highlightBlock(const QString& text)
{
    for (const Rule& rule : std::as_const(m_rules))
    {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext())
        {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Comments last, so they win over anything already colored on the same
    // stretch of text.
    if (m_mode == Mode::CallSyntax)
    {
        // "//" can appear anywhere on the line (confirmed: EmitterMaterials.cfg
        // uses trailing inline comments), not just at the start.
        int idx = text.indexOf(QStringLiteral("//"));
        if (idx >= 0)
        {
            setFormat(idx, text.length() - idx, m_commentFormat);
        }
    }
    else
    {
        // .ai comments are full-line only, prefixed with "//" or ";" as the
        // first non-whitespace content.
        static const QRegularExpression fullLineComment(QStringLiteral("^\\s*(//|;).*$"));
        QRegularExpressionMatch match = fullLineComment.match(text);
        if (match.hasMatch())
        {
            setFormat(match.capturedStart(1), text.length() - match.capturedStart(1), m_commentFormat);
        }
    }
}
