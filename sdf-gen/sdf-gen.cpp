#include <QtCore>
#include "sdf-gen.h"
#include "../src/sdf.h"

SDFGenerator::SDFGenerator(int & argc, char ** argv):QCoreApplication(argc, argv)
{
}

void SDFGenerator::help()
{
    QFileInfo fi(arguments().at(0));
    QTextStream out(stderr);
    out << tr("Usage: %1 [options] FILE...").arg(fi.baseName()) << endl;
    out << tr("This tool converts grayscale images into SDF (Signed Distance Field)") << endl << endl;
    out << tr("Options:") << endl;
    out << tr("  -b, --brute-force  Use exact brute-force algorithm (SLOW)") << endl;
    out << tr("  -4, --4sed         Use very fast 4SED algorithm (this is default)") << endl;
    out << tr("  -8, --8sed         Use fast 8SED algorithm (better quality with reasonable performance)") << endl;
}

int SDFGenerator::exec()
{
    QStringList args = arguments();
    bool files = false;
    int scale = 8;
    int method = SDF::METHOD_4SED;
    QTranslator translator;
    translator.load(":/" + QLocale::system().name());
    installTranslator(&translator);
    for(int i = 1; i < args.size(); i++)
    {
        QString arg = args.at(i);
        if(arg == "-b" || arg == "--brute-force" )
        {
            method = SDF::METHOD_BRUTEFORCE;
        }
        else if(arg == "-4" || arg == "--4sed" )
        {
            method = SDF::METHOD_4SED;
        }
        else if(arg == "-8" || arg == "--8sed" )
        {
            method = SDF::METHOD_8SED;
        }
        else
        {
            QFileInfo fi(arg);
            if(fi.isFile())
            {
                SDF img(arg);
                if(img.isNull())
                {
                    QTextStream(stderr) << tr("Cannot open file") << ' ' << '"'  << arg << '"'  << endl;
                }
                else
                {
                    files = true;
                    QString name(fi.path() + QDir::separator() + fi.baseName() + "-sdf.png");
                    img.setScale(scale);
                    img.setMethod(method);
                    img.calculate()->save(name);
                }
            }
            else
            {
                QTextStream(stderr) << tr("Cannot open file") << ' ' << '"' << arg << '"'  << endl;
            }
        }
    }
    if(!files)
    {
        help();
        return -1;
    }
    return 0;
}
