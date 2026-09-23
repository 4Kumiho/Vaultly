#include "core/Money.h"

#include <QTest>

class TestMoney : public QObject
{
    Q_OBJECT

private slots:
    void format_data()
    {
        QTest::addColumn<qint64>("amount");
        QTest::addColumn<int>("minorUnits");
        QTest::addColumn<QString>("expected");

        QTest::newRow("zero") << qint64(0) << 2 << "0,00";
        QTest::newRow("cents") << qint64(5) << 2 << "0,05";
        // Separatore delle migliaia anche con 4 cifre (QLocale da solo non lo metterebbe).
        QTest::newRow("four digits") << qint64(123456) << 2 << "1.234,56";
        QTest::newRow("millions") << qint64(123456789) << 2 << "1.234.567,89";
        QTest::newRow("thousands") << qint64(1234567) << 2 << "12.345,67";
        QTest::newRow("negative") << qint64(-1234567) << 2 << "-12.345,67";
        QTest::newRow("no decimals") << qint64(15000) << 0 << "15.000";
    }

    void format()
    {
        QFETCH(qint64, amount);
        QFETCH(int, minorUnits);
        QFETCH(QString, expected);
        QCOMPARE(Money::formatNumber(amount, minorUnits, QLocale(QLocale::Italian)), expected);
    }

    void formatWithSymbol()
    {
        const Currency eur{"EUR", "€", 2};
        QCOMPARE(Money::format(1234567, eur, QLocale(QLocale::Italian)), QString("12.345,67 €"));
    }

    void formatWithoutGrouping()
    {
        QCOMPARE(Money::formatNumber(1234567, 2, QLocale(QLocale::Italian), false), QString("12345,67"));
    }

    void parse_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("minorUnits");
        QTest::addColumn<qint64>("expected");

        QTest::newRow("integer") << "12" << 2 << qint64(1200);
        QTest::newRow("comma decimal") << "12,5" << 2 << qint64(1250);
        QTest::newRow("dot decimal") << "12.50" << 2 << qint64(1250);
        QTest::newRow("dot thousands") << "1.234" << 2 << qint64(123400);
        QTest::newRow("italian full") << "1.234,56" << 2 << qint64(123456);
        QTest::newRow("english full") << "1,234.56" << 2 << qint64(123456);
        QTest::newRow("negative") << "-50" << 2 << qint64(-5000);
        QTest::newRow("plus sign") << "+7,1" << 2 << qint64(710);
        QTest::newRow("spaces") << " 1 000,00 " << 2 << qint64(100000);
        QTest::newRow("leading separator") << ",5" << 2 << qint64(50);
        QTest::newRow("yen") << "1.500" << 0 << qint64(1500);
        QTest::newRow("round trip") << Money::formatNumber(-9876543, 2, QLocale(QLocale::Italian)) << 2
                                    << qint64(-9876543);
    }

    void parse()
    {
        QFETCH(QString, text);
        QFETCH(int, minorUnits);
        QFETCH(qint64, expected);
        const auto value = Money::parse(text, minorUnits);
        QVERIFY(value);
        QCOMPARE(*value, expected);
    }

    void parseRejects_data()
    {
        QTest::addColumn<QString>("text");
        QTest::newRow("empty") << "";
        QTest::newRow("only sign") << "-";
        QTest::newRow("letters") << "12a";
        QTest::newRow("symbol") << "12 €";
        QTest::newRow("too many digits") << "12345678901234567";
    }

    void parseRejects()
    {
        QFETCH(QString, text);
        QVERIFY(!Money::parse(text, 2));
    }
};

QTEST_GUILESS_MAIN(TestMoney)
#include "tst_money.moc"
