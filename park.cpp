#define GL_SILENCE_DEPRECATION
#include <GL/glut.h>
#include <cmath>

float angleY   = 20.0f;
float angleX   = 25.0f;
float zoomDist = 100.0f;

int  lastMouseX = -1, lastMouseY = -1;
bool mouseDown  = false;

/* =============================
   GROUND (checker grass)
============================= */
void drawGround()
{
    int   tiles = 16;
    float size  = 140.0f;
    float tileW = size / tiles;
    float off   = -size * 0.5f;

    for (int i = 0; i < tiles; i++)
        for (int j = 0; j < tiles; j++) {
            if ((i + j) % 2 == 0) glColor3f(0.28f, 0.72f, 0.28f);
            else                   glColor3f(0.26f, 0.68f, 0.26f);
            float x0 = off + i * tileW, z0 = off + j * tileW;
            glBegin(GL_QUADS);
            glVertex3f(x0,       0.0f, z0);
            glVertex3f(x0+tileW, 0.0f, z0);
            glVertex3f(x0+tileW, 0.0f, z0+tileW);
            glVertex3f(x0,       0.0f, z0+tileW);
            glEnd();
        }
}

/* =============================
   ROAD  (dark asphalt + dashed centre line)
============================= */
void drawRoad(float x1, float z1, float x2, float z2)
{
    glColor3f(0.22f, 0.22f, 0.22f);
    glBegin(GL_QUADS);
    glVertex3f(x1, 0.03f, z1);
    glVertex3f(x2, 0.03f, z1);
    glVertex3f(x2, 0.03f, z2);
    glVertex3f(x1, 0.03f, z2);
    glEnd();

    bool  horiz = fabsf(x2 - x1) > fabsf(z2 - z1);
    float cx    = (x1 + x2) * 0.5f;
    float cz    = (z1 + z2) * 0.5f;
    float len   = horiz ? fabsf(x2 - x1) : fabsf(z2 - z1);
    int   dashes = (int)(len / 5);

    glColor3f(0.9f, 0.8f, 0.0f);
    for (int d = 0; d < dashes; d += 2) {
        glBegin(GL_QUADS);
        if (horiz) {
            float xa = x1 + d * 5.0f, xb = xa + 4.0f;
            glVertex3f(xa, 0.04f, cz - 0.15f);
            glVertex3f(xb, 0.04f, cz - 0.15f);
            glVertex3f(xb, 0.04f, cz + 0.15f);
            glVertex3f(xa, 0.04f, cz + 0.15f);
        } else {
            float za = z1 + d * 5.0f, zb = za + 4.0f;
            glVertex3f(cx - 0.15f, 0.04f, za);
            glVertex3f(cx + 0.15f, 0.04f, za);
            glVertex3f(cx + 0.15f, 0.04f, zb);
            glVertex3f(cx - 0.15f, 0.04f, zb);
        }
        glEnd();
    }
}

/* =============================
   TREE  (trunk + stacked cones)
============================= */
void drawTree(float x, float z)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    /* trunk */
    glColor3f(0.42f, 0.27f, 0.12f);
    glPushMatrix();
    glTranslatef(0, 1.5f, 0);
    glRotatef(-90, 1, 0, 0);
    GLUquadric* q = gluNewQuadric();
    gluCylinder(q, 0.25, 0.18, 3.0, 8, 1);
    gluDeleteQuadric(q);
    glPopMatrix();

    /* foliage — 3 stacked cones */
    for (int layer = 0; layer < 3; layer++) {
        glColor3f(0.12f, 0.55f - layer * 0.04f, 0.18f);
        glPushMatrix();
        glTranslatef(0, 3.0f + layer * 1.6f, 0);
        glRotatef(-90, 1, 0, 0);
        GLUquadric* qc = gluNewQuadric();
        gluCylinder(qc, 1.5f - layer * 0.35f, 0.0f, 2.0f, 8, 1);
        gluDeleteQuadric(qc);
        glPopMatrix();
    }

    glPopMatrix();
}

