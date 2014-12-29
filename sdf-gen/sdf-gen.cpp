#include <QtCore>
#include "sdf-gen.h"
#include "../src/sdf.h"

SDFGenerator::SDFGenerator(int & argc, char ** argv):QCoreApplication(argc, argv)
{
}

void SDFGenerator::help()
{
    QFileInfo fi(arguments().at(0));
    qDebug(_("Usage: %s [options] FILE..."), fi.baseName().toStdString().c_str());
    qDebug() << _("This tool converts grayscale images into SDF (Digned Distance Field)") << '\n';
    qDebug() << _("Options:");
    qDebug() << _("  -t, --test         Generate image for tests");
    qDebug() << _("  -b, --brute-force  Use exact brute-force algorithm (SLOW)");
    qDebug() << _("  -4, --4sed         Use very fast 4SED algorithm (this is default)");
    qDebug() << _("  -8, --8sed         Use fast 8SED algorithm (better quality with reasonable performance)");
}

int SDFGenerator::exec()
{
    QStringList args = arguments();
    bool files = false;
    int scale = 1;
    int method = SDF::METHOD_4SED;
    for(int i = 1; i < args.size(); i++)
    {
        QString arg = args.at(i);if(arg == "-b" || arg == "--brute-force" )
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
                files = true;
                SDF img(arg);
                if(img.isNull())
                {
                    qWarning() << _("Cannot open file") << arg;
                }
                else
                {
                    QString name(fi.path() + QDir::separator() + fi.baseName() + "-sdf.png");
                    img.setScale(scale);
                    img.setMethod(method);
                    img.calculate()->save(name);
                }
            }
            else
            {
                qWarning() << _("Cannot open file") << arg;
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
