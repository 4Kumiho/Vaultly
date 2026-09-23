#include "ui/Theme.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QStyleFactory>
#include <QStyleHints>

namespace {

// Colori
// sfondo     #0f1115   superficie #171a21   bordo  #252a35 / #2a2f3a
// testo      #e6e8ee   secondario #8b93a7   spento #5d6475
// accento    #5b8cff   negativo   #ff6b6b
const char *const kStyleSheet = R"(
QLabel { background: transparent; }

QLabel[role="brand"]      { color: #5b8cff; font-size: 11pt; font-weight: 700; }
QLabel[role="title"]      { font-size: 22pt; font-weight: 600; }
QLabel[role="subtitle"]   { color: #8b93a7; font-size: 10.5pt; }
QLabel[role="fieldLabel"] { color: #aab1c2; font-size: 9pt; font-weight: 600; }
QLabel[role="caption"]    { color: #8b93a7; font-size: 8.5pt; font-weight: 700; }
QLabel[role="muted"]      { color: #8b93a7; }
QLabel[role="balance"]    { font-size: 36pt; font-weight: 700; }
QLabel[role="cardTitle"]  { font-size: 11pt; font-weight: 600; }
QLabel[role="cardAmount"] { font-size: 14pt; font-weight: 700; }
QLabel[role="sectionTitle"] { font-size: 13pt; font-weight: 600; }
QLabel[role="hint"]       { color: #5d6475; font-size: 8.5pt; }
QLabel[role="rowTitle"]   { font-weight: 600; }
QLabel[role="rowAmount"]  { font-size: 10.5pt; font-weight: 700; }
QLabel[role="statAmount"] { font-size: 12.5pt; font-weight: 700; }
QLabel[tone="negative"]   { color: #ff6b6b; }
QLabel[tone="positive"]   { color: #34d399; }

QLabel#rowIcon { border-radius: 19px; font-size: 11pt; font-weight: 700; }
QLabel#rowIcon[tone="positive"] { background: rgba(52, 211, 153, 0.14); color: #34d399; }
QLabel#rowIcon[tone="negative"] { background: rgba(255, 107, 107, 0.14); color: #ff8a8a; }
QLabel#rowIcon[tone="accent"]   { background: rgba(91, 140, 255, 0.16); color: #7aa2ff; }
QFrame#listRow { background: transparent; border-radius: 10px; }
QFrame#listRow:hover { background: #1e222c; }

QLabel[role="tag"] {
    background: rgba(91, 140, 255, 0.14);
    color: #9db8ff;
    border-radius: 9px;
    padding: 1px 8px;
    font-size: 8.5pt;
    font-weight: 600;
}

QFrame#tagInput {
    background: #0f1115;
    border: 1px solid #2a2f3a;
    border-radius: 10px;
}
QFrame#tagInput:hover { border-color: #3a4152; }
QFrame#tagInput QLineEdit {
    background: transparent;
    border: none;
    min-height: 28px;
    padding: 0 4px;
}
QFrame#tagChip {
    background: rgba(91, 140, 255, 0.18);
    border-radius: 13px;
}
QFrame#tagChip QLabel { color: #b5c9ff; font-weight: 600; }
QFrame#tagChip QToolButton {
    background: transparent;
    border: none;
    color: #8fa8e0;
    font-size: 11pt;
    padding: 0 4px;
}
QFrame#tagChip QToolButton:hover { color: #ffffff; }

QFrame#tagRow { background: transparent; border: 1px solid transparent; border-radius: 10px; }
QFrame#tagRow:hover { background: #1e222c; }
QFrame#tagRow[selected="true"] { background: #1a2133; border-color: #5b8cff; }
QProgressBar#tagBar { background: #0f1115; border: none; border-radius: 3px; }
QProgressBar#tagBar::chunk {
    border-radius: 3px;
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #5b8cff, stop:1 #8fb0ff);
}
QProgressBar#tagBar[tone="muted"]::chunk { background: #3a4152; }

QListView {
    background: #171a21;
    border: 1px solid #2a2f3a;
    color: #e6e8ee;
    selection-background-color: #5b8cff;
    outline: none;
}

QProgressBar {
    background: #0f1115;
    border: 1px solid #252a35;
    border-radius: 4px;
}
QProgressBar::chunk {
    border-radius: 3px;
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8fb0ff, stop:1 #5b8cff);
}
QTextBrowser#releaseNotes {
    background: #12151c;
    border: 1px solid #252a35;
    border-radius: 10px;
    padding: 6px 10px;
    color: #cfd4df;
}

QLabel#toast {
    background: #232834;
    color: #e6e8ee;
    border: 1px solid #323948;
    border-radius: 10px;
    padding: 10px 18px;
    font-weight: 600;
}
QFrame#divider { background: #252a35; border: none; }
QLabel[role="error"] {
    color: #ff8a8a;
    background: rgba(255, 107, 107, 0.10);
    border: 1px solid rgba(255, 107, 107, 0.35);
    border-radius: 8px;
    padding: 8px 10px;
}

QFrame#card {
    background: #171a21;
    border: 1px solid #252a35;
    border-radius: 18px;
}
QFrame#drawer {
    background: #171a21;
    border-left: 1px solid #2a2f3a;
    border-top-left-radius: 18px;
    border-bottom-left-radius: 18px;
}
QFrame#accountCard {
    background: #171a21;
    border: 1px solid #252a35;
    border-radius: 14px;
}
QFrame#accountCard:hover { background: #1b1f28; border-color: #3a4152; }
QFrame#accountCard[selected="true"] { background: #1a2133; border: 1px solid #5b8cff; }
QFrame#newAccountCard {
    background: transparent;
    border: 1px dashed #3a4152;
    border-radius: 14px;
}
QFrame#newAccountCard:hover { border-color: #5b8cff; background: rgba(91, 140, 255, 0.06); }
QFrame#newAccountCard QLabel { color: #8b93a7; font-weight: 600; }
QFrame#newAccountCard:hover QLabel { color: #7aa2ff; }

