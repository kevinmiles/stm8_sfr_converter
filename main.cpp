
#include <QCoreApplication>
#include <QCommandLineParser>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#define PADLENGTH 42

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName("IAR2COSMIC");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("IAR2COSMIC - converts the IAR *.sfr device files to Cosmic header files.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("sfr", QCoreApplication::translate("main", "IAR SFR file"));
    parser.addPositionalArgument("header", QCoreApplication::translate("main", "DEstination header file"));

    // Process the actual command line arguments given by the user
    parser.process(a);

    const QStringList args = parser.positionalArguments();

    if (args.length() != 2) {
        parser.showHelp();
        return 0;
    }

    QFile sfrFile, header;
    sfrFile.setFileName(args.at(0));
    header.setFileName(args.at(1));

    if (!header.open(QFile::WriteOnly))
        return -1;


    // ;; sfr = "name", "zone", address, size, base=<base>[, bitRange=bit[-bit]]
    // ;;       [, readOnly/writeOnly]
    // sfr = "PA_IDR",            "Memory", 0x5001, 1, base=16, tooltip="Port A input pin value register"
    if (sfrFile.open(QFile::ReadOnly | QFile::Text)) {
        header.write("#ifndef IOSTM8S003K3_H\n" \
                     "#define IOSTM8S003K3_H\n\n" \
                     "//Generated from IAR's iostm8s003k3.h\n\n");
        QTextStream stream(&sfrFile);
        bool lastSFR = true;
        QString bitFieldBaseAddr, bitFieldRegName;
        while(!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.startsWith("sfr =")) {
                if (!line.contains("bitRange")) {
                    if (!lastSFR) {
                        QString pad;
                        pad = pad.leftJustified(PADLENGTH-QString("volatile __BITS_  _bits").length() - 2 * bitFieldRegName.length(), ' ');
                        header.write(QString("} __BITS_%1;\n").arg(bitFieldRegName).toLocal8Bit());
                        header.write(QString("volatile __BITS_%1  %1_bits%3@0x%2;\n\n\n").arg(bitFieldRegName).arg(bitFieldBaseAddr).arg(pad).toLocal8Bit());
                    }

                    QRegularExpression re(".*=\\s\\\"(\\S*)\\\",\\s*\\\".*\\\",\\s0x([0-9A-F]*),.*tooltip=\\\"(.*)\\\"");
                    QRegularExpressionMatch match = re.match(line);
                    if (match.hasMatch()) {
                        QString matched = match.captured(0);
                        QString pad;
                        pad = pad.leftJustified(PADLENGTH-match.captured(1).length() - QString("volatile unsigned char ").length(), ' ');
                        header.write(QString("volatile unsigned char %1%4@0x%2; // %3\n").arg( match.captured(1)).arg( match.captured(2)).arg( match.captured(3)).arg(pad).toLocal8Bit());
                    }
                    lastSFR = true;
                } else {
                    if (lastSFR) {
                        header.write("typedef struct\n{\n");
                    }

                    QRegularExpression re("sfr\\s=\\s\\\"(\\S*)\\.(\\S*)\\\",\\s*\\\".*\\\",\\s0x([0-9A-F]*),.*bitRange=(.*)");
                    QRegularExpressionMatch match = re.match(line);
                    if (match.hasMatch()) {
                        bitFieldRegName = match.captured(1);
                        bitFieldBaseAddr = match.captured(3);
                        if (match.captured(4).contains('-')) {
                            QStringList parts = match.captured(4).split('-');
                            int firstBitIndex = parts.first().toInt();
                            int lastBitIndex = parts.at(1).toInt();
                            QString pad;
                            pad = pad.leftJustified(PADLENGTH-match.captured(2).length() - QString("    unsigned char  : ").length(), ' ');
                            header.write(QString("    unsigned char %1%2 : %3;\n")
                                         .arg(match.captured(2))
                                         .arg(pad)
                                         .arg(lastBitIndex - firstBitIndex + 1)
                                         .toLocal8Bit());
                        } else {
                            QString pad;
                            pad = pad.leftJustified(PADLENGTH-match.captured(2).length() - QString("    unsigned char  : ").length(), ' ');
                            header.write(QString("    unsigned char %1%2 : 1;\n")
                                         .arg(match.captured(2))
                                         .arg(pad).toLocal8Bit());
                        }
                    }
                    lastSFR = false;
                }
            }
        }
        header.write("#endif // #ifdef IOSTM8S003K3_H");
    } else {
        header.close();
        return -1;
    }
    header.close();
    sfrFile.close();

    return 0;
}
