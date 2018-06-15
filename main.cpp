
#include <QCoreApplication>
#include <QCommandLineParser>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QString>
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
    // An option with a value
    QCommandLineOption sfrOption(QStringList() << "sfr",
                                 QCoreApplication::translate("main", "Path to the *.sfr file from IAR"),
                                 "sfr");
    parser.addOption(sfrOption);

    QCommandLineOption cosmicOption(QStringList() << "header",
                                    QCoreApplication::translate("main", "Path to the output header file"),
                                    "header");
    parser.addOption(cosmicOption);

    QCommandLineOption sdccOption(QStringList() << "type",
                                    QCoreApplication::translate("main", "Type of the generated header. Possible values: sdcc or cosmic"),
                                  "type",
                                  "cosmic");
    parser.addOption(sdccOption);

    QCommandLineOption ddfOption(QStringList() << "ddf",
                                 QCoreApplication::translate("main", "Path to the *.ddf file from IAR"),
                                 "ddf");
    parser.addOption(ddfOption);

    // Process the actual command line arguments given by the user
    parser.process(a);

    if (!parser.isSet("sfr")) {
        parser.showHelp(0);
    }

    if (!parser.isSet("header")) {

    } else {
        QFile sfrFile, header;
        sfrFile.setFileName(parser.value(sfrOption));
        header.setFileName(parser.value(cosmicOption));

        QFileInfo headerInfo(header);

        if (!header.open(QFile::WriteOnly)) {
            qWarning() << QObject::tr("Unable to open the %1 header file for reading").arg(parser.value(cosmicOption));
            return -1;
        }

        // IAR SFR format:
        // ;; sfr = "name", "zone", address, size, base=<base>[, bitRange=bit[-bit]]
        // ;;       [, readOnly/writeOnly]
        // sfr = "PA_IDR",            "Memory", 0x5001, 1, base=16, tooltip="Port A input pin value register"
        if (sfrFile.open(QFile::ReadOnly | QFile::Text)) {
            header.write(QString("#ifndef %1_H\n" \
                                 "#define %1_H\n\n" \
                                 "//Generated from %2\n\n")
                         .arg(headerInfo.baseName().toUpper())
                         .arg(sfrFile.fileName())
                         .toLocal8Bit());

            QTextStream stream(&sfrFile);
            bool lastSFR = true;
            QString bitFieldBaseAddr, bitFieldRegName;

            int structHeaderStaticFieldLength = 0;
            int registerLineStaticFieldLength = 0;

            if (parser.value("type") == "cosmic") {
                structHeaderStaticFieldLength = QString("volatile __BITS_  ").length();
                registerLineStaticFieldLength = QString("volatile unsigned char ").length();
            } else if (parser.value("type") == "sdcc") {
                structHeaderStaticFieldLength = QString("volatile _bits __at(0x)").length();
                registerLineStaticFieldLength = QString("volatile unsigned char __at(0x)").length();
            }

            while(!stream.atEnd()) {
                QString line = stream.readLine();
                if (line.startsWith("sfr =")) {
                    if (!line.contains("bitRange")) {
                        QString pad;
                        // this is a line describing a register
                        // like:
                        // sfr = "PA_ODR",            "Memory", 0x5000, 1, base=16, tooltip="Port A data output latch register"

                        if (!lastSFR) {
                            pad = pad.fill(' ', PADLENGTH - bitFieldRegName.length() - structHeaderStaticFieldLength);
                            if (parser.value("type") == "cosmic") {
                                header.write(QString("} __BITS_%1;\n").arg(bitFieldRegName).toLocal8Bit());
                                header.write(QString("volatile __BITS_%1 %1_bits%3@0x%2;\n\n\n")
                                             .arg(bitFieldRegName)
                                             .arg(bitFieldBaseAddr)
                                             .arg(pad).toLocal8Bit());
                            } else if (parser.value("type") == "sdcc") {
                                header.write(QString("} __BITS_%1;\n").arg(bitFieldRegName).toLocal8Bit());
                                header.write(QString("volatile __BITS_%1 __at(0x%2) %1_bits;\n\n\n")
                                             .arg(bitFieldRegName)
                                             .arg(bitFieldBaseAddr)
                                             .toLocal8Bit());
                            }
                        }

                        QRegularExpression re(".*=\\s\\\"(\\S*)\\\",\\s*\\\".*\\\",\\s0x([0-9A-F]*),.*tooltip=\\\"(.*)\\\"");
                        QRegularExpressionMatch match = re.match(line);
                        if (match.hasMatch()) {
                            QString matched = match.captured(0);
                            pad = pad.fill(' ', PADLENGTH - match.captured(1).length() - registerLineStaticFieldLength);
                            if (parser.value("type") == "cosmic") {
                                header.write(QString("volatile unsigned char %1%4@0x%2; // %3\n")
                                             .arg( match.captured(1)) // name
                                             .arg( match.captured(2)) // address
                                             .arg( match.captured(3)) // tooltip
                                             .arg(pad).toLocal8Bit());
                            } else if (parser.value("type") == "sdcc") {
                                header.write(QString("volatile unsigned char __at(0x%1) %2; // %3\n")
                                             .arg( match.captured(2)) // address
                                             .arg( match.captured(1)) // name
                                             .arg( match.captured(3)) // tooltip
                                             .toLocal8Bit());
                            }
                        }
                        lastSFR = true;
                    } else {
                        // this is a line describing a bitfield
                        // like:
                        // sfr = "PA_ODR.ODR0",       "Memory", 0x5000, 1, base=16, bitRange=0
                        if (lastSFR) {
                            header.write("typedef struct\n{\n");
                        }

                        QRegularExpression re("sfr\\s=\\s\\\"(\\S*)\\.(\\S*)\\\",\\s*\\\".*\\\",\\s0x([0-9A-F]*),.*bitRange=(.*)");
                        QRegularExpressionMatch match = re.match(line);
                        if (match.hasMatch()) {
                            QString pad;
                            bitFieldRegName = match.captured(1);
                            bitFieldBaseAddr = match.captured(3);
                            if (match.captured(4).contains('-')) {
                                // multi bit bitfield like:
                                // sfr = "ITC_SPR4.VECT12SPR", "Memory", 0x7F73, 1, base=16, bitRange=0-1
                                QStringList parts = match.captured(4).split('-');
                                int firstBitIndex = parts.first().toInt();
                                int lastBitIndex = parts.at(1).toInt();
                                pad = pad.leftJustified(PADLENGTH-match.captured(2).length() - QString("    unsigned char  : ").length(), ' ');
                                header.write(QString("    unsigned char %1%2 : %3;\n")
                                             .arg(match.captured(2))
                                             .arg(pad)
                                             .arg(lastBitIndex - firstBitIndex + 1)
                                             .toLocal8Bit());
                            } else {
                                // single bit bitfield like:
                                // sfr = "PA_ODR.ODR3",       "Memory", 0x5000, 1, base=16, bitRange=3
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

            if (parser.value("type") == "sdcc") {
                // generate the ISR vector offsets for SDCC
                if (parser.isSet(ddfOption)) {
                    QFile ddfFile;
                    ddfFile.setFileName(parser.value(ddfOption));

                    if (ddfFile.open(QFile::ReadOnly)) {
                        header.write("\n\n\n// Offsets for the interrupt vector table\n");
                        QTextStream ddfStream(&ddfFile);
                        while(!ddfStream.atEnd()) {
                            QString line = ddfStream.readLine();
                            if (line.startsWith("Interrupt =")) {
                                // Interrupt line example:
                                // Interrupt = AWU                   0x800C   3  AWU_CSR1.AWUEN        AWU_CSR1.AWUF         ITC_SPR1.VECT1SPR
                                // Interrupt = CLK_CSS               0x8010   4  CLK_CSSR.CSSDIE       CLK_CSSR.CSSD         ITC_SPR1.VECT2SPR
                                QRegularExpression re("Interrupt =\\s*(\\S*)\\s*0x([0-9A-F]*)\\s*([0-9]*).*");
                                QRegularExpressionMatch match = re.match(line);
                                if (match.hasMatch()) {
                                    QString pad;
                                    pad = pad.fill(' ', PADLENGTH - match.captured(1).length() - registerLineStaticFieldLength - 7);
                                    header.write(QString("#define %1_vector%2 %3\n")
                                                 .arg(match.captured(1)) // name
                                                 .arg(pad)
                                                 .arg(match.captured(3).toInt() - 2).toLocal8Bit()); // offset SDCC offsets are smaller with 2
                                }
                            }
                        }
                        ddfFile.close();
                    } else {
                        qWarning() << QObject::tr("Unable to open the %1 header file for reading").arg(parser.value(cosmicOption));
                    }
                }
            }

            header.write(QString("#endif // #ifdef %1_H").arg(headerInfo.baseName().toUpper()).toLocal8Bit());
        } else {
            qWarning() << QObject::tr("Unable to open the %1 sfr file for reading").arg(parser.value(sfrOption));
            header.close();
            return -1;
        }
        header.close();
        sfrFile.close();
    }
    return 0;
}
