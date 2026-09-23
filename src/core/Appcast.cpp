#include "core/Appcast.h"

#include <QXmlStreamReader>

namespace {

const QString kSparkleNs = QStringLiteral("http://www.andymatuschak.org/xml-namespaces/sparkle");
constexpr qint64 kMaxInstallerBytes = 200LL * 1024 * 1024;

bool isForWindows(const QString &os)
{
    return os.isEmpty() || os == QLatin1String("windows") || os == QLatin1String("windows-x64");
}

} // namespace

std::optional<UpdateInfo> Appcast::parse(const QByteArray &xml)
{
    QXmlStreamReader reader(xml);
    std::optional<UpdateInfo> best;
    UpdateInfo current;
    bool inItem = false;
    bool itemValid = false;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("item")) {
                inItem = true;
                itemValid = false;
                current = {};
            } else if (inItem && reader.name() == QLatin1String("description")) {
                current.notesHtml = reader.readElementText().trimmed();
            } else if (inItem && reader.name() == QLatin1String("enclosure")) {
                const auto attrs = reader.attributes();
                const QUrl url(attrs.value("url").toString());
                const QByteArray signature =
                    QByteArray::fromBase64(attrs.value(kSparkleNs, "edSignature").toLatin1());
                bool lengthOk = false;
                const qint64 length = attrs.value("length").toLongLong(&lengthOk);

                current.version = attrs.value(kSparkleNs, "version").toString().trimmed();
                current.url = url;
                current.signature = signature;
                current.length = length;
                itemValid = url.isValid() && url.scheme() == QLatin1String("https") && signature.size() == 64
                    && lengthOk && length > 0 && length <= kMaxInstallerBytes && !current.version.isEmpty()
                    && isForWindows(attrs.value(kSparkleNs, "os").toString());
            }
        } else if (reader.isEndElement() && reader.name() == QLatin1String("item")) {
            if (itemValid && (!best || compareVersions(current.version, best->version) > 0))
                best = current;
            inItem = false;
        }
    }
    if (reader.hasError())
        return std::nullopt;
    return best;
}

int Appcast::compareVersions(const QString &a, const QString &b)
{
    const QStringList pa = a.split('.');
    const QStringList pb = b.split('.');
    for (qsizetype i = 0; i < std::max(pa.size(), pb.size()); ++i) {
        const int x = i < pa.size() ? pa[i].toInt() : 0;
        const int y = i < pb.size() ? pb[i].toInt() : 0;
        if (x != y)
            return x < y ? -1 : 1;
    }
    return 0;
}
