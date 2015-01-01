#include <QCoreApplication>

class SDFGenerator : public QCoreApplication
{
Q_OBJECT
private:
    void help();
public:
    SDFGenerator(int & argc, char ** argv);
    int exec();
};