QLineEdit, QComboBox, QDateTimeEdit {
    background: #0f1115;
    border: 1px solid #2a2f3a;
    border-radius: 10px;
    padding: 0 12px;
    min-height: 40px;
    color: #e6e8ee;
    selection-background-color: #5b8cff;
}
QLineEdit:hover, QComboBox:hover, QDateTimeEdit:hover { border-color: #3a4152; }
QLineEdit:focus, QComboBox:focus, QDateTimeEdit:focus { border-color: #5b8cff; }
QLineEdit:disabled, QComboBox:disabled { color: #5d6475; border-color: #20242d; }
QPlainTextEdit {
    background: #0f1115;
    border: 1px solid #2a2f3a;
    border-radius: 10px;
    padding: 6px 8px;
    color: #e6e8ee;
    selection-background-color: #5b8cff;
}
QPlainTextEdit:hover { border-color: #3a4152; }
QPlainTextEdit:focus { border-color: #5b8cff; }
QLineEdit[emphasis="large"] { font-size: 18pt; font-weight: 600; min-height: 54px; }
QComboBox::drop-down, QDateTimeEdit::drop-down { border: none; width: 30px; }

QCalendarWidget QWidget#qt_calendar_navigationbar { background: #171a21; }
QCalendarWidget QToolButton {
    background: transparent;
    color: #e6e8ee;
    min-height: 28px;
    padding: 0 8px;
    border-radius: 6px;
    font-weight: 600;
}
QCalendarWidget QToolButton:hover { background: #232834; }
QCalendarWidget QAbstractItemView {
    background: #171a21;
    color: #e6e8ee;
    selection-background-color: #5b8cff;
    selection-color: #ffffff;
    outline: none;
}
QCalendarWidget QAbstractItemView:disabled { color: #5d6475; }

QFrame#segmented {
    background: #0f1115;
    border: 1px solid #252a35;
    border-radius: 10px;
}
QPushButton[variant="segment"] {
    background: transparent;
    color: #8b93a7;
    min-height: 30px;
    padding: 0 12px;
    border-radius: 8px;
}
QPushButton[variant="segment"]:hover { color: #e6e8ee; }
QPushButton[variant="segment"]:checked { background: #232834; color: #e6e8ee; }
QPushButton[variant="segment"][tone="expense"]:checked { background: rgba(255, 107, 107, 0.16); color: #ff8a8a; }
QPushButton[variant="segment"][tone="income"]:checked  { background: rgba(52, 211, 153, 0.16); color: #34d399; }
QComboBox QAbstractItemView {
    background: #171a21;
    border: 1px solid #2a2f3a;
    selection-background-color: #5b8cff;
    outline: none;
}

QPushButton {
    border: none;
    border-radius: 10px;
    min-height: 40px;
    padding: 0 18px;
    font-weight: 600;
}
QPushButton[variant="primary"]         { background: #5b8cff; color: #ffffff; }
QPushButton[variant="primary"]:hover   { background: #769fff; }
QPushButton[variant="primary"]:pressed { background: #4a78e0; }
QPushButton[variant="secondary"]         { background: #232834; color: #e6e8ee; }
QPushButton[variant="secondary"]:hover   { background: #2c3240; }
QPushButton[variant="secondary"]:pressed { background: #1e222c; }
QPushButton[variant="link"] {
    background: transparent;
    color: #7aa2ff;
    min-height: 0;
    padding: 2px 4px;
}
QPushButton[variant="link"]:hover { color: #a9c1ff; }
QPushButton[variant="danger"] {
    background: transparent;
    color: #ff6b6b;
    min-height: 0;
    padding: 6px 8px;
}
QPushButton[variant="danger"]:hover { color: #ff9a9a; }
QPushButton[variant="chip"] {
    background: #232834;
    color: #cfd4df;
    min-height: 32px;
    padding: 0 12px;
    border-radius: 8px;
    font-size: 9pt;
}
QPushButton[variant="chip"]:hover    { background: #2c3240; color: #ffffff; }
QPushButton[variant="chip"]:pressed  { background: #1e222c; }
QPushButton[variant="chip"]:disabled { color: #4a5060; background: #1b1f27; }

QScrollArea { background: transparent; border: none; }
QScrollArea > QWidget > QWidget { background: transparent; }
QScrollBar:horizontal { height: 8px; background: transparent; margin: 0; }
QScrollBar::handle:horizontal { background: #2a2f3a; border-radius: 4px; min-width: 40px; }
QScrollBar::handle:horizontal:hover { background: #3a4152; }
QScrollBar:vertical { width: 10px; background: transparent; margin: 4px 2px; }
QScrollBar::handle:vertical { background: #2a2f3a; border-radius: 3px; min-height: 40px; }
QScrollBar::handle:vertical:hover { background: #3a4152; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QToolTip {
    background: #232834;
    color: #e6e8ee;
    border: 1px solid #2a2f3a;
    border-radius: 6px;
    padding: 4px 8px;
}
)";

} // namespace

void Theme::apply(QApplication &app)
{
    // Barra del titolo scura su Windows e palette di base coerente.
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window, QColor("#0f1115"));
    p.setColor(QPalette::WindowText, QColor("#e6e8ee"));
    p.setColor(QPalette::Base, QColor("#0f1115"));
    p.setColor(QPalette::AlternateBase, QColor("#171a21"));
    p.setColor(QPalette::Text, QColor("#e6e8ee"));
    p.setColor(QPalette::Button, QColor("#232834"));
    p.setColor(QPalette::ButtonText, QColor("#e6e8ee"));
    p.setColor(QPalette::Highlight, QColor("#5b8cff"));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::PlaceholderText, QColor("#5d6475"));
    p.setColor(QPalette::ToolTipBase, QColor("#232834"));
    p.setColor(QPalette::ToolTipText, QColor("#e6e8ee"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#5d6475"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#5d6475"));
    app.setPalette(p);

    QFont font("Segoe UI", 10);
    font.setHintingPreference(QFont::PreferNoHinting);
    app.setFont(font);

    app.setStyleSheet(QString::fromUtf8(kStyleSheet));
}
