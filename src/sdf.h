#include <QImage>

class SDF : public QImage
{
private:
    int method;
    bool test;
    QImage sdf;
    void calculate_bruteforce();
    void calculate_sed(int method);
public:
    enum {METHOD_BRUTEFORCE, METHOD_4SED, METHOD_8SED, TEST_MODE};
    SDF(const QString & fileName, const char * format = 0);
    void setMethod(int m)
    {
        method = m;
    }
    void setTest(bool t)
    {
        test = t;
    }
    QImage *calculate();
};