/* =============================
   FENCE  (posts + two rails)
============================= */
void drawFenceSegment(float x1, float z1, float x2, float z2)
{
    float dx   = x2 - x1, dz = z2 - z1;
    float len  = sqrtf(dx * dx + dz * dz);
    float ang  = atan2f(dx, dz) * 180.0f / (float)M_PI;
    int   posts = (int)(len / 3.0f) + 1;

    glPushMatrix();
    glTranslatef(x1, 0.0f, z1);
    glRotatef(ang, 0, 1, 0);

    for (int p = 0; p < posts; p++) {
        float px = p * (len / (posts - 1));

        /* post */
        glColor3f(0.65f, 0.45f, 0.20f);
        glPushMatrix();
        glTranslatef(0, 1.0f, px);
        glRotatef(-90, 1, 0, 0);
        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, 0.08, 0.08, 2.0, 6, 1);
        gluDeleteQuadric(q);
        glPopMatrix();

        /* post cap */
        glColor3f(0.55f, 0.35f, 0.12f);
        glPushMatrix();
        glTranslatef(0, 3.1f, px);
        glutSolidSphere(0.12, 6, 6);
        glPopMatrix();
    }

    /* two horizontal rails */
    glColor3f(0.60f, 0.42f, 0.18f);
    for (int r = 0; r < 2; r++) {
        float ry = (r == 0) ? 1.6f : 2.6f;
        glBegin(GL_QUADS);
        glVertex3f(-0.06f, ry, 0);
        glVertex3f( 0.06f, ry, 0);
        glVertex3f( 0.06f, ry, len);
        glVertex3f(-0.06f, ry, len);
        glEnd();
    }

    glPopMatrix();
}

/* =============================
   DISPLAY
============================= */
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float radX = angleX * (float)M_PI / 180.0f;
    float radY = angleY * (float)M_PI / 180.0f;
    float ex   = zoomDist * cosf(radX) * sinf(radY);
    float ey   = zoomDist * sinf(radX);
    float ez   = zoomDist * cosf(radX) * cosf(radY);
    gluLookAt(ex, ey, ez,  0, 0, 0,  0, 1, 0);

    drawGround();

    /* ---- Horizontal roads ---- */
    drawRoad(-55, -22,  55, -16);
    drawRoad(-55,   0,  55,   6);
    drawRoad(-55,  22,  55,  28);

    /* ---- Vertical roads ---- */
    drawRoad(-22, -55, -16,  55);
    drawRoad(  0, -55,   6,  55);
    drawRoad( 22, -55,  28,  55);

    /* ---- Perimeter fence ---- */
    float F = 62.0f;
    drawFenceSegment(-F, -F,  F, -F);   /* south */
    drawFenceSegment( F, -F,  F,  F);   /* east  */
    drawFenceSegment( F,  F, -F,  F);   /* north */
    drawFenceSegment(-F,  F, -F, -F);   /* west  */

    /* ---- Perimeter trees (inside fence) ---- */
    float T = 57.0f;
    for (int i = -5; i <= 5; i++) {
        drawTree(-T,  i * 11.0f);
        drawTree( T,  i * 11.0f);
        drawTree( i * 11.0f, -T);
        drawTree( i * 11.0f,  T);
    }

    /* ---- Trees lining the vertical roads ---- */
    for (int i = -4; i <= 4; i++) {
        drawTree(-23, i * 11.0f);
        drawTree(-13, i * 11.0f);
        drawTree( -3, i * 11.0f);
        drawTree(  9, i * 11.0f);
        drawTree( 19, i * 11.0f);
        drawTree( 31, i * 11.0f);
    }

    glutSwapBuffers();
}

/* =============================
   RESHAPE
============================= */
void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55, (float)w / h, 1, 500);
    glMatrixMode(GL_MODELVIEW);
}

/* =============================
   KEYBOARD
============================= */
void keyboard(unsigned char key, int, int)
{
    if (key == '+' || key == '=') zoomDist -= 5.0f;
    if (key == '-')               zoomDist += 5.0f;
    zoomDist = fmaxf(20.0f, fminf(250.0f, zoomDist));
    glutPostRedisplay();
}

void specialKeys(int key, int, int)
{
    if (key == GLUT_KEY_LEFT)  angleY -= 3.0f;
    if (key == GLUT_KEY_RIGHT) angleY += 3.0f;
    if (key == GLUT_KEY_UP)    angleX = fminf(angleX + 2.0f, 85.0f);
    if (key == GLUT_KEY_DOWN)  angleX = fmaxf(angleX - 2.0f,  5.0f);
    glutPostRedisplay();
}

/* =============================
   MOUSE DRAG + SCROLL
============================= */
void mouseButton(int btn, int state, int x, int y)
{
    if (btn == GLUT_LEFT_BUTTON) {
        mouseDown  = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
    }
    if (btn == 3) { zoomDist -= 3.0f; glutPostRedisplay(); }
    if (btn == 4) { zoomDist += 3.0f; glutPostRedisplay(); }
}

void mouseMotion(int x, int y)
{
    if (!mouseDown) return;
    int dx = x - lastMouseX, dy = y - lastMouseY;
    angleY += dx * 0.4f;
    angleX  = fmaxf(5.0f, fminf(85.0f, angleX - dy * 0.35f));
    lastMouseX = x;
    lastMouseY = y;
    glutPostRedisplay();
}

/* =============================
   INIT
============================= */
void init()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.42f, 0.72f, 0.98f, 1.0f);
}

/* =============================
   MAIN
============================= */
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 750);
    glutCreateWindow("3D Amusement Park — Base Layout");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);

    glutMainLoop();
    return 0;
}