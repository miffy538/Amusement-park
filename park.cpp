/*
 Amusement Park Layout - macOS version
 Compile:
 g++ amusement_park_layout.cpp -o park -framework OpenGL -framework GLUT -Wno-deprecated
 ./park
*/

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include <cmath>
#include <vector>
#include <cstring>

const int W = 1000;
const int H = 720;


// ==============================
// ISOMETRIC TRANSFORM
// ==============================

const float TILE  = 54.f;
const float TILEY = 27.f;

const float OX = W * 0.5f;
const float OY = H * 0.84f;

struct P2
{
    float x;
    float y;
};


P2 g2s(float gc, float gr)
{
    P2 p;

    p.x = OX + (gc - gr) * TILE;
    p.y = OY - (gc + gr) * TILEY;

    return p;
}



// ==============================
// COLOR
// ==============================

void col(float r, float g, float b)
{
    glColor3f(r, g, b);
}



// ==============================
// DRAW POLYGON
// ==============================

void screenPoly(const std::vector<P2>& pts, float r, float g, float b)
{
    col(r, g, b);

    glBegin(GL_POLYGON);

    for (auto& p : pts)
        glVertex2f(p.x, p.y);

    glEnd();
}



// ==============================
// ISO TILE
// ==============================

void isoTile(float gc, float gr, float w, float h,
             float r, float g, float b)
{
    P2 l = g2s(gc,     gr + h);
    P2 t = g2s(gc + w, gr + h);
    P2 r2 = g2s(gc + w, gr);
    P2 b2 = g2s(gc,     gr);

    screenPoly({l, t, r2, b2}, r, g, b);
}



// ==============================
// ROAD
// ==============================

void road(float gc1, float gr1,
          float gc2, float gr2,
          float width = 0.36f)
{
    float dx = gc2 - gc1;
    float dy = gr2 - gr1;

    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;

    float nx =  dy / len * width;
    float ny = -dx / len * width;

    P2 a = g2s(gc1 - nx, gr1 - ny);
    P2 b = g2s(gc1 + nx, gr1 + ny);
    P2 c = g2s(gc2 + nx, gr2 + ny);
    P2 d = g2s(gc2 - nx, gr2 - ny);

    col(0.93f, 0.80f, 0.44f);

    glBegin(GL_QUADS);

    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glVertex2f(c.x, c.y);
    glVertex2f(d.x, d.y);

    glEnd();
}



void windRoad(const std::vector<std::pair<float, float>>& pts,
              float w = 0.36f)
{
    for (int i = 0; i + 1 < pts.size(); i++)
    {
        road(
            pts[i].first,
            pts[i].second,
            pts[i + 1].first,
            pts[i + 1].second,
            w
        );
    }
}



// ==============================
// ISO BOX
// ==============================

void isoBox(
    float gc, float gr,
    float w, float d,
    float h,

    float rt, float gt, float bt,
    float rf, float gf, float bf,
    float rr, float gr2, float br2
)
{
    P2 fl = g2s(gc, gr);
    P2 fr = g2s(gc + w, gr);

    P2 bl = g2s(gc, gr + d);
    P2 br = g2s(gc + w, gr + d);

    auto up = [&](P2 p)
    {
        P2 q;
        q.x = p.x;
        q.y = p.y + h;
        return q;
    };

    P2 Ufl = up(fl);
    P2 Ufr = up(fr);
    P2 Ubl = up(bl);
    P2 Ubr = up(br);


    screenPoly({Ubl, Ubr, Ufr, Ufl}, rt, gt, bt);
    screenPoly({Ufl, Ufr, fr, fl},   rf, gf, bf);
    screenPoly({Ufr, Ubr, br, fr},   rr, gr2, br2);
}



// ==============================
// ZONE
// ==============================

void zone(
    float gc,
    float gr,
    float w,
    float d,
    const char* lbl
)
{
    float h = 22.f;

    isoBox(
        gc, gr, w, d, h,

        0.42f, 0.66f, 0.24f,
        0.30f, 0.50f, 0.16f,
        0.24f, 0.42f, 0.12f
    );


    P2 mid = g2s(gc + w * 0.5f,
                 gr + d * 0.5f);

    mid.y += 26;


    glColor3f(0, 0, 0);

    glRasterPos2f(
        mid.x - strlen(lbl) * 3,
        mid.y
    );

    for (const char* c = lbl; *c; c++)
        glutBitmapCharacter(
            GLUT_BITMAP_HELVETICA_10,
            *c
        );
}



// ==============================
// DISPLAY
// ==============================

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);



    // SKY

    col(0.76f, 0.87f, 1.0f);

    glBegin(GL_QUADS);

    glVertex2f(0, 0);
    glVertex2f(W, 0);
    glVertex2f(W, H);
    glVertex2f(0, H);

    glEnd();



    // GROUND

    isoTile(
        -0.2f,
        -0.2f,
        10.4f,
        10.4f,

        0.30f,
        0.55f,
        0.14f
    );


    isoTile(
        0.3f,
        0.3f,
        9.4f,
        9.4f,

        0.40f,
        0.70f,
        0.20f
    );



    // ROADS

    // Main spine: entrance (right side) winding through park center to back
    windRoad({
        {5.0f, 0.3f},
        {5.0f, 1.5f},
        {4.5f, 2.5f},
        {4.2f, 3.5f},
        {3.8f, 5.0f},
        {4.0f, 6.5f},
        {4.3f, 7.5f}
    });

    // Left branch: Carousel -> Track -> Roller zones
    windRoad({
        {4.2f, 3.5f},
        {2.8f, 3.2f},
        {1.5f, 3.8f},
        {1.2f, 5.2f},
        {1.0f, 6.8f},
        {1.3f, 7.8f}
    });

    // Right branch: Cars -> Stage zones
    windRoad({
        {4.2f, 3.5f},
        {5.8f, 3.2f},
        {7.2f, 4.2f},
        {7.8f, 5.5f},
        {7.5f, 7.2f}
    });

    // Shops branch (top-left area)
    windRoad({
        {5.0f, 1.5f},
        {3.2f, 1.5f},
        {2.0f, 2.0f}
    });

    // Food branch (top-right area)
    windRoad({
        {5.0f, 1.5f},
        {6.8f, 1.5f},
        {7.5f, 2.2f}
    });



    // ZONES

    zone(0.3f,7.2f,2.4f,2,"Roller");
    zone(3.5f,7.2f,2.4f,2,"Lake");
    zone(6.8f,6.9f,2.5f,2,"Stage");

    zone(0.3f,4.8f,2.1f,2,"Track");
    zone(0.4f,3.0f,1.9f,2,"Carousel");

    zone(2.8f,5.0f,2.5f,2,"Circus");
    zone(6.8f,4.6f,2.3f,2,"Cars");

    zone(1.0f,1.3f,2.0f,2.0f,"Shops");
    zone(6.4f,1.3f,2,2,"Food");



    glutSwapBuffers();
}



// ==============================
// RESHAPE
// ==============================

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(0, W, 0, H);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}



// ==============================
// MAIN
// ==============================

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE | GLUT_RGB
    );

    glutInitWindowSize(W, H);

    glutCreateWindow(
        "Amusement Park Layout"
    );

    glClearColor(
        0.76f,
        0.87f,
        1.0f,
        1.0f
    );

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glutMainLoop();

    return 0;
}