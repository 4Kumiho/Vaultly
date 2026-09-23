// Genera l'icona di Vaultly: PNG 1024x1024 + ICO multi-dimensione.
#include <QBuffer>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QtMath>

static const QColor kAccentLight("#8fb0ff");
static const QColor kAccent("#5b8cff");
static const QColor kAccentDark("#3f6be0");

// Disegna in coordinate 1024x1024; `detailed` = dettagli fini (solo per le dimensioni grandi).
static void paintIcon(QPainter &p, bool detailed)
{
    p.setRenderHint(QPainter::Antialiasing);

    // Sfondo: quadrato arrotondato scuro con leggera luce in alto.
    QLinearGradient bg(0, 48, 0, 976);
    bg.setColorAt(0, QColor("#232c42"));
    bg.setColorAt(1, QColor("#0e1118"));
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(QRectF(48, 48, 928, 928), 210, 210);
    if (detailed) {
        p.setPen(QPen(QColor(255, 255, 255, 18), 6));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(51, 51, 922, 922), 207, 207);
    }

    // Piedini e cerniere.
    p.setPen(Qt::NoPen);
    if (detailed) {
        p.setBrush(QColor("#161a24"));
        p.drawRoundedRect(QRectF(250, 712, 90, 58), 18, 18);
        p.drawRoundedRect(QRectF(600, 712, 90, 58), 18, 18);
        p.setBrush(QColor("#4a5268"));
        p.drawRoundedRect(QRectF(162, 300, 40, 100), 14, 14);
        p.drawRoundedRect(QRectF(162, 540, 40, 100), 14, 14);
    }

    // Corpo della cassaforte.
    QLinearGradient body(0, 200, 0, 740);
    body.setColorAt(0, QColor("#48516a"));
    body.setColorAt(1, QColor("#262c3b"));
    p.setBrush(body);
    p.drawRoundedRect(QRectF(180, 200, 580, 540), 78, 78);

    // Sportello.
    QLinearGradient door(0, 240, 0, 700);
    door.setColorAt(0, QColor("#343c50"));
    door.setColorAt(1, QColor("#1c2130"));
    p.setBrush(door);
    if (detailed)
        p.setPen(QPen(QColor(255, 255, 255, 22), 5));
    p.drawRoundedRect(QRectF(224, 244, 492, 452), 50, 50);
    p.setPen(Qt::NoPen);

    const QPointF c(470, 470);

    // Maniglie della ghiera.
    if (detailed) {
        p.setBrush(QColor("#9aa3b8"));
        for (int i = 0; i < 6; ++i) {
            p.save();
            p.translate(c);
            p.rotate(i * 60 + 30);
            p.drawRoundedRect(QRectF(-14, -205, 28, 70), 14, 14);
            p.restore();
        }
    }

    // Ghiera: anello blu, disco scuro, tacche, pomello.
    QLinearGradient ring(c.x() - 150, c.y() - 150, c.x() + 150, c.y() + 150);
    ring.setColorAt(0, kAccentLight);
    ring.setColorAt(1, kAccentDark);
    p.setBrush(ring);
    p.drawEllipse(c, 150, 150);
    p.setBrush(QColor("#171c28"));
    p.drawEllipse(c, 112, 112);
    if (detailed) {
        p.setPen(QPen(QColor(255, 255, 255, 70), 6, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < 24; ++i) {
            const qreal a = qDegreesToRadians(i * 15.0);
            const qreal r1 = (i % 6 == 0) ? 84 : 94;
            p.drawLine(c + QPointF(qCos(a) * r1, qSin(a) * r1), c + QPointF(qCos(a) * 104, qSin(a) * 104));
        }
        p.setPen(Qt::NoPen);
    }
    QRadialGradient knob(c - QPointF(16, 16), 70);
    knob.setColorAt(0, kAccentLight);
    knob.setColorAt(1, kAccentDark);
    p.setBrush(knob);
    p.drawEllipse(c, 50, 50);

    // Scudo con spunta, in basso a destra, staccato dal corpo da un bordo scuro.
    QPainterPath shield;
    shield.moveTo(730, 520);
    shield.cubicTo(790, 555, 850, 568, 900, 566);
    shield.lineTo(900, 690);
    shield.cubicTo(900, 800, 830, 870, 730, 912);
    shield.cubicTo(630, 870, 560, 800, 560, 690);
    shield.lineTo(560, 566);
    shield.cubicTo(610, 568, 670, 555, 730, 520);
    shield.closeSubpath();

    p.setPen(QPen(QColor("#0e1118"), 30, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(shield);
    QLinearGradient shieldFill(600, 520, 880, 900);
    shieldFill.setColorAt(0, kAccentLight);
    shieldFill.setColorAt(0.5, kAccent);
    shieldFill.setColorAt(1, kAccentDark);
    p.setPen(Qt::NoPen);
    p.setBrush(shieldFill);
    p.drawPath(shield);

    p.setPen(QPen(Qt::white, detailed ? 46 : 60, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QPainterPath check;
    check.moveTo(648, 718);
    check.lineTo(710, 780);
    check.lineTo(818, 662);
    p.drawPath(check);
}

static QImage render(int size)
{
    // Si disegna a 4x e si riduce: bordi più puliti nelle dimensioni piccole.
    const int work = std::max(size * 4, 1024);
    QImage img(work, work, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.scale(work / 1024.0, work / 1024.0);
    paintIcon(p, size >= 64);
    p.end();
    return img.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

static QByteArray png(const QImage &img)
{
    QByteArray data;
    QBuffer buf(&data);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return data;
}

// ICO con voci PNG (supportate da Windows Vista in poi).
static bool writeIco(const QString &path, const QList<int> &sizes)
{
    QList<QByteArray> images;
    for (int s : sizes)
        images.append(png(render(s)));

    QByteArray out;
    auto u16 = [&out](quint16 v) { out.append(char(v & 0xff)); out.append(char(v >> 8)); };
    auto u32 = [&out](quint32 v) { for (int i = 0; i < 4; ++i) out.append(char((v >> (8 * i)) & 0xff)); };
    u16(0);
    u16(1);
    u16(quint16(sizes.size()));
    quint32 offset = 6 + 16 * sizes.size();
    for (int i = 0; i < sizes.size(); ++i) {
        const int s = sizes[i];
        out.append(char(s >= 256 ? 0 : s));
        out.append(char(s >= 256 ? 0 : s));
        out.append(char(0));
        out.append(char(0));
        u16(1);
        u16(32);
        u32(quint32(images[i].size()));
        u32(offset);
        offset += images[i].size();
    }
    for (const QByteArray &img : images)
        out.append(img);

    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(out) == out.size();
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString(".");
    const bool ok = render(1024).save(dir + "/vaultly.png")
        && writeIco(dir + "/vaultly.ico", {16, 20, 24, 32, 40, 48, 64, 128, 256})
        && render(32).save(dir + "/preview-32.png") && render(16).save(dir + "/preview-16.png");
    return ok ? 0 : 1;
}
