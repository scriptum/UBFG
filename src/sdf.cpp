/* http://www.codersnotes.com/algorithms/signed-distance-fields */

#include <QtCore>
#include "sdf.h"

SDF::SDF(const QString & fileName, const char * format) : 
    QImage(fileName, format), 
    method(METHOD_4SED), 
    scale(1), 
    sdf(size(), QImage::Format_Indexed8)
{
    for(int i = 0; i < 256; i++)
    {
        sdf.setColor(i, qRgb(i, i, i));
    }
}

struct Point
{
    short dx, dy;
    int f;
};

struct Grid
{
    int w, h;
    Point *grid;
    Grid(int width, int height) : w(width), h(height)
    {
        grid = new Point[(w + 2) * (h + 2)];
    }
    ~Grid()
    {
        delete[] grid;
    }
};

static const Point pointInside = { 0, 0, 0 };
static const Point pointEmpty = { SHRT_MAX, SHRT_MAX, INT_MAX/2 };

static inline Point Get(Grid &g, int x, int y)
{
    return g.grid[y * (g.w + 2) + x];
}

static inline void Put(Grid &g, int x, int y, const Point &p)
{
    g.grid[y * (g.w + 2) + x] = p;
}

static inline void Compare(Grid &g, int x, int y, int offsetx, int offsety)
{
    int add;
    Point other = Get(g, x + offsetx, y + offsety);
    if(offsety == 0) {
        add = 2 * other.dx + 1;
    }
    else if(offsetx == 0) {
        add = 2 * other.dy + 1;
    }
    else {
        add = 2 * (other.dy + other.dx + 1);
    }
    other.f += add;
    if (other.f < Get(g, x, y).f)
    {
        if(offsety == 0) {
            other.dx++;
        }
        else if(offsetx == 0) {
            other.dy++;
        }
        else {
            other.dy++;
            other.dx++;
        }
        Put(g, x, y, other);
    }
}

#define COMPARE(offsetx, offsety) Compare(g, x, y, offsetx, offsety)

#define CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

static void Generate8SED(Grid &g)
{
    /* forward scan */
    for(int y = 1; y <= g.h; y++)
    {
        for(int x = 1; x <= g.w; x++)
        {
            COMPARE( 0, -1);
            COMPARE(-1,  0);
            COMPARE(-1, -1);
        }
        for(int x = g.w; x >= 0; x--)
        {
            COMPARE( 1,  0);
            COMPARE( 1, -1);
        }
    }

    /* backward scan */
    for(int y = g.h - 1; y > 0; y--)
    {
        for(int x = 1; x <= g.w; x++)
        {
            COMPARE( 0,  1);
            COMPARE(-1,  0);
            COMPARE(-1,  1);
        }
        for(int x = g.w - 1; x > 0; x--)
        {
            COMPARE( 1,  0);
            COMPARE( 1,  1);
        }
    }
}
static void Generate4SED(Grid &g)
{
    /* forward scan */
    for(int y = 1; y <= g.h; y++)
    {
        for(int x = 1; x <= g.w; x++)
        {
            COMPARE( 0, -1);
            COMPARE(-1,  0);
        }
        for(int x = g.w - 1; x > 0; x--)
        {
            COMPARE( 1,  0);
        }
    }

    /* backward scan */
    for(int y = g.h - 1; y > 0; y--)
    {
        for(int x = 1; x <= g.w; x++)
        {
            COMPARE( 0,  1);
            COMPARE(-1,  0);
        }
        for(int x = g.w - 1; x > 0; x--)
        {
            COMPARE( 1,  0);
        }
    }
}

void SDF::calculate_sed(int method)
{
    int w = width(), h = height();
    Grid grid(w, h);
    /* create 1-pixel gap */
    for(int x = 0; x < w + 2; x++)
    {
        Put(grid, x, 0, pointEmpty);
        Put(grid, x, h + 1, pointEmpty);
    }
    uchar *data = bits();
    uchar pixel = bytesPerLine() / w;
    #pragma omp parallel for
    for(int y = 1; y <= h; y++)
    {
        Put(grid, 0, y, pointEmpty);
        for(int x = 1; x <= w; x++)
        {
            if(data[((y - 1) * w + (x - 1)) * pixel] > 128)
            {
                Put(grid, x, y, pointInside);
            }
            else
            {
                Put(grid, x, y, pointEmpty);
            }
        }
        Put(grid, w + 1, y, pointEmpty);
    }
    if(method == METHOD_4SED)
        Generate4SED(grid);
    else
        Generate8SED(grid);
    data  = sdf.bits();
    #pragma omp parallel for
    for(int y = 1; y <= h; y++)
    {
        for(int x = 1; x <= w; x++)
        {
            qreal dist2 = qSqrt((qreal)Get(grid, x, y).f);
            int c = dist2 * scale;
            data[(y - 1) * w + (x - 1)] = CLAMP(c, 0, 255);
        }
    }
}

void SDF::calculate_bruteforce()
{
    int w = width(), h = height();
    QVector< QPair<short, short> > cache;
    for(short y = 0; y < h; y++)
    {
        for(short x = 0; x < w; x++)
        {
            if(qGreen(pixel(x, y)) > 128)
                cache.append({x,y});
        }
    }
    uchar *data  = sdf.bits();
    #pragma omp parallel for
    for(short y = 0; y < h; y++)
    {
        for(short x = 0; x < w; x++)
        {
            int min = INT_MAX;
            for(int l = 0; l < cache.size(); l++)
            {
                int i = cache.at(l).first - x;
                int j = cache.at(l).second - y;
                int d = i * i + j * j;
                if(d < min)
                {
                    min = d;
                }
            }
            int c = sqrtf(min) * scale;
            data[y * w + x] = CLAMP(c, 0, 255);
        }
    }
}
QImage *SDF::calculate()
{
    switch(method)
    {
        case METHOD_BRUTEFORCE:
            calculate_bruteforce();
            break;
        case METHOD_4SED:
            calculate_sed(method);
            break;
        case METHOD_8SED:
            calculate_sed(method);
            break;
    }
    return &sdf;
}
