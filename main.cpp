/*  ============================================================
    3D Amusement Park — FUNLAND ENHANCED VERSION
    CHANGES FROM ORIGINAL:
      - Camera starts facing entrance gate (south view)
      - "FUNLAND" entrance gate — big, colorful, decorated
      - Clean non-overlapping road system with proper widths
      - Circular roundabout road around the grand fountain
      - Food court: distinct stalls + nicely arranged tables/chairs/umbrellas
      - Solid perimeter boundary wall (not just fence)
      - Roads stay within boundary, no overlap with rides/stalls
      - Improved zone layout and decorative elements
    Controls:
      Arrow keys   = orbit
      +/-          = zoom
      Mouse drag   = orbit
      Scroll wheel = zoom
      W/R/C        = toggle wheel / coaster / merry-go-round
    ============================================================ */

#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

/* ─────────────────────────────
   GLOBALS
───────────────────────────── */
/* Camera starts looking from SOUTH toward the entrance gate */
float angleY = 0.0f;    /* face north (toward gate) */
float angleX = 18.0f;   /* slight elevation */
float zoomDist = 110.0f;

int   lastMX=-1, lastMY=-1;
bool  mouseDown=false;

float merryAngle=0, wheelAngle=0, coasterProg=0;
float windTime=0, walkTime=0, fountainTime=0;
float spotAngle=0,ledTime=0;

bool wheelSpinning=true, coasterRunning=true, merryRunning=true;

float trainAngle = 0.0f;
bool trainRunning = true;
float trainSpeed = 1.0f;
float wheelRotation = 0.0f;

float pirateAngle = 0.0f;
bool pirateMoving = true;


GLuint groundTex=0;


/* ─────────────────────────────
   BMP LOADER
───────────────────────────── */
unsigned char* loadBMP(const char* fn,int*w,int*h)
{
        FILE* f = fopen(fn,"rb");
    if(!f){
        printf("BMP not found: %s\n",fn);
        return nullptr;
    }

    unsigned char hdr[54];
    fread(hdr,1,54,f);

    *w = *(int*)&hdr[18];
    *h = *(int*)&hdr[22];

    int row_padded = ((*w * 3 + 3) & (~3));
    unsigned char* data = new unsigned char[row_padded * (*h)];
    fread(data,1,row_padded * (*h),f);
    fclose(f);

    unsigned char* finalData = new unsigned char[(*w)*(*h)*3];

    for(int i=0;i<*h;i++){
        for(int j=0;j<*w;j++){
            for(int k=0;k<3;k++){
                finalData[(((*h-1-i)*(*w)+j)*3)+k] =
                    data[i*row_padded + j*3 + (2-k)];
            }
        }
    }

    delete[] data;
    return finalData;
}
GLuint createTexture(const char* file){
    int w,h;
    unsigned char* data = loadBMP(file,&w,&h);
    if(!data) return 0;

    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);


    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,data);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    delete[] data;
    return tex;
}

GLuint pizzaTex;
 GLuint burgerTex;
  GLuint iceTex;
GLuint waterTex;

float waterTime = 0.0f;
/* ─────────────────────────────
   LIGHTING
───────────────────────────── */
void setupLighting()
{
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
    GLfloat a0[]={0.35f,0.32f,0.28f,1}, d0[]={1.00f,0.96f,0.84f,1}, p0[]={80,150,100,0};
    glLightfv(GL_LIGHT0,GL_AMBIENT,a0); glLightfv(GL_LIGHT0,GL_DIFFUSE,d0);
    glLightfv(GL_LIGHT0,GL_POSITION,p0);
    GLfloat d1[]={0.28f,0.36f,0.50f,1}, p1[]={-50,40,-70,0};
    glLightfv(GL_LIGHT1,GL_DIFFUSE,d1); glLightfv(GL_LIGHT1,GL_POSITION,p1);
    GLfloat ga[]={0.20f,0.20f,0.22f,1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ga);
}

/* ─────────────────────────────
   SKY GRADIENT
───────────────────────────── */
void drawSky()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(-1,1,-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3f(0.10f,0.38f,0.88f); glVertex2f(-1,1); glVertex2f(1,1);
    glColor3f(0.60f,0.82f,1.00f); glVertex2f(1,-1); glVertex2f(-1,-1);
    glEnd();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

/* ─────────────────────────────
   CLOUDS
───────────────────────────── */
void drawCloudPuff(float x,float y,float z,float sc)
{
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,0.88f);
    GLUquadric*q=gluNewQuadric();
    float ox[]={0,-1.4f,1.4f,0,0.8f};
    float oy[]={0,0.3f,0.2f,1.1f,-0.2f};
    float rad[]={1.6f,1.2f,1.3f,1.0f,0.9f};
    for(int i=0;i<5;i++){
        glPushMatrix(); glTranslatef(x+ox[i]*sc,y+oy[i]*sc,z);
        gluSphere(q,rad[i]*sc,10,10); glPopMatrix();
    }
    gluDeleteQuadric(q);
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);
}
void drawClouds(){
    drawCloudPuff(-80,55,-60,3.0f); drawCloudPuff(-20,65,-100,2.5f);
    drawCloudPuff(40,58,-80,3.5f);  drawCloudPuff(90,60,-50,2.8f);
    drawCloudPuff(-50,70,-130,2.2f);drawCloudPuff(20,48,-40,2.0f);
    drawCloudPuff(60,62,-110,3.2f); drawCloudPuff(-30,52,-55,2.0f);
}

void drawWheel(float radius = 0.5f, float width = 0.2f) {
    GLUquadric* q = gluNewQuadric();

    glColor3f(0.05f, 0.05f, 0.05f); // dark grey

    glRotatef(wheelRotation, 0, 0, 1);

    // Outer wheel
    glutSolidTorus(0.12, radius, 30, 30);

    // Inner disk (fills hollow look)
    glPushMatrix();
    glRotatef(90, 1, 0, 0);
    gluDisk(q, 0, radius - 0.1f, 20, 1);
    glPopMatrix();

    gluDeleteQuadric(q);
}

void drawEngine() {
    GLUquadric* q = gluNewQuadric();

    // ======================
    // MAIN BODY
    // ======================
    glColor3f(0.6f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0.5f, 0.6f, 0);
    glRotatef(90, 0, 1, 0);
    gluCylinder(q, 0.6, 0.6, 2.0, 30, 1);
    glPopMatrix();

    // FRONT (rounded)
    glPushMatrix();
    glTranslatef(2.5f, 0.6f, 0);
    glutSolidSphere(0.6, 30, 30);
    glPopMatrix();

    // BASE
    glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix();
    glTranslatef(0.5f, -0.3f, 0);
    glScalef(3.2f, 0.6f, 1.8f);
    glutSolidCube(1);
    glPopMatrix();

    // CABIN
    glColor3f(0.5f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(-0.8f, 1.2f, 0);
    glScalef(1.3f, 1.3f, 1.6f);
    glutSolidCube(1);
    glPopMatrix();

    // CHIMNEY
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(1.5f, 1.4f, 0);
    gluCylinder(q, 0.25, 0.2, 0.8, 20, 1);
    glPopMatrix();

    // HEADLIGHT
    glColor3f(1.0f, 0.9f, 0.5f);
    glPushMatrix();
    glTranslatef(3.0f, 0.6f, 0);
    glutSolidSphere(0.25, 20, 20);
    glPopMatrix();

    // WHEELS
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * 1.3f, -0.7f, 0.8f);
        drawWheel(0.5f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(i * 1.3f, -0.7f, -0.8f);
        drawWheel(0.5f);
        glPopMatrix();
    }

    gluDeleteQuadric(q);
}
void drawWagon() {

    // BODY
    glColor3f(0.7f, 0.2f, 0.2f);
    glPushMatrix();
    glScalef(2.6f, 0.9f, 1.6f);
    glutSolidCube(1);
    glPopMatrix();

    // TOP STRIP
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(0, 0.6f, 0);
    glScalef(2.7f, 0.2f, 1.7f);
    glutSolidCube(1);
    glPopMatrix();

    // SEATS
  glColor3f(0.9f, 0.8f, 0.3f);
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * 0.7f, 0.6f, 0);
        glScalef(0.9f, 0.7f, 1.2f);
        glutSolidCube(1);
        glPopMatrix();
    }

    // SUPPORT RODS
    glColor3f(0.2f, 0.2f, 0.2f);
    for (int i = -1; i <= 1; i += 2) {
        for (int j = -1; j <= 1; j += 2) {
            glPushMatrix();
            glTranslatef(i * 1.1f, 1.2f, j * 0.7f);
            glScalef(0.1f, 1.6f, 0.1f);
            glutSolidCube(1);
            glPopMatrix();
        }
    }

    // ROOF
    glColor3f(0.6f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0, 1.9f, 0);
    glScalef(2.7f, 0.5f, 1.7f);
    glutSolidCube(1);
    glPopMatrix();

    // WHEELS
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * 1.3f, -0.8f, 0.75f);
        drawWheel(0.35f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(i * 1.3f, -0.8f, -0.75f);
        drawWheel(0.35f);
        glPopMatrix();
    }
}
void drawTrain() {
    float radius = 12.5f;
    float spacing = 0.7f;

    // ENGINE
    float x = radius * cos(trainAngle);
    float z = radius * sin(trainAngle);

    glPushMatrix();
    glTranslatef(x, 2.3f, z);
    glRotatef(-trainAngle * 180 / 3.14159f - 90, 0, 1, 0);
    glScalef(1.8f, 1.8f, 1.8f);
    drawEngine();
    glPopMatrix();

    // WAGONS
    for (int i = 1; i <= 3; i++) {
        float wagonAngle = trainAngle - i * spacing;

        float wx = radius * cos(wagonAngle);
        float wz = radius * sin(wagonAngle);

        glPushMatrix();
        glTranslatef(wx, 3.0f, wz);
        glRotatef(-wagonAngle * 180 / 3.14159f - 90, 0, 1, 0);
        glScalef(2.5f, 2.5f, 2.5f);

        drawWagon();

        // connector
        glColor3f(0.2f, 0.2f, 0.2f);
        glPushMatrix();
        glTranslatef(-2.0f, 0.3f, 0);
        glScalef(1.0f, 0.1f, 0.1f);
        glutSolidCube(1);
        glPopMatrix();

        glPopMatrix();
    }
}

void drawTrainTrack() {
    float radius = 12.5f;
    float railOffset = 0.9f;

    GLUquadric* q = gluNewQuadric();

    // ======================
    // SLEEPERS (WOOD)
    // ======================
    glColor3f(0.45f, 0.25f, 0.1f);

    for (int i = 0; i < 360; i += 8) {   // less dense = realistic
        float angle = i * 3.14159f / 180;

        float x = radius * cos(angle);
        float z = radius * sin(angle);

        glPushMatrix();
        glTranslatef(x, 0.05f, z);
        glRotatef(-i, 0, 1, 0);

        // slightly thicker + wider
        glScalef(2.5f, 0.15f, 0.5f);
        glutSolidCube(1);

        glPopMatrix();
    }

    // ======================
    // RAILS (REAL CYLINDERS)
    // ======================
    glColor3f(0.6f, 0.6f, 0.6f);

    for (int i = 0; i < 360; i += 2) {
        float angle = i * 3.14159f / 180;
        float nextAngle = (i + 2) * 3.14159f / 180;

        // OUTER RAIL
        float x1 = (radius + railOffset) * cos(angle);
        float z1 = (radius + railOffset) * sin(angle);

        float x2 = (radius + railOffset) * cos(nextAngle);
        float z2 = (radius + railOffset) * sin(nextAngle);

        glPushMatrix();
        glTranslatef(x1, 0.2f, z1);

        float dx = x2 - x1;
        float dz = z2 - z1;
        float len = sqrt(dx * dx + dz * dz);

        float angleRot = atan2(dz, dx) * 180 / 3.14159f;

        glRotatef(-angleRot, 0, 1, 0);
        glRotatef(90, 0, 0, 1);

        gluCylinder(q, 0.08, 0.08, len, 10, 1);
        glPopMatrix();

        // INNER RAIL
        x1 = (radius - railOffset) * cos(angle);
        z1 = (radius - railOffset) * sin(angle);

        x2 = (radius - railOffset) * cos(nextAngle);
        z2 = (radius - railOffset) * sin(nextAngle);

        glPushMatrix();
        glTranslatef(x1, 0.2f, z1);

        dx = x2 - x1;
        dz = z2 - z1;
        len = sqrt(dx * dx + dz * dz);

        angleRot = atan2(dz, dx) * 180 / 3.14159f;

        glRotatef(-angleRot, 0, 1, 0);
        glRotatef(90, 0, 0, 1);

        gluCylinder(q, 0.08, 0.08, len, 10, 1);
        glPopMatrix();
    }

    gluDeleteQuadric(q);
}
/* ─────────────────────────────
   HILLS
───────────────────────────── */
void drawHills(){
    glDisable(GL_LIGHTING);
    for(int l=0;l<4;l++){
        float z=-210-l*45, h=38-l*7;
        float gr=0.18f+l*0.04f, gg=0.38f+l*0.03f, gb=0.12f+l*0.02f;
        glColor3f(gr,gg,gb);
        glBegin(GL_TRIANGLE_STRIP);
        for(float x=-500;x<=500;x+=4){
            float y=h+9*sinf(x*0.018f+l*1.1f)+5*sinf(x*0.041f+l*0.7f);
            glVertex3f(x,0,z); glVertex3f(x,y,z);
        }
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

/* ─────────────────────────────
   GROUND
───────────────────────────── */
void drawZoneFloor(float x1,float z1,float x2,float z2,float r,float g,float b,float alpha){
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r,g,b,alpha);
    glBegin(GL_QUADS);
    glVertex3f(x1,0.04f,z1); glVertex3f(x2,0.04f,z1);
    glVertex3f(x2,0.04f,z2); glVertex3f(x1,0.04f,z2);
    glEnd();
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);
}

void drawGround(){
     
    const float F=65;
    glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);
    /* Dirt outside boundary */
    glColor3f(0.42f,0.28f,0.14f);
    glBegin(GL_QUADS);
    glVertex3f(-500,-0.05f,-500);glVertex3f(-F,-0.05f,-500);glVertex3f(-F,-0.05f,500);glVertex3f(-500,-0.05f,500);
    glVertex3f(F,-0.05f,-500);glVertex3f(500,-0.05f,-500);glVertex3f(500,-0.05f,500);glVertex3f(F,-0.05f,500);
    glVertex3f(-F,-0.05f,-500);glVertex3f(F,-0.05f,-500);glVertex3f(F,-0.05f,-F);glVertex3f(-F,-0.05f,-F);
    glVertex3f(-F,-0.05f,F);glVertex3f(F,-0.05f,F);glVertex3f(F,-0.05f,500);glVertex3f(-F,-0.05f,500);
    glEnd();
    /* Green grass inside */
    if(groundTex){ glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,groundTex); }
    else glColor3f(0.30f,0.62f,0.22f);
    glColor3f(1,1,1);
    float ts=4.0f, rep=0.10f;
    for(float x=-F;x<F;x+=ts) for(float z2=-F;z2<F;z2+=ts){
        float tx=(x+F)*rep, tz=(z2+F)*rep;
        glBegin(GL_QUADS);
        glNormal3f(0,1,0);
        glTexCoord2f(tx,tz);              glVertex3f(x,    -0.05f,z2    );
        glTexCoord2f(tx+ts*rep,tz);       glVertex3f(x+ts, -0.05f,z2    );
        glTexCoord2f(tx+ts*rep,tz+ts*rep);glVertex3f(x+ts, -0.05f,z2+ts);
        glTexCoord2f(tx,tz+ts*rep);       glVertex3f(x,    -0.05f,z2+ts );
        glEnd();
    }
    glDisable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
 

    /* Zone colored floor highlights */
    // drawZoneFloor( 20,-62, 65,-10, 0.18f,0.48f,0.92f, 0.13f); /* Ride zone - blue */
    // drawZoneFloor(-65,-62,-20,-10, 0.92f,0.58f,0.18f, 0.13f); /* Food court - orange */
    // drawZoneFloor(-65, 10,-20, 62, 0.18f,0.72f,0.32f, 0.10f); /* Game stalls - green */
    // drawZoneFloor(-20,-62, 20,-25, 0.58f,0.18f,0.88f, 0.10f); /* Stage area - purple */
    // drawZoneFloor(-20,-20, 20, 20, 0.92f,0.88f,0.72f, 0.18f); /* Central plaza - warm */
}

/* ─────────────────────────────
   BOUNDARY WALL (replaces plain fence - solid decorative wall)
───────────────────────────── */
void drawBoundaryWall(){
    const float F=65.0f;
    const float WH=2.5f;   /* wall height */
    const float WT=0.9f;   /* wall thickness */
    const float GW=8.0f;   /* gate width half */

    glEnable(GL_LIGHTING);

    /* Wall color: cream stone */
    glColor3f(0.88f,0.84f,0.74f);

    /* ── SOUTH WALL (with gate gap) ── */
    /* left segment */
    glPushMatrix();
    glTranslatef((-F+(-GW))*0.5f, WH*0.5f, -F);
    glScalef(F-GW, WH, WT);
    glutSolidCube(1);
    glPopMatrix();
    /* right segment */
    glPushMatrix();
    glTranslatef((GW+F)*0.5f, WH*0.5f, -F);
    glScalef(F-GW, WH, WT);
    glutSolidCube(1);
    glPopMatrix();

    /* ── NORTH WALL ── */
    glPushMatrix();
    glTranslatef(0, WH*0.5f, F);
    glScalef(F*2, WH, WT);
    glutSolidCube(1);
    glPopMatrix();

    /* ── EAST WALL ── */
    glPushMatrix();
    glTranslatef(F, WH*0.5f, 0);
    glScalef(WT, WH, F*2);
    glutSolidCube(1);
    glPopMatrix();

    /* ── WEST WALL ── */
    glPushMatrix();
    glTranslatef(-F, WH*0.5f, 0);
    glScalef(WT, WH, F*2);
    glutSolidCube(1);
    glPopMatrix();

    /* ── WALL TOP CAP (darker coping stone) ── */
    glColor3f(0.70f,0.60f,0.45f);
    float cH=0.28f, cExtra=0.15f;

    /* South left */
    glPushMatrix();
    glTranslatef((-F+(-GW))*0.5f, WH+cH*0.5f, -F);
    glScalef(F-GW+cExtra, cH, WT+cExtra);
    glutSolidCube(1);
    glPopMatrix();
    /* South right */
    glPushMatrix();
    glTranslatef((GW+F)*0.5f, WH+cH*0.5f, -F);
    glScalef(F-GW+cExtra, cH, WT+cExtra);
    glutSolidCube(1);
    glPopMatrix();
    /* North */
    glPushMatrix();
    glTranslatef(0, WH+cH*0.5f, F);
    glScalef(F*2+cExtra, cH, WT+cExtra);
    glutSolidCube(1);
    glPopMatrix();
    /* East */
    glPushMatrix();
    glTranslatef(F, WH+cH*0.5f, 0);
    glScalef(WT+cExtra, cH, F*2+cExtra);
    glutSolidCube(1);
    glPopMatrix();
    /* West */
    glPushMatrix();
    glTranslatef(-F, WH+cH*0.5f, 0);
    glScalef(WT+cExtra, cH, F*2+cExtra);
    glutSolidCube(1);
    glPopMatrix();

    /* ── DECORATIVE PILLAR POSTS every ~12 units ── */
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.82f,0.72f,0.52f);
    /* South wall pillars */
    for(float px=-F+6;px<-GW-2;px+=12){
        glPushMatrix(); glTranslatef(px, 0, -F);
        glRotatef(-90,1,0,0); gluCylinder(q,0.55f,0.48f,WH+0.6f,10,1);
        glPopMatrix();
        glPushMatrix(); glTranslatef(px, WH+0.6f, -F);
        glutSolidSphere(0.62f,10,10); glPopMatrix();
    }
    for(float px=GW+2;px<F-4;px+=12){
        glPushMatrix(); glTranslatef(px, 0, -F);
        glRotatef(-90,1,0,0); gluCylinder(q,0.55f,0.48f,WH+0.6f,10,1);
        glPopMatrix();
        glPushMatrix(); glTranslatef(px, WH+0.6f, -F);
        glutSolidSphere(0.62f,10,10); glPopMatrix();
    }
    /* North wall pillars */
    for(float px=-F+6;px<F-4;px+=12){
        glPushMatrix(); glTranslatef(px, 0, F);
        glRotatef(-90,1,0,0); gluCylinder(q,0.55f,0.48f,WH+0.6f,10,1);
        glPopMatrix();
        glPushMatrix(); glTranslatef(px, WH+0.6f, F);
        glutSolidSphere(0.62f,10,10); glPopMatrix();
    }
    /* East/West pillars */
    for(float pz=-F+6;pz<F-4;pz+=12){
        glPushMatrix(); glTranslatef(F, 0, pz);
        glRotatef(-90,1,0,0); gluCylinder(q,0.55f,0.48f,WH+0.6f,10,1);
        glPopMatrix();
        glPushMatrix(); glTranslatef(F, WH+0.6f, pz);
        glutSolidSphere(0.62f,10,10); glPopMatrix();

        glPushMatrix(); glTranslatef(-F, 0, pz);
        glRotatef(-90,1,0,0); gluCylinder(q,0.55f,0.48f,WH+0.6f,10,1);
        glPopMatrix();
        glPushMatrix(); glTranslatef(-F, WH+0.6f, pz);
        glutSolidSphere(0.62f,10,10); glPopMatrix();
    }
    /* Corner towers */
    float corners[][2]={{-F,-F},{F,-F},{F,F},{-F,F}};
    for(int c=0;c<4;c++){
        glColor3f(0.80f,0.68f,0.48f);
        glPushMatrix(); glTranslatef(corners[c][0],0,corners[c][1]);
        glRotatef(-90,1,0,0);
        gluCylinder(q,1.2f,1.0f,WH+1.2f,14,1);
        glPopMatrix();
        glColor3f(0.92f,0.28f,0.18f);
        glPushMatrix(); glTranslatef(corners[c][0],WH+1.3f,corners[c][1]);
        glutSolidSphere(1.3f,12,12); glPopMatrix();
    }
    gluDeleteQuadric(q);
}

/* ─────────────────────────────
   ROAD SYSTEM — clean, non-overlapping
   All roads are flat quads with sidewalk border and dashes
───────────────────────────── */

/* Draw a road segment between two points with proper width */
void drawRoadSegment(float x1,float z1,float x2,float z2,float roadW){
    glDisable(GL_LIGHTING);

    float dx=x2-x1, dz=z2-z1;
    float len=sqrtf(dx*dx+dz*dz);
    if(len<0.01f){glEnable(GL_LIGHTING);return;}

    float nx=-dz/len, nz=dx/len;
    float rh=roadW*0.5f;
    float sh=rh+1.0f;

    /* sidewalk */
   
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glColor3f(0.76f,0.72f,0.66f);
    glBegin(GL_QUADS);

    glVertex3f(x1+nx*sh, 0.06f, z1+nz*sh);   // 🔥 was 0.01
    glVertex3f(x2+nx*sh, 0.06f, z2+nz*sh);
    glVertex3f(x2-nx*sh, 0.06f, z2-nz*sh);
    glVertex3f(x1-nx*sh, 0.06f, z1-nz*sh);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnd();

    /* asphalt */
    glEnable(GL_POLYGON_OFFSET_FILL);
glPolygonOffset(-2.0f, -2.0f); // slightly stronger
    glColor3f(0.22f,0.22f,0.24f);
    glBegin(GL_QUADS);
    glVertex3f(x1+nx*rh, 0.06f, z1+nz*rh);   // 🔥 was 0.03
    glVertex3f(x2+nx*rh, 0.06f, z2+nz*rh);
    glVertex3f(x2-nx*rh, 0.06f, z2-nz*rh);
    glVertex3f(x1-nx*rh, 0.06f, z1-nz*rh);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnd();

    /* center dashes */
    glColor3f(0.96f,0.88f,0.0f);
    float dashLen=3.5f, gapLen=2.5f, total=dashLen+gapLen;
    int numDash=(int)(len/total);

    for(int d=0;d<numDash;d++){
        float t0=(d*total)/len, t1=((d*total)+dashLen)/len;
        if(t1>1)t1=1;

        float cx0=x1+dx*t0, cz0=z1+dz*t0;
        float cx1=x1+dx*t1, cz1=z1+dz*t1;

        glBegin(GL_QUADS);
        glVertex3f(cx0+nx*0.12f,0.07f,cz0+nz*0.12f); // 🔥 was 0.05
        glVertex3f(cx1+nx*0.12f,0.07f,cz1+nz*0.12f);
        glVertex3f(cx1-nx*0.12f,0.07f,cz1-nz*0.12f);
        glVertex3f(cx0-nx*0.12f,0.07f,cz0-nz*0.12f);
        glEnd();
    }

    glEnable(GL_LIGHTING);
}

/* Circular roundabout road around fountain (radius ~14) */
void drawRoundabout(float cx, float cz, float innerR, float outerR){
    glDisable(GL_LIGHTING);
    const int SEG=64;

    /* sidewalk ring */
    glColor3f(0.76f,0.72f,0.66f);
    float sOuter=outerR+1.0f, sInner=innerR-1.0f;
    if(sInner<0)sInner=0;
    glBegin(GL_TRIANGLE_STRIP);
    for(int i=0;i<=SEG;i++){
        float a=i*2*(float)M_PI/SEG;
        glVertex3f(cx+cosf(a)*sOuter, 0.01f, cz+sinf(a)*sOuter);
        glVertex3f(cx+cosf(a)*sInner, 0.01f, cz+sinf(a)*sInner);
    }
    glEnd();

    /* asphalt ring */
    glColor3f(0.22f,0.22f,0.24f);
    glBegin(GL_TRIANGLE_STRIP);
    for(int i=0;i<=SEG;i++){
        float a=i*2*(float)M_PI/SEG;
        glVertex3f(cx+cosf(a)*outerR, 0.03f, cz+sinf(a)*outerR);
        glVertex3f(cx+cosf(a)*innerR, 0.03f, cz+sinf(a)*innerR);
    }
    glEnd();

    /* dashed center line of roundabout */
    float midR=(innerR+outerR)*0.5f;
    glColor3f(0.96f,0.88f,0.0f);
    for(int i=0;i<SEG;i+=2){
        float a0=i*2*(float)M_PI/SEG;
        float a1=(i+1)*2*(float)M_PI/SEG;
        glBegin(GL_QUADS);
        glVertex3f(cx+cosf(a0)*(midR-0.12f),0.05f,cz+sinf(a0)*(midR-0.12f));
        glVertex3f(cx+cosf(a1)*(midR-0.12f),0.05f,cz+sinf(a1)*(midR-0.12f));
        glVertex3f(cx+cosf(a1)*(midR+0.12f),0.05f,cz+sinf(a1)*(midR+0.12f));
        glVertex3f(cx+cosf(a0)*(midR+0.12f),0.05f,cz+sinf(a0)*(midR+0.12f));
        glEnd();
    }

    /* Inner island paving (decorative cobblestone look) */
    glColor3f(0.68f,0.62f,0.52f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(cx,0.02f,cz);
    for(int i=0;i<=SEG;i++){
        float a=i*2*(float)M_PI/SEG;
        glVertex3f(cx+cosf(a)*sInner,0.02f,cz+sinf(a)*sInner);
    }
    glEnd();

    /* cobblestone rings on island */
    for(int ring=1;ring<=3;ring++){
        glColor3f(ring%2==0?0.60f:0.72f, ring%2==0?0.54f:0.66f, ring%2==0?0.44f:0.54f);
        float r=sInner*ring/3.5f;
        glBegin(GL_LINE_LOOP);
        for(int i=0;i<SEG;i++){
            float a=i*2*(float)M_PI/SEG;
            glVertex3f(cx+cosf(a)*r,0.025f,cz+sinf(a)*r);
        }
        glEnd();
    }

    glEnable(GL_LIGHTING);
}

/* Pathway (decorative paving stone walkway) */
void drawPathway(float x1,float z1,float x2,float z2,float width){
    glDisable(GL_LIGHTING);
    float dx=x2-x1, dz=z2-z1;
    float len=sqrtf(dx*dx+dz*dz);
    if(len<0.01f){glEnable(GL_LIGHTING);return;}
    float nx=-dz/len, nz=dx/len;
    float hw=width*0.5f;

    glColor3f(0.84f,0.78f,0.68f);
    glBegin(GL_QUADS);
    glVertex3f(x1+nx*hw,0.02f,z1+nz*hw);
    glVertex3f(x2+nx*hw,0.02f,z2+nz*hw);
    glVertex3f(x2-nx*hw,0.02f,z2-nz*hw);
    glVertex3f(x1-nx*hw,0.02f,z1-nz*hw);
    glEnd();
    /* paving lines */
    glColor3f(0.70f,0.64f,0.52f);
    float step=2.5f;
    int ns=(int)(len/step);
    for(int i=0;i<ns;i++){
        float t=(float)i/ns;
        float px=x1+dx*t, pz=z1+dz*t;
        glBegin(GL_LINES);
        glVertex3f(px+nx*hw,0.025f,pz+nz*hw);
        glVertex3f(px-nx*hw,0.025f,pz-nz*hw);
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

/* ─────────────────────────────
   TREE
───────────────────────────── */
void drawTree(float x,float z,float sc=1.0f){
    glPushMatrix(); glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.38f,0.23f,0.10f);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.22*sc,.15*sc,3.5*sc,10,1);glPopMatrix();
    float gc[][3]={{0.10f,0.48f,0.14f},{0.08f,0.42f,0.12f},{0.12f,0.52f,0.16f}};
    for(int t=0;t<3;t++){
        glColor3f(gc[t][0],gc[t][1],gc[t][2]);
        glPushMatrix();glTranslatef(0,(2.8f+t*1.8f)*sc,0);glRotatef(-90,1,0,0);
        gluCylinder(q,(1.6f-t*.38f)*sc,0,2.2f*sc,10,1);glPopMatrix();
    }
    gluDeleteQuadric(q); glPopMatrix();
}


/* ─────────────────────────────
   LAMP POST
───────────────────────────── */
void drawLampPost(float x,float z){
    glPushMatrix();glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.22f,0.22f,0.30f);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.12,.08,7.5,10,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,7.5f,0);glRotatef(60,0,0,1);
    gluCylinder(q,.07,.07,1.5,8,1);glPopMatrix();
    glPushMatrix();glTranslatef(-1.3f,7.5f,0);
    glColor3f(0.22f,0.22f,0.28f);glutSolidSphere(.3,8,8);
    glColor3f(1.0f,0.96f,0.72f);glTranslatef(0,0,.22f);glutSolidSphere(.15,6,6);glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   BENCH
───────────────────────────── */
void drawBench(float x,float z,float ry){
    glPushMatrix();glTranslatef(x,0,z);glRotatef(ry,0,1,0);
    glColor3f(0.32f,0.20f,0.08f);
    float lx[]={-.85f,.85f,-.85f,.85f},lzz[]={-.32f,-.32f,.32f,.32f};
    for(int i=0;i<4;i++){glPushMatrix();glTranslatef(lx[i],.55f,lzz[i]);glScalef(.12f,1,.12f);glutSolidCube(1);glPopMatrix();}
    glColor3f(0.58f,0.38f,0.14f);
    glPushMatrix();glTranslatef(0,1.1f,0);glScalef(1.8f,.1f,.68f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0,1.72f,-.3f);glScalef(1.8f,.55f,.1f);glutSolidCube(1);glPopMatrix();
    glPopMatrix();
}

/* ─────────────────────────────
   TRASH BIN
───────────────────────────── */
void drawTrashBin(float x,float z,float r,float g,float b){
    glPushMatrix();glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();
    glColor3f(r,g,b);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.28,.32,1.1f,12,1);glPopMatrix();
    glColor3f(r*.7f,g*.7f,b*.7f);
    glPushMatrix();glTranslatef(0,1.12f,0);glRotatef(-90,1,0,0);gluDisk(q,0,.32f,12,1);glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   BALLOON
───────────────────────────── */
void drawBalloon(float x,float y,float z,float r,float g,float b,float sway){
    glPushMatrix();glTranslatef(x+sway,y,z);
    glDisable(GL_LIGHTING);
    glColor3f(.75f,.75f,.75f);
    glBegin(GL_LINES);glVertex3f(0,0,0);glVertex3f(0,-2.5f,0);glEnd();
    glEnable(GL_LIGHTING);
    glColor3f(r*.7f,g*.7f,b*.7f);glutSolidSphere(.09,6,6);
    glColor3f(r,g,b);
    glPushMatrix();glScalef(1.0f,1.25f,1.0f);glutSolidSphere(.58,14,14);glPopMatrix();
    glColor3f(std::min(r+.45f,1.f),std::min(g+.45f,1.f),std::min(b+.45f,1.f));
    glPushMatrix();glTranslatef(-.14f,.28f,.38f);glutSolidSphere(.11,6,6);glPopMatrix();
    glPopMatrix();
}

/* ─────────────────────────────
   BALLOON STALL
───────────────────────────── */
void drawBalloonStall(float cx,float cz,float ang){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(ang,0,1,0);
    glColor3f(.52f,.30f,.10f);
    glPushMatrix();glScalef(4.5f,1.2f,2.3f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(.80f,.70f,.52f);
    glPushMatrix();glTranslatef(0,1.26f,0);glScalef(4.7f,.14f,2.5f);glutSolidCube(1);glPopMatrix();
    float px[]={-2.1f,2.1f,-2.1f,2.1f},pzz[]={-1.1f,-1.1f,1.1f,1.1f};
    GLUquadric*q=gluNewQuadric();
    for(int i=0;i<4;i++){glColor3f(.80f,.12f,.12f);
        glPushMatrix();glTranslatef(px[i],1.26f,pzz[i]);glRotatef(-90,1,0,0);
        gluCylinder(q,.12,.12,4.2,8,1);glPopMatrix();}
    int S=10; float cw=4.4f/S,ry=5.5f;
    for(int s=0;s<S;s++){
        glColor3f(s%2==0?.92f:.98f,s%2==0?.10f:.98f,s%2==0?.10f:.98f);
        float xs=-2.2f+s*cw;
        glBegin(GL_QUADS);
        glVertex3f(xs,ry,-1.2f);glVertex3f(xs+cw,ry,-1.2f);glVertex3f(xs+cw,ry-.6f,-1.8f);glVertex3f(xs,ry-.6f,-1.8f);glEnd();
        glBegin(GL_QUADS);
        glVertex3f(xs,ry,1.2f);glVertex3f(xs+cw,ry,1.2f);glVertex3f(xs+cw,ry-.6f,1.8f);glVertex3f(xs,ry-.6f,1.8f);glEnd();
        glBegin(GL_QUADS);
        glVertex3f(xs,ry,-1.2f);glVertex3f(xs+cw,ry,-1.2f);glVertex3f(xs+cw,ry,1.2f);glVertex3f(xs,ry,1.2f);glEnd();
    }
    float bc[][3]={{1,.18f,.18f},{.18f,.5f,1},{1,.88f,.08f},{.18f,.82f,.28f},{1,.42f,.05f},{.72f,.18f,.88f}};
    for(int b=0;b<6;b++){float bx=-1.8f+(b%3)*.65f,bz2=-.4f+(b/3)*.8f,by=1.26f+2.0f+(b%2)*.5f;
        float sw=sinf(windTime*1.2f+b)*.14f;
        drawBalloon(bx,by,bz2,bc[b][0],bc[b][1],bc[b][2],sw);}
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   FUNLAND ENTRANCE GATE  (big, decorated, with name)
───────────────────────────── */
void drawFunlandGate(float cx,float cz){
    glPushMatrix();glTranslatef(cx,0,cz);
    GLUquadric*q=gluNewQuadric();

    float gap=10.0f;  /* half gap for pedestrian path */
    float ph=13.0f;   /* pillar height */

    /* ── Main pillars (2 tall decorated columns) ── */
    for(int s=-1;s<=1;s+=2){
        /* Base plinth */
        glColor3f(0.90f,0.82f,0.60f);
        glPushMatrix();glTranslatef(s*gap,0,0);glScalef(2.8f,0.8f,2.8f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();

        /* Column shaft */
        glColor3f(0.95f,0.88f,0.68f);
        glPushMatrix();glTranslatef(s*gap,0.8f,0);glRotatef(-90,1,0,0);
        gluCylinder(q,1.3f,1.1f,ph,18,2);glPopMatrix();

        /* Column capital rings */
        glColor3f(0.88f,0.72f,0.40f);
        for(int r=0;r<3;r++){
            glPushMatrix();glTranslatef(s*gap,ph-r*0.55f,0);glRotatef(-90,1,0,0);
            gluCylinder(q,1.4f-r*0.05f,1.3f-r*0.05f,0.38f,16,1);glPopMatrix();
        }

        /* Glowing orb on top */
        glColor3f(1.0f,0.95f,0.50f);
        glPushMatrix();glTranslatef(s*gap,ph+0.8f+1.6f,0);glutSolidSphere(1.6f,16,16);glPopMatrix();
        /* Inner glow */
        glColor3f(1.0f,1.0f,0.8f);
        glPushMatrix();glTranslatef(s*gap,ph+0.8f+1.6f,0);glutSolidSphere(1.0f,12,12);glPopMatrix();

        /* Spiral stripe on column */
        glColor3f(0.92f,0.22f,0.22f);
        for(int st=0;st<20;st++){
            float t=(float)st/20*ph;
            float a=st*360.f/20*(float)M_PI/180;
            float r2=1.35f;
            glPushMatrix();glTranslatef(s*gap+cosf(a)*r2,0.8f+t,sinf(a)*r2);
            glScalef(0.18f,0.38f,0.18f);glutSolidSphere(1,5,5);glPopMatrix();
        }

        /* Waving flag */
        float fc[][3]={{1.0f,0.82f,0.08f},{0.92f,0.18f,0.18f},{0.18f,0.62f,0.92f}};
        for(int fl=0;fl<3;fl++){
            glColor3f(fc[fl][0],fc[fl][1],fc[fl][2]);
            float fy=ph+0.8f+3.4f+(fl%2)*0.5f;
            glPushMatrix();glTranslatef(s*gap,fy,0);
            glBegin(GL_TRIANGLES);
            glVertex3f(0,0,0);glVertex3f(s*3.8f,0.5f+fl*0.3f,0);glVertex3f(s*3.2f,-1.1f,0);
            glEnd();
            glPopMatrix();
        }
    }

    /* ── Arched beam connecting the two columns ── */
    glColor3f(0.90f,0.80f,0.55f);
    glPushMatrix();glTranslatef(0,ph+0.8f,0);glScalef(gap*2.0f,1.8f,2.0f);glutSolidCube(1);glPopMatrix();

    /* Arch over the beam — rainbow segments */
    float AR=gap+1.5f;
    float AT=0.65f;
    int AS=24;
    float ac[][3]={{1,0,0},{1,.5f,0},{1,1,0},{0,.8f,0},{0,.5f,1},{.3f,0,1},{.8f,0,.8f}};
    for(int s=0;s<AS;s++){
        float a0=s*(float)M_PI/AS, a1=(s+1)*(float)M_PI/AS;
        glColor3f(ac[s%7][0],ac[s%7][1],ac[s%7][2]);
        glBegin(GL_QUADS);
        glVertex3f((AR-AT)*cosf(a0),ph+0.8f+(AR-AT)*sinf(a0),-.4f);
        glVertex3f(AR*cosf(a0),ph+0.8f+AR*sinf(a0),-.4f);
        glVertex3f(AR*cosf(a1),ph+0.8f+AR*sinf(a1),-.4f);
        glVertex3f((AR-AT)*cosf(a1),ph+0.8f+(AR-AT)*sinf(a1),-.4f);
        glEnd();
        glBegin(GL_QUADS);
        glVertex3f((AR-AT)*cosf(a0),ph+0.8f+(AR-AT)*sinf(a0),.4f);
        glVertex3f(AR*cosf(a0),ph+0.8f+AR*sinf(a0),.4f);
        glVertex3f(AR*cosf(a1),ph+0.8f+AR*sinf(a1),.4f);
        glVertex3f((AR-AT)*cosf(a1),ph+0.8f+(AR-AT)*sinf(a1),.4f);
        glEnd();
    }

    /* ── "FUNLAND" sign board on beam ── */
    /* Background board */
    glColor3f(0.08f,0.06f,0.28f);
    glPushMatrix();glTranslatef(0,ph+0.8f+4.8f,0.1f);glScalef(14.5f,3.2f,0.4f);glutSolidCube(1);glPopMatrix();

    /* Colored inner panel */
    glColor3f(0.12f,0.08f,0.45f);
    glPushMatrix();glTranslatef(0,ph+0.8f+4.8f,0.35f);glScalef(13.8f,2.6f,0.15f);glutSolidCube(1);glPopMatrix();

    /* Gold border top/bottom */
    glColor3f(1.0f,0.85f,0.10f);
    glPushMatrix();glTranslatef(0,ph+0.8f+6.3f,0.35f);glScalef(14.2f,0.32f,0.18f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0,ph+0.8f+3.3f,0.35f);glScalef(14.2f,0.32f,0.18f);glutSolidCube(1);glPopMatrix();

    /* Letter F */
    glColor3f(1.0f,0.92f,0.12f);
    glPushMatrix();glTranslatef(-5.8f,ph+0.8f+4.8f,0.45f);
    glScalef(0.38f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-5.22f,ph+0.8f+5.7f,0.45f);
    glScalef(0.75f,0.32f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-5.32f,ph+0.8f+4.9f,0.45f);
    glScalef(0.55f,0.30f,0.12f);glutSolidCube(1);glPopMatrix();

    /* Letter U */
    glColor3f(1.0f,0.40f,0.10f);
    glPushMatrix();glTranslatef(-4.0f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-3.2f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-3.6f,ph+0.8f+3.85f,0.45f);
    glScalef(1.1f,0.30f,0.12f);glutSolidCube(1);glPopMatrix();

    /* Letter N */
    glColor3f(0.18f,0.88f,0.28f);
    glPushMatrix();glTranslatef(-2.15f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-1.35f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glBegin(GL_TRIANGLES);
    glVertex3f(-2.15f,ph+0.8f+5.82f,0.47f);
    glVertex3f(-1.35f,ph+0.8f+5.82f,0.47f);
    glVertex3f(-2.15f,ph+0.8f+3.82f,0.47f);
    glEnd();

    /* Letter L */
    glColor3f(0.18f,0.62f,1.0f);
    glPushMatrix();glTranslatef(-0.35f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0.15f,ph+0.8f+3.85f,0.45f);
    glScalef(1.32f,0.30f,0.12f);glutSolidCube(1);glPopMatrix();

    /* Letter A */
    glColor3f(0.82f,0.18f,0.88f);
    glPushMatrix();glTranslatef(1.05f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(1.85f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    /* A crossbar */
    glPushMatrix();glTranslatef(1.45f,ph+0.8f+4.9f,0.45f);
    glScalef(1.1f,0.28f,0.12f);glutSolidCube(1);glPopMatrix();
    /* A top */
    glBegin(GL_TRIANGLES);
    glVertex3f(1.05f,ph+0.8f+5.82f,0.47f);
    glVertex3f(1.85f,ph+0.8f+5.82f,0.47f);
    glVertex3f(1.45f,ph+0.8f+6.6f,0.47f);
    glEnd();

    /* Letter N (second) */
    glColor3f(1.0f,0.62f,0.08f);
    glPushMatrix();glTranslatef(2.85f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(3.65f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glBegin(GL_TRIANGLES);
    glVertex3f(2.85f,ph+0.8f+5.82f,0.47f);
    glVertex3f(3.65f,ph+0.8f+5.82f,0.47f);
    glVertex3f(2.85f,ph+0.8f+3.82f,0.47f);
    glEnd();

    /* Letter D */
    glColor3f(0.92f,0.10f,0.28f);
    glPushMatrix();glTranslatef(4.65f,ph+0.8f+4.8f,0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    /* D arc */
    for(int i=0;i<8;i++){
        float a0=i*180.f/8*(float)M_PI/180 - (float)M_PI/2;
        float a1=(i+1)*180.f/8*(float)M_PI/180 - (float)M_PI/2;
        glBegin(GL_QUADS);
        glVertex3f(4.65f+cosf(a0)*0.85f,ph+0.8f+4.8f+sinf(a0)*1.05f,0.47f);
        glVertex3f(4.65f+cosf(a0)*0.55f,ph+0.8f+4.8f+sinf(a0)*0.75f,0.47f);
        glVertex3f(4.65f+cosf(a1)*0.55f,ph+0.8f+4.8f+sinf(a1)*0.75f,0.47f);
        glVertex3f(4.65f+cosf(a1)*0.85f,ph+0.8f+4.8f+sinf(a1)*1.05f,0.47f);
        glEnd();
    }

    /* Star decorations around sign */
    float starC[][3]={{1,1,0},{1,0.5f,0},{0.5f,1,0},{0,0.8f,1},{1,0.2f,0.8f}};
    for(int st=0;st<8;st++){
        glColor3f(starC[st%5][0],starC[st%5][1],starC[st%5][2]);
        float sx=-6.2f+st*1.8f;
        float sy=ph+0.8f+6.8f+sinf(windTime*2+st)*0.25f;
        glPushMatrix();glTranslatef(sx,sy,0.48f);glutSolidSphere(0.22f,8,8);glPopMatrix();
    }

    /* Bulb lights across beam bottom */
    for(int b=0;b<16;b++){
        float bx=-7.2f+b*0.96f;
        glColor3f(1,1,0.6f);
        glPushMatrix();glTranslatef(bx,ph+0.8f-0.12f,0.6f);glutSolidSphere(0.18f,6,6);glPopMatrix();
        /* wire */
        glDisable(GL_LIGHTING);
        glColor3f(0.3f,0.3f,0.3f);
        glBegin(GL_LINES);
        glVertex3f(bx,ph+0.8f,0.6f);
        glVertex3f(bx,ph+0.8f-0.12f,0.6f);
        glEnd();
        glEnable(GL_LIGHTING);
    }

    /* Welcome mat / paving at gate entrance */
    glDisable(GL_LIGHTING);
    glColor3f(0.78f,0.62f,0.42f);
    glBegin(GL_QUADS);
    glVertex3f(-gap+0.5f,0.05f,-2.5f);glVertex3f(gap-0.5f,0.05f,-2.5f);
    glVertex3f(gap-0.5f,0.05f,2.5f);  glVertex3f(-gap+0.5f,0.05f,2.5f);
    glEnd();
    /* checkerboard on mat */
    for(int i=0;i<6;i++) for(int j=0;j<2;j++){
        float mx=-gap+1.2f+i*3.0f, mz=-2.2f+j*2.2f;
        glColor3f((i+j)%2==0?0.88f:0.62f, (i+j)%2==0?0.72f:0.48f, (i+j)%2==0?0.52f:0.32f);
        glBegin(GL_QUADS);
        glVertex3f(mx,0.06f,mz);glVertex3f(mx+2.8f,0.06f,mz);
        glVertex3f(mx+2.8f,0.06f,mz+2.0f);glVertex3f(mx,0.06f,mz+2.0f);
        glEnd();
    }
    glEnable(GL_LIGHTING);

    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   TICKET BOOTH
───────────────────────────── */
void drawTicketBooth(float cx,float cz,float ry){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(ry,0,1,0);
    GLUquadric*q=gluNewQuadric();

    /* main body (slightly modern color) */
    glColor3f(0.12f,0.28f,0.65f);
    glPushMatrix();glScalef(3.5f,4.5f,3.0f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();

    /* roof (cleaner pyramid) */
    glColor3f(0.85f,0.1f,0.1f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-2.0f,4.8f,1.5f);glVertex3f(2.0f,4.8f,1.5f);glVertex3f(0,6.5f,0);
    glVertex3f(-2.0f,4.8f,-1.5f);glVertex3f(2.0f,4.8f,-1.5f);glVertex3f(0,6.5f,0);
    glVertex3f(-2.0f,4.8f,-1.5f);glVertex3f(-2.0f,4.8f,1.5f);glVertex3f(0,6.5f,0);
    glVertex3f(2.0f,4.8f,-1.5f);glVertex3f(2.0f,4.8f,1.5f);glVertex3f(0,6.5f,0);
    glEnd();

    /* glass window (modern look) */
    glColor4f(0.6f,0.85f,1.0f,0.6f);
    glPushMatrix();glTranslatef(0,2.2f,1.55f);glScalef(1.8f,1.0f,0.08f);glutSolidCube(1);glPopMatrix();

    /* counter shelf */
    glColor3f(0.45f,0.25f,0.1f);
    glPushMatrix();glTranslatef(0,1.5f,1.7f);glScalef(2.2f,0.15f,0.6f);glutSolidCube(1);glPopMatrix();

    /* sign board (clean modern style) */
    glColor3f(0.95f,0.85f,0.1f);
    glPushMatrix();glTranslatef(0,3.8f,1.55f);glScalef(2.6f,0.6f,0.08f);glutSolidCube(1);glPopMatrix();

    glColor3f(0.05f,0.05f,0.2f);
    glPushMatrix();glTranslatef(0,3.8f,1.62f);glScalef(2.2f,0.4f,0.04f);glutSolidCube(1);glPopMatrix();

    /* simple letters (clean blocks) */
    for(int i=0;i<7;i++){
        glColor3f(1,0.6f,0.1f);
        glPushMatrix();
        glTranslatef(-1.0f+i*0.33f,3.8f,1.64f);
        glScalef(0.2f,0.3f,0.04f);
        glutSolidCube(1);
        glPopMatrix();
    }

    /* 🎟️ TICKET PERSON (simple human) */
    // body
    glColor3f(0.1f,0.6f,0.9f);
    glPushMatrix();glTranslatef(0,2.0f,1.3f);glScalef(0.4f,0.8f,0.3f);glutSolidCube(1);glPopMatrix();

    // head
    glColor3f(1.0f,0.8f,0.6f);
    glPushMatrix();glTranslatef(0,2.7f,1.3f);glutSolidSphere(0.18f,10,10);glPopMatrix();

    // ticket (small cube in hand)
    glColor3f(1,1,0);
    glPushMatrix();glTranslatef(0.3f,2.1f,1.45f);glScalef(0.15f,0.08f,0.02f);glutSolidCube(1);glPopMatrix();

    /* balloons (keep same but smaller) */
    float bc[][3]={{1,.18f,.18f},{.18f,.5f,1},{1,.85f,.05f},{.18f,.82f,.28f}};
    for(int b=0;b<4;b++){
        float sw=sinf(ledTime*1.5f+b)*.12f;
        drawBalloon(-0.7f+b*0.45f,7.0f,0,bc[b][0],bc[b][1],bc[b][2],sw);
    }

    /* side lights (modern minimal) */
    for(int s=-1;s<=1;s+=2){
        glColor3f(0.3f,0.3f,0.35f);
        glPushMatrix();glTranslatef(s*2.2f,0,0);glRotatef(-90,1,0,0);
        gluCylinder(q,0.08f,0.08f,2.0f,8,1);glPopMatrix();

        glColor3f(1,1,0.6f);
        glPushMatrix();glTranslatef(s*2.2f,2.2f,0);glutSolidSphere(0.25f,10,10);glPopMatrix();
    }

    gluDeleteQuadric(q);glPopMatrix();
}




   /* FOOD COURT  — big attractive stalls + vendor + big tables
   All three stalls clustered together SW quadrant
═══════════════════════════════════════════════════════ */

/* Shared awning helper */
void drawStallAwning(float hw,float dep,float ht,int stripes,float r1,float g1,float b1){
    float sw2=hw*2/stripes;
    for(int s=0;s<stripes;s++){
        glColor3f(s%2==0?r1:1.0f, s%2==0?g1:1.0f, s%2==0?b1:1.0f);
        float ax=-hw+s*sw2;
        glBegin(GL_QUADS);
        glVertex3f(ax,ht,-dep*.5f); glVertex3f(ax+sw2,ht,-dep*.5f);
        glVertex3f(ax+sw2,ht-0.65f,-dep*.9f); glVertex3f(ax,ht-0.65f,-dep*.9f); glEnd();
        glBegin(GL_QUADS);
        glVertex3f(ax,ht, dep*.5f); glVertex3f(ax+sw2,ht, dep*.5f);
        glVertex3f(ax+sw2,ht-0.65f, dep*.9f); glVertex3f(ax,ht-0.65f, dep*.9f); glEnd();
        glBegin(GL_QUADS);
        glVertex3f(ax,ht,-dep*.5f); glVertex3f(ax+sw2,ht,-dep*.5f);
        glVertex3f(ax+sw2,ht, dep*.5f); glVertex3f(ax,ht, dep*.5f); glEnd();
    }
    /* hanging fringe */
    glColor3f(1.0f,0.88f,0.0f);
    for(int t=0;t<stripes*2+1;t++){
        float tx=-hw+t*(hw*2/(stripes*2));
        glBegin(GL_TRIANGLES);
        glVertex3f(tx,ht-0.65f,-dep*.9f);
        glVertex3f(tx+(hw*2/(stripes*2))*.5f,ht-1.1f,-dep*0.95f);
        glVertex3f(tx+(hw*2/(stripes*2)),ht-0.65f,-dep*.9f); glEnd();
    }
}

void drawPizzaStall(float cx,float cz,float ang){
    glPushMatrix();
    glTranslatef(cx,0,cz);
    glRotatef(ang,0,1,0);

    GLUquadric* q = gluNewQuadric();

    /* ===== BIGGER COUNTER ===== */
    glColor3f(0.55f,0.30f,0.12f);
    glPushMatrix();
    glScalef(9.0f,2.2f,4.5f);
    glTranslatef(0,0.5f,0);
    glutSolidCube(1);
    glPopMatrix();

    /* ===== TOP SLAB ===== */
    glColor3f(0.82f,0.78f,0.65f);
    glPushMatrix();
    glTranslatef(0,2.2f,0);
    glScalef(9.2f,0.25f,4.7f);
    glutSolidCube(1);
    glPopMatrix();

    /* ===== POSTS ===== */
    float px[]={-4.2f,4.2f,-4.2f,4.2f};
    float pz[]={-2.2f,-2.2f,2.2f,2.2f};

    for(int i=0;i<4;i++){
        for(int st=0;st<5;st++){
            glColor3f(st%2?1.0f:0.9f, st%2?1.0f:0.15f, st%2?1.0f:0.15f);
            glPushMatrix();
            glTranslatef(px[i],2.2f+st*1.0f,pz[i]);
            glRotatef(-90,1,0,0);
            gluCylinder(q,0.18f,0.18f,1.0f,8,1);
            glPopMatrix();
        }
    }

    /* ===== BIG AWNING ===== */
    drawStallAwning(4.5f,4.5f,8.5f,14, 0.92f,0.18f,0.18f);

    /* ===== BIG FRONT LOGO (NO MIRROR ISSUE) ===== */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pizzaTex);
    glColor3f(1,1,1);

    glPushMatrix();
    glTranslatef(0,9.5f,-3.0f);   // position
    glRotatef(-12,1,0,0);         // tilt like real shop

    glBegin(GL_QUADS);
      glTexCoord2f(0,1); glVertex3f(-4.5f,0,0);
        glTexCoord2f(1,1); glVertex3f( 4.5f,0,0);
        glTexCoord2f(1,0); glVertex3f( 4.5f,2.5f,0);
        glTexCoord2f(0,0); glVertex3f(-4.5f,2.5f,0);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    /* ===== PIZZA DISPLAY ON COUNTER ===== */
    for(int i=-2;i<=2;i++){
        float x = i*1.5f;

        glColor3f(0.95f,0.75f,0.25f);
        glPushMatrix();
        glTranslatef(x,2.4f,0.8f);
        glRotatef(-90,1,0,0);
        gluDisk(q,0,0.8f,16,1);
        glPopMatrix();

        glColor3f(1.0f,0.9f,0.3f);
        glPushMatrix();
        glTranslatef(x,2.42f,0.8f);
        glRotatef(-90,1,0,0);
        gluDisk(q,0,0.7f,16,1);
        glPopMatrix();

        glColor3f(0.8f,0.1f,0.1f);
        glPushMatrix();
        glTranslatef(x+0.2f,2.5f,0.8f);
        glutSolidSphere(0.1,8,8);
        glPopMatrix();
    }

    gluDeleteQuadric(q);
    glPopMatrix();

    
}

void drawBurgerStall(float cx,float cz,float ang){
    glPushMatrix();
    glTranslatef(cx,0,cz);
    glRotatef(ang,0,1,0);

    GLUquadric*q=gluNewQuadric();

    /* ===== BIGGER COUNTER ===== */
    glColor3f(0.44f,0.24f,0.08f);
    glPushMatrix();
    glScalef(8.5f,2.0f,4.2f);
    glTranslatef(0,0.5f,0);
    glutSolidCube(1);
    glPopMatrix();

    /* top slab */
    glColor3f(0.82f,0.72f,0.45f);
    glPushMatrix();
    glTranslatef(0,2.0f,0);
    glScalef(8.7f,0.25f,4.4f);
    glutSolidCube(1);
    glPopMatrix();

    /* ===== POSTS ===== */
    float px[]={-4.0f,4.0f,-4.0f,4.0f};
    float pz[]={-2.0f,-2.0f,2.0f,2.0f};

    for(int i=0;i<4;i++){
        for(int st=0;st<5;st++){
            glColor3f(st%2?1.0f:0.95f, st%2?0.8f:0.2f, 0.0f);

            glPushMatrix();
            glTranslatef(px[i],2.0f+st*0.9f,pz[i]);
            glRotatef(-90,1,0,0);
            gluCylinder(q,0.18f,0.18f,0.9f,8,1);
            glPopMatrix();
        }
    }

    /* ===== AWNING ===== */
    drawStallAwning(4.2f,4.2f,7.8f,14, 1.0f,0.6f,0.0f);

    /* ===== BIG FRONT LOGO (LIKE PIZZA) ===== */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, burgerTex);
    glColor3f(1,1,1);

    glPushMatrix();
    glTranslatef(0,9.5f,-3.2f);
    glRotatef(-12,1,0,0);

    glBegin(GL_QUADS);
        glTexCoord2f(0,1); glVertex3f(-4.5f,0,0);
        glTexCoord2f(1,1); glVertex3f( 4.5f,0,0);
        glTexCoord2f(1,0); glVertex3f( 4.5f,2.5f,0);
        glTexCoord2f(0,0); glVertex3f(-4.5f,2.5f,0);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    /* ===== BURGER DISPLAY ===== */
    for(int i=-2;i<=2;i++){
        float x=i*1.5f;

        /* top bun */
        glColor3f(0.9f,0.7f,0.3f);
        glPushMatrix();
        glTranslatef(x,2.2f,0.8f);
        glScalef(1.0f,0.5f,1.0f);
        glutSolidSphere(0.6,12,8);
        glPopMatrix();

        /* patty */
        glColor3f(0.5f,0.2f,0.1f);
        glPushMatrix();
        glTranslatef(x,2.0f,0.8f);
        glScalef(0.9f,0.2f,0.9f);
        glutSolidSphere(0.6,10,6);
        glPopMatrix();

        /* bottom bun */
        glColor3f(0.85f,0.6f,0.25f);
        glPushMatrix();
        glTranslatef(x,1.85f,0.8f);
        glScalef(1.0f,0.3f,1.0f);
        glutSolidSphere(0.6,10,6);
        glPopMatrix();
    }



    gluDeleteQuadric(q);
    glPopMatrix();
}

void drawIceCreamStall(float cx,float cz,float ang){
    glPushMatrix();
    glTranslatef(cx,0,cz);
    glRotatef(ang,0,1,0);

    GLUquadric*q=gluNewQuadric();

    /* ===== BIGGER COUNTER ===== */
    glColor3f(0.85f,0.55f,0.80f);
    glPushMatrix();
    glScalef(8.5f,2.0f,4.2f);
    glTranslatef(0,0.5f,0);
    glutSolidCube(1);
    glPopMatrix();

    /* top slab */
    glColor3f(0.98f,0.88f,0.95f);
    glPushMatrix();
    glTranslatef(0,2.0f,0);
    glScalef(8.7f,0.25f,4.4f);
    glutSolidCube(1);
    glPopMatrix();

    /* ===== POSTS ===== */
    float px[]={-4.0f,4.0f,-4.0f,4.0f};
    float pz[]={-2.0f,-2.0f,2.0f,2.0f};

    for(int i=0;i<4;i++){
        for(int st=0;st<5;st++){
            glColor3f(st%2?1.0f:0.95f, st%2?0.7f:0.4f, st%2?0.9f:0.7f);

            glPushMatrix();
            glTranslatef(px[i],2.0f+st*0.9f,pz[i]);
            glRotatef(-90,1,0,0);
            gluCylinder(q,0.18f,0.18f,0.9f,8,1);
            glPopMatrix();
        }
    }

    /* ===== AWNING ===== */
    drawStallAwning(4.2f,4.2f,7.8f,14, 1.0f,0.6f,0.9f);

    /* ===== BIG FRONT BMP LOGO ===== */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, iceTex);
    glColor3f(1,1,1);

    glPushMatrix();
    glTranslatef(0,9.5f,-3.2f);
    glRotatef(-12,1,0,0);

    glBegin(GL_QUADS);
      glTexCoord2f(0,1); glVertex3f(-4.5f,0,0);
        glTexCoord2f(1,1); glVertex3f( 4.5f,0,0);
        glTexCoord2f(1,0); glVertex3f( 4.5f,2.5f,0);
        glTexCoord2f(0,0); glVertex3f(-4.5f,2.5f,0);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    /* ===== ICE CREAM DISPLAY ===== */
    for(int i=-2;i<=2;i++){
        float x=i*1.5f;

        /* cone */
        glColor3f(0.9f,0.75f,0.5f);
        glPushMatrix();
        glTranslatef(x,2.2f,0.8f);
        glRotatef(180,1,0,0);
        gluCylinder(q,0.0f,0.4f,0.8f,10,1);
        glPopMatrix();

        /* scoop colors */
        if(i%3==0) glColor3f(1.0f,0.6f,0.8f);
        else if(i%3==1) glColor3f(0.6f,0.8f,1.0f);
        else glColor3f(1.0f,0.9f,0.5f);

        glPushMatrix();
        glTranslatef(x,2.5f,0.8f);
        glutSolidSphere(0.45f,12,12);
        glPopMatrix();
    }



    gluDeleteQuadric(q);
    glPopMatrix();
}

/* ─────────────────────────────
   DINING TABLE WITH UMBRELLA (food court)
   Enhanced with better chairs
───────────────────────────── */
void drawBigDiningTable(float x,float z,float r,float g,float b){
    glPushMatrix();glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();
    /* table legs */
    float lx[]={-1.0f,1.0f,-1.0f,1.0f},lz[]={-1.0f,-1.0f,1.0f,1.0f};
    glColor3f(0.52f,0.38f,0.22f);
    for(int i=0;i<4;i++){glPushMatrix();glTranslatef(lx[i],0,lz[i]);glRotatef(-90,1,0,0);
        gluCylinder(q,.075,.095,1.0f,8,1);glPopMatrix();}
    /* table top */
    glColor3f(0.82f,0.66f,0.44f);
    glPushMatrix();glTranslatef(0,1.02f,0);glRotatef(-90,1,0,0);gluDisk(q,0,1.35f,24,3);glPopMatrix();
    /* umbrella pole */
    glColor3f(0.40f,0.40f,0.45f);
    glPushMatrix();glTranslatef(0,1.02f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.065,.065,3.8f,8,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,4.85f,0);glutSolidSphere(0.16f,8,8);glPopMatrix();
    /* umbrella canopy 14 segments */
    int US=14;
    for(int s=0;s<US;s++){
        float a0=s*2*(float)M_PI/US,a1=(s+1)*2*(float)M_PI/US;
        glColor3f(s%2==0?r:1,s%2==0?g:1,s%2==0?b:1);
        glBegin(GL_TRIANGLES);
        glVertex3f(0,4.82f,0);
        glVertex3f(2.4f*cosf(a0),3.65f,2.4f*sinf(a0));
        glVertex3f(2.4f*cosf(a1),3.65f,2.4f*sinf(a1));
        glEnd();
    }
    /* 4 chairs (FIXED - face inward) */
    for(int c=0;c<4;c++){
    float angle = c * 90.0f * M_PI / 180.0f;

    float chx = 1.85f * cosf(angle);
    float chz = 1.85f * sinf(angle);

    glPushMatrix();
    glTranslatef(chx,0,chz);

    // 🔥 FACE CENTER (IMPORTANT)
    float rot = atan2f(-chx, -chz) * 180.0f / M_PI;
    glRotatef(rot,0,1,0);

    float clx[]={-.32f,.32f,-.32f,.32f};
    float clz[]={-.32f,-.32f,.32f,.32f};

    glColor3f(0.52f,0.36f,0.16f);
    for(int l=0;l<4;l++){
        glPushMatrix();
        glTranslatef(clx[l],0,clz[l]);
        glRotatef(-90,1,0,0);
        gluCylinder(q,.05f,.05f,.55f,6,1);
        glPopMatrix();
    }

    glColor3f(0.72f,0.50f,0.22f);
    glPushMatrix();
    glTranslatef(0,.58f,0);
    glScalef(.72f,.10f,.72f);
    glutSolidCube(1);
    glPopMatrix();

    // backrest posts
    for(int s=-1;s<=1;s+=2){
        glColor3f(0.52f,0.36f,0.16f);
        glPushMatrix();
        glTranslatef(s*.28f,.58f,-.30f);
        glRotatef(-90,1,0,0);
        gluCylinder(q,.04f,.04f,.68f,6,1);
        glPopMatrix();
    }

    glColor3f(0.68f,0.46f,0.18f);
    glPushMatrix();
    glTranslatef(0,.98f,-.30f);
    glScalef(.64f,.62f,.08f);
    glutSolidCube(1);
    glPopMatrix();

    glPopMatrix();
}
     
    gluDeleteQuadric(q);glPopMatrix();
}


/* ─────────────────────────────
   GAME STALL — Ring Toss
───────────────────────────── */
void drawRingTossStall(float cx,float cz,float ang){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(ang,0,1,0);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.48f,0.26f,0.10f);
    glPushMatrix();glScalef(3.5f,1.0f,2.2f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(.78f,.65f,.50f);
    glPushMatrix();glTranslatef(0,1.06f,0);glScalef(3.6f,.10f,2.3f);glutSolidCube(1);glPopMatrix();
    for(int i=-1;i<=1;i+=2){
        glColor3f(1.0f,0.1f,0.1f);
        glPushMatrix();glTranslatef(i*1.6f,1.1f,-1.0f);glRotatef(-90,1,0,0);
        gluCylinder(q,.12,.12,4.5f,8,1);glPopMatrix();
        glPushMatrix();glTranslatef(i*1.6f,1.1f,1.0f);glRotatef(-90,1,0,0);
        gluCylinder(q,.12,.12,4.5f,8,1);glPopMatrix();
        for(int s=0;s<4;s++){
            glColor3f(1,s%2==0?1:0.1f,0.1f);
            glPushMatrix();glTranslatef(i*1.6f,1.8f+s*.9f,-1.0f);
            gluCylinder(q,.13,.13,.4f,8,1);glPopMatrix();
            glPushMatrix();glTranslatef(i*1.6f,1.8f+s*.9f,1.0f);
            gluCylinder(q,.13,.13,.4f,8,1);glPopMatrix();
        }
    }
    for(int s=0;s<8;s++){
        glColor3f(s%2==0?.98f:.22f,s%2==0?.10f:.55f,s%2==0?.10f:.22f);
        float ax=-1.6f+s*.4f;
        glBegin(GL_QUADS);
        glVertex3f(ax,5.65f,-1.1f);glVertex3f(ax+.4f,5.65f,-1.1f);
        glVertex3f(ax+.4f,5.12f,-1.7f);glVertex3f(ax,5.12f,-1.7f);glEnd();
        glBegin(GL_QUADS);
        glVertex3f(ax,5.65f,-1.1f);glVertex3f(ax+.4f,5.65f,-1.1f);
        glVertex3f(ax+.4f,5.65f,1.1f);glVertex3f(ax,5.65f,1.1f);glEnd();
        glBegin(GL_QUADS);
        glVertex3f(ax,5.65f,1.1f);glVertex3f(ax+.4f,5.65f,1.1f);
        glVertex3f(ax+.4f,5.12f,1.7f);glVertex3f(ax,5.12f,1.7f);glEnd();
    }
    for(int p=0;p<3;p++){
        glColor3f(0.22f,0.22f,0.22f);
        glPushMatrix();glTranslatef(-0.8f+p*.8f,1.1f,0);glRotatef(-90,1,0,0);
        gluCylinder(q,.06,.06,.8f,6,1);glPopMatrix();
    }
    float ringC[][3]={{1,.2f,.2f},{.2f,.2f,1},{1,.88f,.0f}};
    for(int r=0;r<3;r++){
        glColor3f(ringC[r][0],ringC[r][1],ringC[r][2]);
        glPushMatrix();glTranslatef(-0.8f+r*.8f,2.0f,0);
        glRotatef(-90,1,0,0);
        gluDisk(q,.22f,.32f,14,2);glPopMatrix();
    }
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   SHOOTING GALLERY
───────────────────────────── */
void drawShootingGallery(float cx,float cz,float ang){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(ang,0,1,0);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.18f,0.28f,0.68f);
    glPushMatrix();glScalef(5.0f,1.2f,2.5f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(.75f,.72f,.65f);
    glPushMatrix();glTranslatef(0,1.28f,0);glScalef(5.2f,.12f,2.6f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.22f,0.38f,0.78f);
    glPushMatrix();glTranslatef(0,3.2f,-1.2f);glScalef(5.2f,4.2f,.15f);glutSolidCube(1);glPopMatrix();
    for(int t=0;t<5;t++){
        float ta=sinf(windTime*0.8f+t*1.2f);
        glColor3f(0.95f,0.78f,0.12f);
        glPushMatrix();glTranslatef(-1.8f+t*.9f,3.5f+ta*.4f,-1.05f);
        glScalef(.35f,.28f,.15f);glutSolidSphere(1,8,8);glPopMatrix();
        glColor3f(1.0f,0.6f,0.0f);
        glPushMatrix();glTranslatef(-1.4f+t*.9f,3.6f+ta*.4f,-1.05f);
        glScalef(.18f,.12f,.08f);glutSolidSphere(1,6,6);glPopMatrix();
    }
    float bc[][3]={{1,.1f,.1f},{.1f,.5f,1},{.1f,.9f,.2f},{1,.85f,.0f}};
    for(int p=0;p<4;p++){
        glColor3f(bc[p][0],bc[p][1],bc[p][2]);
        glPushMatrix();glTranslatef(-1.4f+p*.9f,4.5f,-1.1f);
        glScalef(.5f,.6f,.5f);glutSolidSphere(1,8,8);glPopMatrix();
        glColor3f(1,1,0);
        glPushMatrix();glTranslatef(-1.4f+p*.9f,4.9f,-.8f);
        glutSolidSphere(.12f,6,6);glPopMatrix();
    }
    glColor3f(.42f,.28f,.12f);
    glPushMatrix();glTranslatef(0,1.38f,1.1f);glScalef(5.0f,.18f,.32f);glutSolidCube(1);glPopMatrix();
    for(int s=0;s<10;s++){
        glColor3f(s%2==0?.12f:.98f,s%2==0?.28f:.10f,s%2==0?.68f:.10f);
        float ax=-2.4f+s*.5f;
        glBegin(GL_QUADS);
        glVertex3f(ax,5.5f,1.3f);glVertex3f(ax+.5f,5.5f,1.3f);
        glVertex3f(ax+.5f,4.9f,1.9f);glVertex3f(ax,4.9f,1.9f);glEnd();
        glBegin(GL_QUADS);
        glVertex3f(ax,5.5f,-1.3f);glVertex3f(ax+.5f,5.5f,-1.3f);
        glVertex3f(ax+.5f,5.5f,1.3f);glVertex3f(ax,5.5f,1.3f);glEnd();
    }
    gluDeleteQuadric(q);glPopMatrix();
}

void drawCircusTent(float cx, float cz) {
    glPushMatrix();
    glTranslatef(cx, 0.0f, cz);
    GLUquadric* q = gluNewQuadric();
 
    const int SEGS  = 48;
    const float TR  = 11.5f;   /* tent base radius */
    const float TH  = 9.5f;   /* tent height above base */
    const float BR  = 12.2f;  /* base wall radius */
    const float BH  = 2.8f;   /* cylindrical base wall height */
 
    /* ── ground disc ── */
    glColor3f(0.60f, 0.56f, 0.48f);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluDisk(q, 0, BR + 0.5f, SEGS, 3);
    glPopMatrix();
 
    /* ── cylindrical base wall with red/white stripes ── */
    for (int s = 0; s < SEGS; s++) {
        float a0 = s       * 2.0f * (float)M_PI / SEGS;
        float a1 = (s + 1) * 2.0f * (float)M_PI / SEGS;
        glColor3f(s % 2 == 0 ? 0.90f : 0.98f,
                  s % 2 == 0 ? 0.10f : 0.98f,
                  s % 2 == 0 ? 0.10f : 0.98f);
        glBegin(GL_QUADS);
        glVertex3f(BR * cosf(a0), 0,   BR * sinf(a0));
        glVertex3f(BR * cosf(a1), 0,   BR * sinf(a1));
        glVertex3f(BR * cosf(a1), BH,  BR * sinf(a1));
        glVertex3f(BR * cosf(a0), BH,  BR * sinf(a0));
        glEnd();
    }
    /* base wall top ring */
    glColor3f(0.80f, 0.72f, 0.20f);
    glPushMatrix();
    glTranslatef(0, BH, 0);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, BR, BR, 0.22f, SEGS, 1);
    glPopMatrix();
 
    /* ── main conical tent roof with red/white panels ── */
    const int PANELS = 20;
    float panelC[2][3] = {{0.90f, 0.10f, 0.10f}, {0.98f, 0.98f, 0.98f}};
    for (int p = 0; p < PANELS; p++) {
        float a0 = p       * 2.0f * (float)M_PI / PANELS;
        float a1 = (p + 1) * 2.0f * (float)M_PI / PANELS;
        glColor3f(panelC[p % 2][0], panelC[p % 2][1], panelC[p % 2][2]);
        /* triangular panel from rim to apex */
        glBegin(GL_TRIANGLES);
        glVertex3f(TR * cosf(a0), BH,          TR * sinf(a0));
        glVertex3f(TR * cosf(a1), BH,          TR * sinf(a1));
        glVertex3f(0,              BH + TH,     0);
        glEnd();
        /* inside face (back-face for interior peek) */
        glColor3f(panelC[p % 2][0] * 0.62f,
                  panelC[p % 2][1] * 0.62f,
                  panelC[p % 2][2] * 0.62f);
        glBegin(GL_TRIANGLES);
        glVertex3f(TR * cosf(a1), BH,       TR * sinf(a1));
        glVertex3f(TR * cosf(a0), BH,       TR * sinf(a0));
        glVertex3f(0,              BH + TH,  0);
        glEnd();
    }
 
    /* ── outer valance (decorative skirt around base of cone) ── */
    for (int p = 0; p < PANELS * 2; p++) {
        float a0 = p       * 2.0f * (float)M_PI / (PANELS * 2);
        float a1 = (p + 1) * 2.0f * (float)M_PI / (PANELS * 2);
        glColor3f(p % 2 == 0 ? 0.90f : 0.98f,
                  p % 2 == 0 ? 0.10f : 0.98f,
                  p % 2 == 0 ? 0.10f : 0.98f);
        float rInner = TR - 0.5f, rOuter = TR + 1.2f;
        glBegin(GL_TRIANGLES);
        glVertex3f(rInner * cosf(a0), BH,        rInner * sinf(a0));
        glVertex3f(rInner * cosf(a1), BH,        rInner * sinf(a1));
        glVertex3f(rOuter * cosf((a0+a1)*0.5f), BH - 1.0f, rOuter * sinf((a0+a1)*0.5f));
        glEnd();
    }
 
    /* ── centre main pole (inside, pokes through apex) ── */
    glColor3f(0.30f, 0.28f, 0.24f);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.42f, 0.35f, BH + TH + 2.5f, 12, 2);
    glPopMatrix();
 
    /* ── 3 smaller outer poles ── */
    for (int op = 0; op < 3; op++) {
        float oa = op * 120.0f * (float)M_PI / 180.0f;
        glColor3f(0.32f, 0.28f, 0.22f);
        glPushMatrix();
        glTranslatef(cosf(oa) * 7.5f, 0, sinf(oa) * 7.5f);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.28f, 0.22f, BH + 2.0f, 10, 1);
        glPopMatrix();
    }
 
    /* ── golden star on apex ── */
    glColor3f(0.95f, 0.85f, 0.10f);
    glPushMatrix();
    glTranslatef(0, BH + TH + 0.5f, 0);
    glutSolidSphere(0.55f, 14, 14);
    glPopMatrix();
    /* apex ring of small balls */
    for (int i = 0; i < 8; i++) {
        float ba = i * 45.0f * (float)M_PI / 180.0f;
        glColor3f(0.98f, 0.82f, 0.08f);
        glPushMatrix();
        glTranslatef(cosf(ba) * 0.80f, BH + TH + 0.5f, sinf(ba) * 0.80f);
        glutSolidSphere(0.18f, 8, 8);
        glPopMatrix();
    }
 
    
  
 
    /* ── pennant flags around perimeter ── */
    extern float windTime;
    float flagC[6][3] = {
        {0.92f,0.18f,0.18f},{0.18f,0.52f,0.92f},{0.18f,0.82f,0.22f},
        {0.92f,0.72f,0.10f},{0.82f,0.18f,0.82f},{0.92f,0.48f,0.10f}
    };
    const int FLAGS = 12;
    for (int f = 0; f < FLAGS; f++) {
        float fa  = f * 2.0f * (float)M_PI / FLAGS;
        float fx  = cosf(fa) * (TR + 0.8f);
        float fz  = sinf(fa) * (TR + 0.8f);
        float sway = sinf(windTime * 1.5f + f * 0.8f) * 0.18f;
        glColor3f(flagC[f % 6][0], flagC[f % 6][1], flagC[f % 6][2]);
        glBegin(GL_TRIANGLES);
        glVertex3f(fx,          BH + TH,      fz);
        glVertex3f(fx + 0.55f + sway, BH + TH - 0.5f, fz);
        glVertex3f(fx,          BH + TH - 0.9f, fz);
        glEnd();
        /* flag rope */
        glDisable(GL_LIGHTING);
        glColor3f(0.22f, 0.22f, 0.22f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex3f(0,  BH + TH + 0.5f, 0);
        glVertex3f(fx, BH + TH,        fz);
        glEnd();
        glEnable(GL_LIGHTING);
    }
 
    
 
   
 
    gluDeleteQuadric(q);
    glPopMatrix();
}
void updateFountain(){
    waterTime+=0.03f;
    glutPostRedisplay();
}
/* ══════════════════════════════════════════════
   WATER SURFACE  — flat horizontal textured disc
   Must be called inside a glPushMatrix block
   that is already translated to the water height
   ══════════════════════════════════════════════ */
void drawWaterCircle(float radius) {
    extern GLuint waterTex;
    extern float  waterTime;

    /* flat rotation — CRITICAL: discs must face up */
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    if (waterTex) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, waterTex);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.35f, 0.72f, 0.95f, 0.82f);   /* nice blue, semi-transparent */

    float t = waterTime;

    /* centre vertex */
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.5f + t * 0.08f, 0.5f + t * 0.08f);
    glVertex3f(0.0f, 0.0f, 0.0f);

    for (int i = 0; i <= 360; i += 4) {
        float ang = i * (float)M_PI / 180.0f;
        float x   = cosf(ang) * radius;
        float y   = sinf(ang) * radius;
        /* subtle ripple on UV only — keep geometry flat */
        float u   = (x / radius) * 0.5f + 0.5f + t * 0.06f;
         float v   = (y / radius) * 0.5f + 0.5f + t * 0.04f;
        glTexCoord2f(u, v);
        glVertex3f(x, y, 0.0f);
    }
    glEnd();

    glDisable(GL_BLEND);
    if (waterTex) glDisable(GL_TEXTURE_2D);
}

void drawWaterJets(float radius) {
    extern float waterTime;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);

    /* outer ring — 12 jets */
    for (int i = 0; i < 12; i++) {
        float baseAngle = i * 30.0f * (float)M_PI / 180.0f;
        /* phase-offset height so jets pulse slightly */
        float phaseH    = sinf(waterTime * 1.2f + i * 0.52f) * 0.4f + 2.6f;

        glColor4f(0.65f, 0.88f, 1.0f, 0.70f);
        glLineWidth(2.2f);
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j <= 22; j++) {
            float t  = (float)j / 22.0f;
            float r  = radius * t;
            float x  = cosf(baseAngle) * r;
            float z  = sinf(baseAngle) * r;
            float y  = phaseH * sinf(t * (float)M_PI);   /* parabolic arc */
            glVertex3f(x, y, z);
        }
        glEnd();
    }

    /* inner ring — 6 shorter jets */
    for (int i = 0; i < 6; i++) {
        float baseAngle = i * 60.0f * (float)M_PI / 180.0f + 15.0f * (float)M_PI / 180.0f;
        float phaseH    = sinf(waterTime + i * 1.05f) * 0.3f + 1.8f;
         glColor4f(0.72f, 0.92f, 1.0f, 0.80f);
        glLineWidth(1.8f);
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j <= 18; j++) {
            float t  = (float)j / 18.0f;
            float r  = (radius * 0.55f) * t;
            float x  = cosf(baseAngle) * r;
            float z  = sinf(baseAngle) * r;
            float y  = phaseH * sinf(t * (float)M_PI);
            glVertex3f(x, y, z);
        }
        glEnd();
    }

    /* top spout — vertical fan */
    for (int i = 0; i < 8; i++) {
        float sa    = i * 45.0f * (float)M_PI / 180.0f;
        float phaseH = sinf(waterTime * 1.4f + i * 0.78f) * 0.35f + 2.2f;
        glColor4f(0.78f, 0.94f, 1.0f, 0.75f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j <= 15; j++) {
            float t  = (float)j / 15.0f;
            float r  = 1.2f * t;
            float x  = cosf(sa) * r;
            float z  = sinf(sa) * r;
            float y  = phaseH * sinf(t * (float)M_PI);
            glVertex3f(x, y, z);
        }
        glEnd();
    }
     glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}


/* ─────────────────────────────
   GRAND FOUNTAIN
───────────────────────────── */
void drawFountain(float cx,float cz){
glPushMatrix();
    glTranslatef(cx, 0.0f, cz);
    GLUquadric* q = gluNewQuadric();

    /* ────────────────────────────────────────────
       TIER 1  —  outer basin
       Ground level → rim at Y = 1.05
    ──────────────────────────────────────────── */

    /* basin floor disc */
    glColor3f(0.68f, 0.72f, 0.80f);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);   /* FLAT */
    gluDisk(q, 0.0f, 7.2f, 40, 4);
    glPopMatrix();

    /* basin side wall */
    glColor3f(0.72f, 0.75f, 0.82f);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);   /* UPRIGHT cylinder */
    gluCylinder(q, 7.2f, 7.2f, 0.90f, 40, 1);
    glPopMatrix();

    /* basin rim cap */
    glColor3f(0.80f, 0.82f, 0.90f);
    glPushMatrix();
    glTranslatef(0.0f, 0.90f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 7.2f, 7.6f, 0.30f, 40, 1);
    glPopMatrix();
/* rim top flat ring */
    glPushMatrix();
    glTranslatef(0.0f, 1.20f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluDisk(q, 6.8f, 7.6f, 40, 2);
    glPopMatrix();

    /* decorative border stones */
    for (int s = 0; s < 20; s++) {
        float sa = s * 18.0f * (float)M_PI / 180.0f;
        glColor3f(0.70f, 0.72f, 0.78f);
        glPushMatrix();
        glTranslatef(cosf(sa) * 7.95f, 0.10f, sinf(sa) * 7.95f);
        glScalef(0.60f, 0.40f, 0.60f);
        glutSolidSphere(1.0f, 8, 8);
        glPopMatrix();
    }

    /* WATER SURFACE in outer basin */
    glPushMatrix();
    glTranslatef(0.0f, 0.78f, 0.0f);
    drawWaterCircle(6.8f);              /* drawWaterCircle applies its own -90 rotation */
    glPopMatrix();

    /* ────────────────────────────────────────────
       CENTRE PILLAR  (tier 1 → tier 2)
       Rises from Y=0 to Y=4.0
    ──────────────────────────────────────────── */
    glColor3f(0.80f, 0.82f, 0.90f);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 0.75f, 0.60f, 4.0f, 18, 2);glPopMatrix();

    /* decorative collar at base of pillar */
    glColor3f(0.90f, 0.84f, 0.62f);
    glPushMatrix();
    glTranslatef(0.0f, 0.42f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 1.15f, 1.05f, 0.36f, 18, 1);
    glPopMatrix();

    /* ────────────────────────────────────────────
       TIER 2  —  middle basin at Y = 4.0
    ──────────────────────────────────────────── */
    glColor3f(0.72f, 0.76f, 0.84f);
    glPushMatrix();
    glTranslatef(0.0f, 4.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluDisk(q, 0.0f, 4.4f, 28, 3);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 4.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 4.4f, 4.4f, 0.55f, 28, 1);
    glPopMatrix();

    /* middle basin rim */
    glColor3f(0.80f, 0.84f, 0.92f);
    glPushMatrix();
    glTranslatef(0.0f, 4.55f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 4.4f, 4.75f, 0.22f, 28, 1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, 4.77f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluDisk(q, 4.1f, 4.75f, 28, 2);
    glPopMatrix();

    /* WATER in middle basin */
    glPushMatrix();
    glTranslatef(0.0f, 4.52f, 0.0f);
    drawWaterCircle(4.1f);
    glPopMatrix();

    /* pillar tier 2 → tier 3 */
    glColor3f(0.80f, 0.82f, 0.90f);
    glPushMatrix();
    glTranslatef(0.0f, 4.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 0.52f, 0.40f, 3.6f, 14, 2);
    glPopMatrix();

    /* ────────────────────────────────────────────
       TIER 3  —  top basin at Y = 7.6
    ──────────────────────────────────────────── */
    glColor3f(0.76f, 0.80f, 0.88f);
    glPushMatrix();
    glTranslatef(0.0f, 7.6f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluDisk(q, 0.0f, 2.2f, 22, 3);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 7.6f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 2.2f, 2.2f, 0.42f, 22, 1);
    glPopMatrix();

    glColor3f(0.82f, 0.86f, 0.94f);
    glPushMatrix();
    glTranslatef(0.0f, 8.02f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 2.2f, 2.5f, 0.20f, 22, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 8.22f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluDisk(q, 2.0f, 2.5f, 22, 2);
    glPopMatrix();

    /* WATER in top basin */
    glPushMatrix();
    glTranslatef(0.0f, 8.0f, 0.0f);
    drawWaterCircle(2.0f);
    glPopMatrix();

    /* ────────────────────────────────────────────
       GOLDEN SPIRE on top
    ──────────────────────────────────────────── */
    glColor3f(0.92f, 0.80f, 0.15f);
    glPushMatrix();
    glTranslatef(0.0f, 7.6f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 0.20f, 0.10f, 3.2f, 10, 1);
    glPopMatrix();
    /* top globe */
    glColor3f(1.0f, 0.88f, 0.20f);
    glPushMatrix();
    glTranslatef(0.0f, 10.84f, 0.0f);
    glutSolidSphere(0.52f, 18, 18);
    glPopMatrix();

    /* ring of small gold balls */
    for (int t = 0; t < 6; t++) {
        float ta = t * 60.0f * (float)M_PI / 180.0f;
        glColor3f(1.0f, 0.90f, 0.18f);
        glPushMatrix();
        glTranslatef(cosf(ta) * 0.72f, 10.84f, sinf(ta) * 0.72f);
        glutSolidSphere(0.20f, 8, 8);
        glPopMatrix();
    }

    /* ────────────────────────────────────────────
       WATER JETS  (3 layers matching the 3 basins)
    ──────────────────────────────────────────── */
    /* outer jets from ground level */
    glPushMatrix();
    glTranslatef(0.0f, 0.85f, 0.0f);
    drawWaterJets(6.2f);
    glPopMatrix();

    /* middle jets */
    glPushMatrix();
    glTranslatef(0.0f, 4.58f, 0.0f);
    drawWaterJets(3.5f);
    glPopMatrix();
     /* top jets (small radius) */
    glPushMatrix();
    glTranslatef(0.0f, 8.08f, 0.0f);
    drawWaterJets(1.4f);
    glPopMatrix();

    gluDeleteQuadric(q);
    glPopMatrix();
}

/* ─────────────────────────────
   INFO KIOSK
───────────────────────────── */
void drawInfoKiosk(float x,float z,float ry){
    glPushMatrix();glTranslatef(x,0,z);glRotatef(ry,0,1,0);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.22f,0.42f,0.68f);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.18,.18,1.2f,10,1);glPopMatrix();
    glColor3f(0.18f,0.32f,0.58f);
    glPushMatrix();glTranslatef(0,.1f,0);glScalef(1.0f,.2f,1.0f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.18f,0.38f,0.72f);
    glPushMatrix();glTranslatef(0,2.4f,0);glScalef(2.8f,2.5f,.6f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.05f,0.05f,0.08f);
    glPushMatrix();glTranslatef(0,2.5f,.32f);glScalef(2.3f,2.0f,.06f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.22f,0.72f,0.28f);
    glPushMatrix();glTranslatef(0,2.5f,.36f);glScalef(2.0f,1.7f,.04f);glutSolidCube(1);glPopMatrix();
    glColor3f(1,1,1);
    glPushMatrix();glTranslatef(0,3.6f,.38f);glutSolidSphere(.18f,8,8);glPopMatrix();
    glColor3f(0.12f,0.28f,0.62f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-1.6f,3.7f,-.42f);glVertex3f(1.6f,3.7f,-.42f);glVertex3f(0,4.6f,0);
    glVertex3f(-1.6f,3.7f,.42f);glVertex3f(1.6f,3.7f,.42f);glVertex3f(0,4.6f,0);
    glVertex3f(-1.6f,3.7f,-.42f);glVertex3f(-1.6f,3.7f,.42f);glVertex3f(0,4.6f,0);
    glVertex3f(1.6f,3.7f,-.42f);glVertex3f(1.6f,3.7f,.42f);glVertex3f(0,4.6f,0);
    glEnd();
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   MERRY-GO-ROUND
───────────────────────────── */
void drawMerryGoRound(float cx,float cz,float spin){
    glPushMatrix();
    glTranslatef(cx,0,cz);

    GLUquadric* q = gluNewQuadric();

    float radius = 6.5f;   // 🔥 bigger size

    // =========================
    // BASE PLATFORM
    // =========================
    glColor3f(0.7f,0.2f,0.6f);
    glPushMatrix();
    glRotatef(-90,1,0,0);
    gluDisk(q,0,radius,40,2);
    glPopMatrix();

    // edge
    glColor3f(0.9f,0.8f,0.2f);
    glPushMatrix();
    glTranslatef(0,0.2f,0);
    glRotatef(-90,1,0,0);
    gluCylinder(q,radius,radius,0.5f,40,1);
    glPopMatrix();

    // =========================
    // CENTER POLE
    // =========================
    glColor3f(0.8f,0.8f,0.85f);
    glPushMatrix();
    glRotatef(-90,1,0,0);
    gluCylinder(q,0.4f,0.4f,11.0f,20,1);
    glPopMatrix();

    // =========================
    // TOP CANOPY
    // =========================
    glPushMatrix();
    glTranslatef(0,11.5f,0);
    glRotatef(spin,0,1,0);

    int segments = 20;
    float topR = radius;

    for(int i=0;i<segments;i++){
        float a0 = i*(360.0f/segments)*M_PI/180;
        float a1 = (i+1)*(360.0f/segments)*M_PI/180;

        // alternating colors
        if(i%2==0) glColor3f(1.0f,0.2f,0.5f);
        else       glColor3f(1.0f,0.9f,0.2f);

        glBegin(GL_TRIANGLES);
        glVertex3f(0,2.0f,0);
        glVertex3f(topR*cosf(a0),0,topR*sinf(a0));
        glVertex3f(topR*cosf(a1),0,topR*sinf(a1));
        glEnd();
    }

    glPopMatrix();

    // =========================
    // HORSES + POLES
    // =========================
    glPushMatrix();
    glRotatef(spin,0,1,0);

    int horses = 8;

    for(int i=0;i<horses;i++){
        float angle = i*(360.0f/horses)*M_PI/180;
        float x = (radius-1.5f)*cosf(angle);
        float z = (radius-1.5f)*sinf(angle);

        float bob = 1.5f * sinf(spin*0.05f + i); // up-down motion

        // pole
        glColor3f(0.9f,0.9f,0.9f);
        glPushMatrix();
        glTranslatef(x,1.0f+bob,z);
        glRotatef(-90,1,0,0);
        gluCylinder(q,0.1f,0.1f,10.0f-bob,8,1);
        glPopMatrix();

        // horse (simple stylized body)
        glPushMatrix();
        glTranslatef(x,2.0f+bob,z);
        glRotatef(-angle*180/M_PI+90,0,1,0);

        // body
        glColor3f(0.9f,0.8f,0.7f);
        glPushMatrix();
        glScalef(1.2f,0.6f,0.5f);
        glutSolidCube(1);
        glPopMatrix();

        // head
        glPushMatrix();
        glTranslatef(0.6f,0.3f,0);
        glScalef(0.4f,0.4f,0.4f);
        glutSolidSphere(1,12,12);
        glPopMatrix();

        // legs
        glColor3f(0.3f,0.2f,0.1f);
        for(int l=0;l<4;l++){
            float lx = (l<2 ? -0.3f : 0.3f);
            float lz = (l%2==0 ? -0.2f : 0.2f);

            glPushMatrix();
            glTranslatef(lx,-0.4f,lz);
            glScalef(0.1f,0.6f,0.1f);
            glutSolidCube(1);
            glPopMatrix();
        }

        glPopMatrix();
    }

    glPopMatrix();

    // top ball
    glColor3f(1.0f,0.9f,0.1f);
    glPushMatrix();
    glTranslatef(0,13.5f,0);
    glutSolidSphere(0.7f,20,20);
    glPopMatrix();

    gluDeleteQuadric(q);
    glPopMatrix();
}
/* ─────────────────────────────
   GIANT FERRIS WHEEL
───────────────────────────── */
void drawGiantWheel(float cx,float cz){
    /* draw plaza first */
    
   

    glPushMatrix();glTranslatef(cx,0,cz);
    const float HH=26.0f, WR=18.0f, LS=10.0f;
    GLUquadric*q=gluNewQuadric();

    /* ── A-frame support legs ── */
    glColor3f(0.20f,0.22f,0.26f);
    for(int s=-1;s<=1;s+=2){
        glPushMatrix();glTranslatef(s*LS,0,2.2f);
        glRotatef(s*13.0f,0,0,1);glRotatef(-90,1,0,0);glScalef(0.9f,0.9f,1);
        gluCylinder(q,0.58f,0.42f,HH+4,14,2);glPopMatrix();
        glPushMatrix();glTranslatef(s*LS,0,-2.2f);
        glRotatef(s*13.0f,0,0,1);glRotatef(-90,1,0,0);glScalef(0.9f,0.9f,1);
        gluCylinder(q,0.58f,0.42f,HH+4,14,2);glPopMatrix();
    }
    /* cross-braces */
    glColor3f(0.28f,0.30f,0.36f);
    for(int b=0;b<3;b++){float bh=HH*(0.25f+b*0.22f);
        glPushMatrix();glTranslatef(-LS,bh,0);glRotatef(90,0,1,0);
        gluCylinder(q,0.28f,0.28f,LS*2,8,1);glPopMatrix();}
    /* diagonal X brace */
    for(int side=-1;side<=1;side+=2){
        float x0=-LS*0.82f,y0=HH*0.18f,x1=LS*0.82f,y1=HH*0.78f;
        if(side<0){float t=x0;x0=x1;x1=t;t=y0;y0=y1;y1=t;}
        float dbx=x1-x0,dby=y1-y0,dl=sqrtf(dbx*dbx+dby*dby);
        glColor3f(0.32f,0.32f,0.38f);
        glPushMatrix();glTranslatef(x0,y0,0);
        glRotatef(atan2f(dby,dbx)*180/(float)M_PI,0,0,1);
        glRotatef(90,0,1,0);gluCylinder(q,0.22f,0.22f,dl,8,1);glPopMatrix();}
    /* axle support columns */
    glColor3f(0.26f,0.28f,0.32f);
    for(int sz=-1;sz<=1;sz+=2){
        glPushMatrix();glTranslatef(0,0,sz*3.2f);glRotatef(-90,1,0,0);
        gluCylinder(q,0.48f,0.48f,HH+1.5f,12,1);glPopMatrix();}
    /* axle housing */
    glColor3f(0.22f,0.22f,0.28f);
    glPushMatrix();glTranslatef(0,HH,0);glScalef(2.8f,2.0f,2.8f);glutSolidCube(1);glPopMatrix();
    /* axle shaft */
    glColor3f(0.58f,0.58f,0.65f);
    glPushMatrix();glTranslatef(0,HH,-3.8f);glRotatef(-90,1,0,0);
    gluCylinder(q,0.52f,0.52f,7.6f,14,1);glPopMatrix();

    /* motor house */
    glColor3f(0.22f,0.22f,0.26f);
    glPushMatrix();glTranslatef(LS+2.0f,1.2f,0);glScalef(3.0f,2.4f,2.4f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.92f,0.72f,0.08f);
    for(int i=0;i<5;i++){glPushMatrix();glTranslatef(LS+0.6f+i*0.55f,0.95f,1.25f);
        glScalef(0.25f,1.9f,0.04f);glutSolidCube(1);glPopMatrix();}
    /* safety railing */
    glColor3f(0.88f,0.72f,0.10f);
    for(int s=0;s<8;s++){float ra=s*45*(float)M_PI/180;
        glPushMatrix();glTranslatef(cosf(ra)*4.0f,0,sinf(ra)*4.0f);glRotatef(-90,1,0,0);
        gluCylinder(q,.09f,.09f,1.2f,6,1);glPopMatrix();}
    glDisable(GL_LIGHTING);
    glColor3f(0.88f,0.72f,0.10f); glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<32;i++){float a=i*2*(float)M_PI/32;glVertex3f(cosf(a)*4.0f,1.2f,sinf(a)*4.0f);}
    glEnd();
    glLineWidth(1.0f); glEnable(GL_LIGHTING);

    /* ══════════ ROTATING WHEEL ══════════ */
    glPushMatrix();glTranslatef(0,HH,0);glRotatef(wheelAngle,0,0,1);

    /* outer rim (front + back) with LED chase */
    const int RS=72;
    for(int ring=0;ring<2;ring++){float rz=(ring==0)?-2.2f:2.2f;
        for(int s=0;s<RS;s++){
            float a0=s*2*(float)M_PI/RS,a1=(s+1)*2*(float)M_PI/RS;
            float hue=ledTime*2.2f+s*0.088f;
            float lr=0.5f+0.5f*sinf(hue),lg=0.5f+0.5f*sinf(hue+2.1f),lb=0.5f+0.5f*sinf(hue+4.2f);
            glColor3f(0.72f+lr*0.24f,0.12f+lg*0.1f,0.12f+lb*0.1f);
            glBegin(GL_QUADS);
            glVertex3f(WR*cosf(a0),WR*sinf(a0),rz-0.35f);
            glVertex3f(WR*cosf(a1),WR*sinf(a1),rz-0.35f);
            glVertex3f(WR*cosf(a1),WR*sinf(a1),rz+0.35f);
            glVertex3f(WR*cosf(a0),WR*sinf(a0),rz+0.35f);
            glEnd();}
        /* rim inner face */
        glColor3f(0.52f,0.52f,0.58f);
        for(int s=0;s<RS;s++){float a0=s*2*(float)M_PI/RS,a1=(s+1)*2*(float)M_PI/RS;
            float r2=WR-0.62f;
            glBegin(GL_QUADS);
            glVertex3f(r2*cosf(a0),r2*sinf(a0),rz-0.35f);
            glVertex3f(r2*cosf(a1),r2*sinf(a1),rz-0.35f);
            glVertex3f(r2*cosf(a1),r2*sinf(a1),rz+0.35f);
            glVertex3f(r2*cosf(a0),r2*sinf(a0),rz+0.35f);
            glEnd();}
    }
    /* cross-ties */
    glColor3f(0.45f,0.45f,0.52f);
    for(int s=0;s<RS;s+=4){float a=s*2*(float)M_PI/RS;
        glBegin(GL_LINES);glVertex3f(WR*cosf(a),WR*sinf(a),-2.2f);
        glVertex3f(WR*cosf(a),WR*sinf(a),2.2f);glEnd();}
    /* inner ring */
    float iR2=WR*0.55f;
    glColor3f(0.38f,0.38f,0.44f);
    for(int s=0;s<RS;s++){float a0=s*2*(float)M_PI/RS,a1=(s+1)*2*(float)M_PI/RS;
        glBegin(GL_QUADS);
        glVertex3f(iR2*cosf(a0),iR2*sinf(a0),-0.20f);
        glVertex3f(iR2*cosf(a1),iR2*sinf(a1),-0.20f);
        glVertex3f(iR2*cosf(a1),iR2*sinf(a1), 0.20f);
        glVertex3f(iR2*cosf(a0),iR2*sinf(a0), 0.20f);
        glEnd();}

    /* spokes — 20, box section with LED */
    const int SPOKES=20;
    for(int sp=0;sp<SPOKES;sp++){
        float sa=sp*2*(float)M_PI/SPOKES;
        float sx=WR*cosf(sa),sy=WR*sinf(sa),dl=sqrtf(sx*sx+sy*sy);
        float angle=atan2f(sy,sx)*180/(float)M_PI;
        glColor3f(0.32f,0.32f,0.38f);
        glPushMatrix();glRotatef(angle,0,0,1);glTranslatef(0,0,-1.8f);
        glScalef(dl,0.32f,3.6f);glutSolidCube(1);glPopMatrix();
        /* LED neon line */
        glDisable(GL_LIGHTING);
        float hue=ledTime*1.8f+sp*0.31f;
        glColor3f(0.5f+0.5f*sinf(hue),0.5f+0.5f*sinf(hue+2.1f),0.5f+0.5f*sinf(hue+4.2f));
        glLineWidth(2.5f);
        glBegin(GL_LINES);glVertex3f(0,0,0);glVertex3f(sx,sy,0);glEnd();
        glLineWidth(1.0f);
        /* midpoint glow bead */
        float hue2=ledTime*2.4f+sp*0.42f;
        glColor3f(0.5f+0.5f*sinf(hue2),0.5f+0.5f*sinf(hue2+2.1f),0.5f+0.5f*sinf(hue2+4.2f));
        glPushMatrix();glTranslatef(sx*.58f,sy*.58f,0);glutSolidSphere(0.30f,8,8);glPopMatrix();
        glEnable(GL_LIGHTING);
    }
    /* hub */
    glColor3f(0.90f,0.84f,0.12f);
    glPushMatrix();glRotatef(-90,1,0,0);gluDisk(q,0,2.0f,18,3);glPopMatrix();
    glPushMatrix();glRotatef(90,1,0,0);gluDisk(q,0,2.0f,18,3);glPopMatrix();
    glColor3f(0.98f,0.92f,0.22f);glutSolidSphere(1.5f,18,18);
    /* hub LED ring */
    glDisable(GL_LIGHTING);
    for(int i=0;i<20;i++){float hue=ledTime*3.2f+i*0.31f;
        glColor3f(0.5f+0.5f*sinf(hue),0.5f+0.5f*sinf(hue+2.1f),0.5f+0.5f*sinf(hue+4.2f));
        float a=i*2*(float)M_PI/20;
        glPushMatrix();glTranslatef(cosf(a)*2.1f,sinf(a)*2.1f,0);glutSolidSphere(0.20f,6,6);glPopMatrix();}
    glEnable(GL_LIGHTING);

    /* gondolas — 18 premium cabins */
    const int NG=18;
    float gc[][3]={
        {0.90f,0.18f,0.18f},{0.18f,0.50f,0.92f},{0.18f,0.82f,0.22f},{0.92f,0.72f,0.10f},
        {0.82f,0.18f,0.82f},{0.10f,0.82f,0.82f},{0.92f,0.48f,0.10f},{0.18f,0.18f,0.92f},
        {0.92f,0.18f,0.58f},{0.28f,0.78f,0.28f},{0.88f,0.88f,0.18f},{0.18f,0.58f,0.88f},
        {0.78f,0.28f,0.18f},{0.28f,0.28f,0.78f},{0.88f,0.48f,0.28f},{0.28f,0.88f,0.58f},
        {0.92f,0.62f,0.18f},{0.62f,0.18f,0.88f}
    };
    for(int g=0;g<NG;g++){
        float ga=g*2*(float)M_PI/NG;
        float gx=WR*cosf(ga),gy=WR*sinf(ga);
        glPushMatrix();glTranslatef(gx,gy,0);glRotatef(-wheelAngle,0,0,1);
        /* hanger arm */
        glColor3f(0.42f,0.42f,0.50f);
        glPushMatrix();glTranslatef(0,0,-0.1f);glRotatef(-90,1,0,0);
        gluCylinder(q,0.10f,0.10f,1.8f,6,1);glPopMatrix();
        glTranslatef(0,-1.8f,0);
        int ci=g%NG;
        /* cabin body */
        //glColor3f(gc[ci][0],gc[ci][1],gc[ci][2]);
        //glPushMatrix();glScalef(2.2f,1.8f,1.5f);glutSolidCube(1);glPopMatrix();

        /* ===== REALISTIC CABIN ===== */
        


        /* cabin body (rounded box feel) */
        glColor3f(gc[ci][0]*0.9f,gc[ci][1]*0.9f,gc[ci][2]*0.9f);
        glPushMatrix();
        glScalef(1.8f,1.6f,1.2f);
        glutSolidCube(1);
        glPopMatrix();

        /* glass front */
        glColor4f(0.6f,0.9f,1.0f,0.6f);
        glPushMatrix();
        glTranslatef(0,0.0f,0.75f);
        glScalef(1.2f,1.2f,0.05f);
        glutSolidCube(1);
        glPopMatrix();

        /* glass back */
        glPushMatrix();
        glTranslatef(0,0.0f,-0.75f);
        glScalef(1.2f,1.2f,0.05f);
        glutSolidCube(1);
        glPopMatrix();

        /* door frame */
        glColor3f(0.2f,0.2f,0.25f);
        glPushMatrix();
        glTranslatef(0.5f,0.0f,0.78f);
        glScalef(0.5f,1.2f,0.05f);
        glutSolidCube(1);
        glPopMatrix();

        /* roof cap */
        glColor3f(gc[ci][0]*0.6f,gc[ci][1]*0.6f,gc[ci][2]*0.6f);
        glPushMatrix();
        glTranslatef(0,1.0f,0);
        glScalef(1.6f,0.2f,1.2f);
        glutSolidCube(1);
        glPopMatrix();

        /* bottom base */
        glColor3f(0.3f,0.3f,0.35f);
        glPushMatrix();
        glTranslatef(0,-1.0f,0);
        glScalef(1.6f,0.2f,1.2f);
        glutSolidCube(1);
        glPopMatrix();
        /* roof */
        glColor3f(gc[ci][0]*0.62f,gc[ci][1]*0.62f,gc[ci][2]*0.62f);
        glPushMatrix();glTranslatef(0,1.0f,0);glScalef(2.4f,0.30f,1.65f);glutSolidCube(1);glPopMatrix();
        /* windows */
        glColor3f(0.70f,0.92f,1.0f);
        glPushMatrix();glTranslatef(0,0.08f,0.78f);glScalef(1.65f,0.72f,0.06f);glutSolidCube(1);glPopMatrix();
        glPushMatrix();glTranslatef(0,0.08f,-0.78f);glScalef(1.65f,0.72f,0.06f);glutSolidCube(1);glPopMatrix();
        /* window frame */
        glColor3f(0.25f,0.25f,0.30f);
        glPushMatrix();glTranslatef(0,0.08f,0.80f);glScalef(1.85f,0.80f,0.04f);glutSolidCube(1);glPopMatrix();
        /* interior glow */
        glDisable(GL_LIGHTING);
        float hue=ledTime+g*0.35f;
        glColor3f(0.6f+0.4f*sinf(hue)*.5f,0.6f+0.4f*sinf(hue+2.1f)*.5f,1.0f);
        glPushMatrix();glTranslatef(0,0.08f,0.79f);glutSolidSphere(0.12f,5,5);glPopMatrix();
        glEnable(GL_LIGHTING);
        /* door */
        glColor3f(gc[ci][0]*0.75f,gc[ci][1]*0.75f,gc[ci][2]*0.75f);
        glPushMatrix();glTranslatef(0.60f,-.05f,.77f);glScalef(0.82f,1.4f,.07f);glutSolidCube(1);glPopMatrix();
        glPopMatrix();
    }
    glPopMatrix(); /* end rotating group */
    gluDeleteQuadric(q);glPopMatrix();
}
void updateTrain() {
    if (trainRunning) {
        trainAngle += trainSpeed * 0.02f;

        if (trainAngle > 2 * 3.14159f)
            trainAngle -= 2 * 3.14159f;

         wheelRotation += trainSpeed * 180 / 3.14159f * 3;
    }
   
}

/* ─────────────────────────────
   ROLLER COASTER
───────────────────────────── */
void getTrackPt(float t,float&ox,float&oy,float&oz){
    t=fmodf(t,1);if(t<0)t+=1;
    float a=t*2*(float)M_PI;
    float d=1+sinf(a)*sinf(a);
    ox=20*cosf(a)/d; oz=20*sinf(a)*cosf(a)/d;
    float hb=9+8*(sinf(a)+1)*.5f;
    oy=std::max(3.0f,hb+6*sinf(a*2));
}
void drawRollerCoaster(float cx,float cz){
    glPushMatrix();glTranslatef(cx,0,cz);
    GLUquadric*q=gluNewQuadric();
    const int SEG=360; const float RG=2.4f;
    glColor3f(.32f,.32f,.36f);
    for(float tt=0;tt<1;tt+=.06f){
        float tx,ty,tz;getTrackPt(tt,tx,ty,tz);
        glPushMatrix();glTranslatef(tx,0,tz);glRotatef(-90,1,0,0);
        gluCylinder(q,.42,.52,ty,12,1);glPopMatrix();
    }
    for(int seg=0;seg<SEG;seg++){
        float t0=(float)seg/SEG,t1=(float)(seg+1)/SEG;
        float x0,y0,z0,x1,y1,z1;
        getTrackPt(t0,x0,y0,z0);getTrackPt(t1,x1,y1,z1);
        float dx=x1-x0,dy=y1-y0,dz2=z1-z0;
        float tl=sqrtf(dx*dx+dy*dy+dz2*dz2);
        if(tl<.001f)continue;
        dx/=tl;dy/=tl;dz2/=tl;
        float px=-dz2,pz=dx;
        float pl=sqrtf(px*px+pz*pz);if(pl>.01f){px/=pl;pz/=pl;}
        if(seg%8==0){
            glColor3f(.75f,.10f,.10f);
            glBegin(GL_QUADS);
            glVertex3f(x0-px*RG/2,y0+.18f,z0-pz*RG/2);
            glVertex3f(x0+px*RG/2,y0+.18f,z0+pz*RG/2);
            glVertex3f(x1+px*RG/2,y1+.18f,z1+pz*RG/2);
            glVertex3f(x1-px*RG/2,y1+.18f,z1-pz*RG/2);
            glEnd();
        }
        float yaw=atan2f(dz2,dx)*180/(float)M_PI;
        float pit=atan2f(dy,sqrtf(dx*dx+dz2*dz2))*180/(float)M_PI;
        glColor3f(.88f,.14f,.14f);
        glPushMatrix();glTranslatef(x0-px*RG/2,y0,z0-pz*RG/2);
        glRotatef(yaw,0,1,0);glRotatef(-pit,0,0,1);gluCylinder(q,.28,.28,tl,8,1);glPopMatrix();
        glPushMatrix();glTranslatef(x0+px*RG/2,y0,z0+pz*RG/2);
        glRotatef(yaw,0,1,0);glRotatef(-pit,0,0,1);gluCylinder(q,.28,.28,tl,8,1);glPopMatrix();
    }
    float carC[][3]={{.18f,.58f,.96f},{.96f,.28f,.18f},{.18f,.88f,.28f},{.96f,.82f,.10f}};
    for(int ci=0;ci<4;ci++){
        float ct=fmodf(coasterProg/360+ci*2.2f/360,1);
        float cx2,cy,cz2;getTrackPt(ct,cx2,cy,cz2);
        float nx,ny,nz;getTrackPt(fmodf(ct+.004f,1),nx,ny,nz);
        float dx=nx-cx2,dy=ny-cy,dz2=nz-cz2;
        glPushMatrix();glTranslatef(cx2,cy,cz2);
        if(sqrtf(dx*dx+dy*dy+dz2*dz2)>.001f){
            glRotatef(atan2f(dz2,dx)*180/(float)M_PI,0,1,0);
            glRotatef(atan2f(dy,sqrtf(dx*dx+dz2*dz2))*180/(float)M_PI,0,0,1);
        }
        glColor3f(carC[ci][0],carC[ci][1],carC[ci][2]);
        glPushMatrix();glScalef(1.9f,.95f,1.1f);glutSolidCube(1);glPopMatrix();
        glColor3f(.68f,.90f,1.0f);
        glPushMatrix();glTranslatef(-.65f,.22f,.58f);glScalef(.65f,.52f,.08f);glutSolidCube(1);glPopMatrix();
        glColor3f(.10f,.10f,.10f);
        for(int w=0;w<2;w++){
            glPushMatrix();glTranslatef(-.40f,-.38f,(w==0)?-.62f:.62f);glutSolidSphere(.38f,10,10);glPopMatrix();
            glPushMatrix();glTranslatef(.40f,-.38f,(w==0)?-.62f:.62f);glutSolidSphere(.38f,10,10);glPopMatrix();
        }
        glPopMatrix();
    }
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   CAR
───────────────────────────── */
void drawCar(float x,float z,float face,float r,float g,float b){
    glPushMatrix();glTranslatef(x,0,z);glRotatef(face,0,1,0);
    glColor3f(r,g,b);
    glPushMatrix();glTranslatef(0,.62f,0);glScalef(2.1f,.75f,4.0f);glutSolidCube(1);glPopMatrix();
    glColor3f(r*.72f,g*.72f,b*.72f);
    glPushMatrix();glTranslatef(0,1.18f,.1f);glScalef(1.65f,.58f,2.3f);glutSolidCube(1);glPopMatrix();
    glColor3f(.68f,.88f,.96f);
    glPushMatrix();glTranslatef(0,1.08f,1.28f);glScalef(1.45f,.48f,.06f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0,1.08f,-1.18f);glScalef(1.45f,.48f,.06f);glutSolidCube(1);glPopMatrix();
    glColor3f(1.0f,.96f,.78f);
    glPushMatrix();glTranslatef(-.58f,.58f,2.02f);glutSolidSphere(.20f,8,8);glPopMatrix();
    glPushMatrix();glTranslatef(.58f,.58f,2.02f);glutSolidSphere(.20f,8,8);glPopMatrix();
    GLUquadric*q=gluNewQuadric();
    float wx[]={-1.08f,1.08f,-1.08f,1.08f},wz[]={1.25f,1.25f,-1.25f,-1.25f};
    for(int w=0;w<4;w++){
        glColor3f(.14f,.14f,.14f);
        glPushMatrix();glTranslatef(wx[w],.34f,wz[w]);glRotatef(90,0,1,0);
        gluCylinder(q,.34f,.34f,.24f,14,1);glPopMatrix();
        glColor3f(.62f,.62f,.68f);
        glPushMatrix();glTranslatef(wx[w],.34f,wz[w]);
        glRotatef(w<2?90:-90,0,1,0);gluDisk(q,0,.30f,12,1);glPopMatrix();
    }
    gluDeleteQuadric(q);glPopMatrix();
}

void drawParkingLot(){

    float s = 0.8f;   // 🔥 slight scale down

    // target position
    float cx = 45.0f;
    float cz = -45.0f;

    // original center ~ (49,49)
    float offX = cx - 49.0f * s;
    float offZ = cz - 49.0f * s;

    glPushMatrix();
    glTranslatef(offX, 0.0f, offZ);
    glScalef(s, 1.0f, s);

    glDisable(GL_LIGHTING);

    // =========================
    // BASE (raised to avoid Z-fighting)
    // =========================
    glColor3f(.18f,.18f,.18f);
    glBegin(GL_QUADS);
    glVertex3f(30,0.05f,30);   // 🔥 was 0.02 → now 0.05
    glVertex3f(68,0.05f,30);
    glVertex3f(68,0.05f,68);
    glVertex3f(30,0.05f,68);
    glEnd();

    // =========================
    // LINES
    // =========================
    glColor3f(.90f,.90f,.90f);

    for(int b=0;b<=6;b++){
        float bx=33+b*5.5f;
        glBegin(GL_QUADS);
        glVertex3f(bx,0.06f,34);
        glVertex3f(bx+.12f,0.06f,34);
        glVertex3f(bx+.12f,0.06f,43);
        glVertex3f(bx,0.06f,43);
        glEnd();
    }

    glBegin(GL_QUADS);
    glVertex3f(33,0.06f,34);
    glVertex3f(66,0.06f,34);
    glVertex3f(66,0.06f,34.12f);
    glVertex3f(33,0.06f,34.12f);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(33,0.06f,43);
    glVertex3f(66,0.06f,43);
    glVertex3f(66,0.06f,43.12f);
    glVertex3f(33,0.06f,43.12f);
    glEnd();

    for(int b=0;b<=6;b++){
        float bx=33+b*5.5f;
        glBegin(GL_QUADS);
        glVertex3f(bx,0.06f,51);
        glVertex3f(bx+.12f,0.06f,51);
        glVertex3f(bx+.12f,0.06f,60);
        glVertex3f(bx,0.06f,60);
        glEnd();
    }

    glBegin(GL_QUADS);
    glVertex3f(33,0.06f,51);
    glVertex3f(66,0.06f,51);
    glVertex3f(66,0.06f,51.12f);
    glVertex3f(33,0.06f,51.12f);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(33,0.06f,60);
    glVertex3f(66,0.06f,60);
    glVertex3f(66,0.06f,60.12f);
    glVertex3f(33,0.06f,60.12f);
    glEnd();

    glEnable(GL_LIGHTING);

    // =========================
    // CARS
    // =========================
    float cr1[][3]={{.88f,.10f,.10f},{.14f,.34f,.82f},{.12f,.62f,.18f},
                    {.88f,.75f,.10f},{.62f,.16f,.65f},{.82f,.82f,.82f}};

    for(int c=0;c<6;c++)
        drawCar(35.5f+c*5.5f,37.8f,0,cr1[c][0],cr1[c][1],cr1[c][2]);

    float cr2[][3]={{.92f,.48f,.10f},{.28f,.28f,.28f},
                    {.10f,.56f,.76f},{.76f,.22f,.22f}};

    for(int c=0;c<4;c++)
        drawCar(35.5f+c*5.5f,54.8f,180,cr2[c][0],cr2[c][1],cr2[c][2]);

    // =========================
    // LAMPS
    // =========================
    drawLampPost(35,46.5f);
    drawLampPost(50,46.5f);
    drawLampPost(64.5f,46.5f);

    glPopMatrix();
}

void drawPirateShip(float cx, float cz) {
    glPushMatrix();
    glTranslatef(cx, 0.0f, cz);

    GLUquadric* q = gluNewQuadric();

    extern float pirateAngle;
    float swing = sinf(pirateAngle * 0.03f) * 40.0f;

    // 🔥 NEW PARAMETERS
    float poleOffset = 8.5f;   // wider spacing
    float poleHeight = 11.5f;  // match axle height

    // ======================
    // PLATFORM (INCREASED SIZE)
    // ======================
    glColor3f(0.65f, 0.62f, 0.55f);
    glPushMatrix();
    glTranslatef(0, 0.12f, 0);
    glScalef(18.0f, 0.24f, 6.5f);  // 🔥 bigger
    glutSolidCube(1);
    glPopMatrix();

    // ======================
    // SUPPORT FRAME (WIDER + TALLER)
    // ======================
    glColor3f(0.35f, 0.35f, 0.40f);
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(s * poleOffset, 0, 0);   // 🔥 updated
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.30f, 0.24f, poleHeight, 12, 1); // 🔥 taller
        glPopMatrix();
    }

    // ======================
    // PIVOT ROD (MATCH WIDTH)
    // ======================
    glColor3f(0.6f, 0.6f, 0.65f);
    glPushMatrix();
    glTranslatef(-poleOffset, poleHeight, 0);   // 🔥 aligned
    glRotatef(90, 0, 1, 0);
    gluCylinder(q, 0.28f, 0.28f, poleOffset * 2, 12, 1); // 🔥 full span
    glPopMatrix();

    // ======================
    // PIVOT JOINTS (MATCH)
    // ======================
    glPushMatrix();
    glTranslatef(-poleOffset, poleHeight, 0);
    glutSolidSphere(0.4, 20, 20);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(poleOffset, poleHeight, 0);
    glutSolidSphere(0.4, 20, 20);
    glPopMatrix();

    // ======================
    // SWINGING SHIP
    // ======================
    glPushMatrix();
    glTranslatef(0, poleHeight, 0);   // 🔥 use same height
    glRotatef(swing, 0, 0, 1);

    // support arm
    glColor3f(0.3f, 0.3f, 0.35f);
    glPushMatrix();
    glTranslatef(0, -4.0f, 0);
    glScalef(0.4f, 8.0f, 0.4f);
    glutSolidCube(1);
    glPopMatrix();

    // move to hull
    glTranslatef(0, -8.0f, 0);

    // ======================
    // HULL
    // ======================
    glColor3f(0.45f, 0.25f, 0.10f);
    glPushMatrix();
    glTranslatef(0, -0.6f, 0);
    glScalef(5.0f, 0.8f, 1.4f);
    glutSolidSphere(1.0f, 30, 20);
    glPopMatrix();

    glColor3f(0.35f, 0.18f, 0.08f);
    glPushMatrix();
    glTranslatef(0, -0.4f, 0);
    glScalef(5.2f, 0.6f, 1.45f);
    glutSolidCube(1);
    glPopMatrix();

    // ======================
    // SIDE WALLS
    // ======================
    glColor3f(0.5f, 0.28f, 0.10f);
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(0, 0.6f, s * 0.9f);
        glScalef(5.4f, 1.3f, 0.2f);
        glutSolidCube(1);
        glPopMatrix();
    }

    // ======================
    // SEATS
    // ======================
    glColor3f(0.9f, 0.8f, 0.3f);

    float spacing = 1.4f;

    for (int i = -2; i <= 2; i++) {
        float x = i * spacing;

        glPushMatrix();
        glTranslatef(x, 0.1f, 0);
        glScalef(0.7f, 0.2f, 1.1f);
        glutSolidCube(1);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(x, 0.25f, -0.5f);
        glScalef(0.7f, 0.5f, 0.2f);
        glutSolidCube(1);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(x, 0.25f, 0.5f);
        glScalef(0.7f, 0.5f, 0.2f);
        glutSolidCube(1);
        glPopMatrix();
    }

    // ======================
    // FLAG
    // ======================
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.06f, 0.05f, 3.2f, 6, 1);
    glPopMatrix();

    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(0.9f, 3.7f, 0);
    glRotatef(10, 0, 1, 0);
    glScalef(1.6f, 1.0f, 0.05f);
    glutSolidCube(1);
    glPopMatrix();

    glPopMatrix(); // swinging ship

    gluDeleteQuadric(q);
    glPopMatrix();
}
/* ─────────────────────────────
   PERSON
───────────────────────────── */
void drawPerson(float x,float z,float face,int type,
                float sr,float sg,float sb,int action,float ph){
    float sc=(type==1)?.65f:1.0f;
    float ls=0,as=0;
    if(action==2){ls=sinf(ph)*26;as=-sinf(ph)*22;}
    if(action==3) as=-48+sinf(ph)*16;
    glPushMatrix();glTranslatef(x,0,z);glRotatef(face,0,1,0);glScalef(sc,sc,sc);
    GLUquadric*q=gluNewQuadric();
    for(int s=-1;s<=1;s+=2){
        glColor3f(.22f,.22f,.52f);
        glPushMatrix();glTranslatef(s*.19f,.92f,0);
        glRotatef(action==1?-68:s*ls,1,0,0);glRotatef(-90,1,0,0);
        gluCylinder(q,.105,.09,.92f,7,1);glPopMatrix();
        glColor3f(.16f,.12f,.08f);
        glPushMatrix();glTranslatef(s*.19f,action==1?.40f:.06f,action==1?-.65f:.78f);
        glutSolidSphere(.14f,6,6);glPopMatrix();
    }
    glColor3f(sr,sg,sb);
    float ty=action==1?1.58f:1.32f;
    glPushMatrix();glTranslatef(0,ty,0);glScalef(.44f,.72f,.32f);glutSolidSphere(1,10,10);glPopMatrix();
    float sy=action==1?1.98f:1.78f;
    for(int s=-1;s<=1;s+=2){
        glColor3f(sr,sg,sb);
        glPushMatrix();glTranslatef(s*.37f,sy,0);
        glRotatef(action==1?-12:s*as,1,0,0);glRotatef(-90,1,0,0);
        gluCylinder(q,.085,.075,.68f,7,1);glPopMatrix();
        glColor3f(.86f,.70f,.54f);
        float aay=sy-.58f,aaz=action==1?-.25f:.60f;
        glPushMatrix();glTranslatef(s*.37f,aay,aaz);glutSolidSphere(.11f,6,6);glPopMatrix();
    }
    float ny=action==1?2.28f:2.12f;
    glColor3f(.86f,.70f,.54f);
    glPushMatrix();glTranslatef(0,ny,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.09,.09,.24f,6,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,ny+.40f,0);
    glColor3f(.86f,.70f,.54f);glutSolidSphere(.27f,12,12);
    glColor3f(.20f+sr*.08f,.14f,.06f);
    glPushMatrix();glTranslatef(0,.20f,0);glScalef(1,.48f,1);glutSolidSphere(.28f,8,8);glPopMatrix();
    glColor3f(.08f,.06f,.06f);
    glPushMatrix();glTranslatef(-.10f,.06f,.24f);glutSolidSphere(.042f,5,5);glPopMatrix();
    glPushMatrix();glTranslatef(.10f,.06f,.24f);glutSolidSphere(.042f,5,5);glPopMatrix();
    glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   PEOPLE SCENE
───────────────────────────── */
void drawPeopleScene(){
    drawPerson(-17.5f,-5.5f,85,0,.85f,.28f,.58f,1,0);
    drawPerson(-15.8f,-5.8f,100,0,.20f,.34f,.76f,0,0);
    float k1a=walkTime*1.8f;
   // drawPerson(-8+8.9f*cosf(k1a),-9+8.9f*sinf(k1a),90-k1a*180/(float)M_PI,1,1.0f,.52f,.10f,2,walkTime*4);
    drawPerson(24,-14.5f,200,1,.90f,.26f,.26f,3,walkTime*3);
    drawPerson(26,-13.8f,215,0,.58f,.18f,.18f,0,0);
    float k3x=-12.5f+sinf(walkTime*1.2f)*.4f;
    drawPerson(k3x,-2.5f,185,1,.88f,.88f,.16f,2,walkTime*3.5f);
    drawPerson(k3x+1.5f,-2.5f,175,1,.26f,.68f,.92f,2,walkTime*3.5f+1);
    float cz2=fmodf(walkTime*3.5f,60)-40;
    float stopZ = -23.0f;
    if(cz2 > stopZ) cz2 = stopZ;  // 🔥 stop at -23
    drawPerson(-1.5f,cz2,0,0,.36f,.20f,.62f,2,walkTime*3);
    drawPerson(1.5f,cz2,0,0,.86f,.46f,.20f,2,walkTime*3+.5f);
    drawPerson(31,-8.5f,185,0,.50f,.26f,.68f,0,0);
    /* Food court people */
    drawPerson(-42,-36,45,0,.72f,.28f,.18f,1,0);
    drawPerson(-38,-34,90,1,.28f,.62f,.88f,1,0);
    drawPerson(-50,-28,135,0,.88f,.58f,.18f,0,0);
    /* Game zone people */
    float gw=fmodf(walkTime*2,8)-4;
    drawPerson(-38+gw,28,0,0,.62f,.22f,.72f,2,walkTime*3.5f);
    drawPerson(-36,32,90,1,.28f,.82f,.28f,2,walkTime*4);
    
}

/* ─────────────────────────────
   DISPLAY
───────────────────────────── */
void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    float rx=angleX*(float)M_PI/180, ry2=angleY*(float)M_PI/180;
    float ex=zoomDist*cosf(rx)*sinf(ry2);
    float ey=zoomDist*sinf(rx);
    float ez=zoomDist*cosf(rx)*cosf(ry2);
    gluLookAt(ex,ey,ez,0,0,0,0,1,0);
    GLfloat lp0[]={80,150,100,0};
    glLightfv(GL_LIGHT0,GL_POSITION,lp0);

    drawSky();
    drawHills();
    drawClouds();
    drawGround();

    pizzaTex = createTexture("pizza.bmp");
    burgerTex = createTexture("burger.bmp");   // converted PNG
    iceTex = createTexture("icecream.bmp");
    waterTex = createTexture("water.bmp");

    /* ══ BOUNDARY WALL ══ */
    drawBoundaryWall();

    /* ══════════════════════════════════════
       COORDINATE SYSTEM:
         Z = -65  South boundary  (Entrance / Gate)
         Z =   0  Center plaza   (Grand Fountain + Roundabout)
         Z = +65  North boundary
         X = -65  West           (Food Court + Game Zone)
         X = +65  East           (Ride Zone)
       Main boulevard runs S→N along X≈0
    ══════════════════════════════════════ */

    /* ═════════════════════════════
   CLEAN ROAD SYSTEM (YELLOW LAYOUT ONLY)
   ═════════════════════════════ */

/* Main entry road */
drawRoadSegment(0, -65, 0, -22, 5);

/* Cross roads */
drawRoadSegment(-60, -22, 60, -22, 4);
//drawRoadSegment(  8, -22, 62, -22, 4);

/* Upper roads remain unchanged */
drawRoadSegment(-60,  22, 60,  22, 4);
//drawRoadSegment(  8,  22, 62,  22, 4);

/* Side roads */
drawRoadSegment(27, -62, 27, 62, 4);
drawRoadSegment(-27, -62, -27, 24, 4);

/* Roundabout stays */
//drawRoundabout(0, 0, 9.5f, 15.0f);

drawTrainTrack();  // optional
drawTrain();

    /* ── DECORATIVE WALKWAYS (pedestrian paths) ── */
    // drawPathway(-8,-62,  8,-62, 10.0f); /* Entry plaza at gate */
    // drawPathway(-8,-65,  8,-55, 11.0f); /* Gate approach */

    /* ── PERIMETER TREES (inside wall) ── */
float T = 61;

for(int i = -5; i <= 5; i++) {

    // LEFT & RIGHT (always)
    drawTree(-T, i * 11);
    drawTree( T, i * 11);

    // NORTH SIDE (skip here instead)
    if(!(i >= -1 && i <= 1)) {
        drawTree(i * 11, -T);   // 🔥 moved condition here
    }

    // SOUTH SIDE (always draw)
    drawTree(i * 11, T);
}
    /* Boulevard trees flanking main N-S path */
    for(int i=0;i<10;i++){
        float tz=-62+i*10.5f;
        if(tz<-30 && tz>-50){ /* skip roundabout area */
            drawTree(-8.5f,tz,1.1f);
            drawTree( 8.5f,tz,1.1f);
        }
    }
float treeRadius = 18.0f;   // radius for circular tree arrangement at roundabout corners

for(int i=0;i<8;i++){
    float ta = i * 45 * (float)M_PI/180 + 22.5f * (float)M_PI/180;
    drawTree(cosf(ta) * treeRadius, sinf(ta) * treeRadius, 0.95f);
}
    /* Interior zone dividers */
    for(int i=-2;i<=2;i++){
        drawTree(-24,i*10,0.85f);
        drawTree( 24,i*10,0.85f);
    }
   

    /* ── FUNLAND ENTRANCE GATE ── */
    drawFunlandGate(0,-65);

    /*  / ticket zone near gate */
    drawTicketBooth( 16,-56,180);
    drawTicketBooth(-16,-56,180);
    drawTicketBooth(36,30,270);//GiantWheel ticket booth
    drawTicketBooth(36,-10,270);//merry go round ticket booth
    drawTicketBooth(-50,30,180);//merry go round ticket booth
    drawTicketBooth(-20,-15,180);//merry go round ticket booth

    drawLampPost(-14,-58);drawLampPost( 14,-58);
    drawLampPost( -8,-47);drawLampPost(  8,-47);
    drawBench(-18,-50,90);drawBench( 18,-50,90);
    drawTrashBin(-12,-46,0.22f,0.62f,0.18f);
    drawTrashBin( 12,-46,0.22f,0.62f,0.18f);

    /* ── GRAND FOUNTAIN + ROUNDABOUT CENTERPIECE ── */
    drawFountain(0, 0);
    /* Benches around fountain (on roundabout island) */
    for(int b=0;b<8;b++){
        float ba=b*45*(float)M_PI/180;
        float br=17.5f; /* outside roundabout */
        drawBench(cosf(ba)*br,sinf(ba)*br, b*45.0f+90);
    }
    /* Lamp posts at roundabout entries */
    for(int l=0;l<4;l++){
        float la=l*90*(float)M_PI/180;
        drawLampPost(cosf(la)*20.5f,sinf(la)*20.5f);
    }
    drawInfoKiosk( 13,-17,180);
    drawInfoKiosk(-13,-17,0);
    drawTrashBin( 11,  5,0.68f,0.68f,0.68f);
    drawTrashBin(-11,  5,0.68f,0.68f,0.68f);

    /* ── STAGE / EVENT AREA (north) ── */
    drawCircusTent(0, 42);
    drawLampPost(-14,28);drawLampPost(14,28);
    drawLampPost(-14,36);drawLampPost(14,36);
    drawBench(-16,24,0);drawBench(18,24,0);
    drawBench(-16,32,0);drawBench(18,32,0);
    drawTrashBin(-12,26,0.62f,0.18f,0.88f);drawTrashBin(14,26,0.62f,0.18f,0.88f);
    drawBalloonStall(24,28,270);
    drawBalloonStall(-22,28,90);

    /* ── RIDE ZONE (east, X=30-62) ── */
    drawGiantWheel(46,42);
    drawMerryGoRound(46,0,merryAngle);
    drawPirateShip(-45,10);
    drawLampPost(30,-28);drawLampPost(42,-15);drawLampPost(57,-15);drawLampPost(57,-42);
    drawBench(34,-20,90);drawBench(57,-28,0);drawBench(46,-15,0);
    drawTrashBin(36,-18,0.28f,0.48f,0.78f);drawTrashBin(57,-38,0.28f,0.48f,0.78f);

    /* ── ROLLER COASTER (NW, away from roads) ── */
    drawRollerCoaster(-40,42);


/* ═══ FOOD COURT (SW quadrant) ═══
       3 stalls side by side, vendor behind each, big tables in front */
    /* Food court paved floor */
    //glDisable(GL_LIGHTING);
    // glColor3f(0.76f,0.72f,0.62f);
    // glBegin(GL_QUADS);
    // glVertex3f(-62,0.01f,-62);glVertex3f(-20,0.01f,-62);
    // glVertex3f(-20,0.01f,-10);glVertex3f(-62,0.01f,-10);
   // glEnd();

   glEnable(GL_LIGHTING);


 /* 3 food stalls side by side */
    drawPizzaStall(-53,-54, 90);    /* pizza — leftmost, facing east (+X) */
    drawBurgerStall(-43,-54, 90);   /* burger — middle */
    drawIceCreamStall(-33,-54, 90); /* ice cream — rightmost */

    /* big dining tables in front of stalls */
    float dtc[][3]={{0.92f,0.22f,0.22f},{0.22f,0.52f,0.92f},{0.22f,0.78f,0.28f},
                    {0.92f,0.72f,0.10f},{0.78f,0.18f,0.82f},{0.92f,0.42f,0.18f},
                    {0.18f,0.72f,0.82f},{0.92f,0.28f,0.48f},{0.58f,0.82f,0.22f},
                    {0.88f,0.58f,0.10f},{0.28f,0.48f,0.88f},{0.88f,0.88f,0.18f}};
    for(int r=0;r<3;r++) for(int c=0;c<4;c++)
        drawBigDiningTable(-54+c*7.0f,-42+r*7.0f,dtc[r*4+c][0],dtc[r*4+c][1],dtc[r*4+c][2]);

    drawLampPost(-54,-52);drawLampPost(-38,-52);drawLampPost(-24,-52);
    drawLampPost(-54,-18);drawLampPost(-38,-18);drawLampPost(-24,-18);
    drawTrashBin(-58,-30,0.88f,0.48f,0.10f);drawTrashBin(-22,-30,0.88f,0.48f,0.10f);
    drawTrashBin(-58,-48,0.88f,0.48f,0.10f);drawTrashBin(-22,-48,0.88f,0.48f,0.10f);




    /* ── GAME STALLS (NW, X=-62..-22, Z=+10..+62) ── */
    drawRingTossStall  (-44,28,  0);
    drawShootingGallery(-52,40, 10);
    drawBalloonStall   (-36,18,180);
    drawBalloonStall   (-48,18, 90);
    drawLampPost(-32,20);drawLampPost(-57,22);drawLampPost(-48,50);
    drawBench(-32,24,0);drawBench(-52,50,90);
    drawTrashBin(-34,46,0.18f,0.68f,0.28f);drawTrashBin(-57,46,0.18f,0.68f,0.28f);

    /* ── BALLOON STALL near fountain plaza ── */
    drawBalloonStall( 30,-10, 0);
    drawBalloonStall(  0, 24, 90);

    /* ── FLOATING BALLOONS ── */
    float bC[][3]={{1,.10f,.10f},{.10f,.40f,1},{.92f,.86f,.05f},{.10f,.76f,.20f},{.86f,.20f,.86f}};
    float bP[][2]={{20,-20},{-25,25},{10,40},{-35,-10},{35,10}};
    for(int b=0;b<5;b++){
        float sw=sinf(windTime+b*1.3f)*.32f;
        drawBalloon(bP[b][0],9,bP[b][1],bC[b][0],bC[b][1],bC[b][2],sw);
    }
    drawBalloon(-45,6,-30,1,0.5f,0.1f,sinf(windTime)*.25f);
    drawBalloon(-38,6,-28,0.2f,0.8f,1,sinf(windTime+1)*.2f);
    drawBalloon(10,8,35,1,0.9f,0.1f,sinf(windTime+2)*.3f);

    /* ── PARKING LOT ── */
    drawParkingLot();

    drawPeopleScene();
    waterTex=createTexture("water.bmp");

    glutSwapBuffers();
}

/* ─────────────────────────────
   TIMER
───────────────────────────── */
void timerFunc(int){
    if(wheelSpinning){wheelAngle+=.50f;if(wheelAngle>=360)wheelAngle-=360;}
    if(coasterRunning){coasterProg+=1.5f;if(coasterProg>=360)coasterProg-=360;}
    if(merryRunning){merryAngle+=1.2f;if(merryAngle>=360)merryAngle-=360;}
    windTime+=.038f;walkTime+=.020f;fountainTime+=.06f;
    spotAngle+=0.5f;if(spotAngle>=360)spotAngle-=360;
    glutPostRedisplay();
    glutTimerFunc(16,timerFunc,0);
}

/* ─────────────────────────────
   INPUT
───────────────────────────── */
void reshape(int w,int h){
    if(!h)h=1;glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    gluPerspective(52,(float)w/h,5.0f,200.0f);
    glMatrixMode(GL_MODELVIEW);
}
void keyboard(unsigned char k,int,int){
    if(k=='+'||k=='=')zoomDist-=5;if(k=='-')zoomDist+=5;
    zoomDist=std::max(15.f,std::min(400.f,zoomDist));
    if(k=='w'||k=='W')wheelSpinning=!wheelSpinning;
    if(k=='r'||k=='R')coasterRunning=!coasterRunning;
    if(k=='c'||k=='C')merryRunning=!merryRunning;
    if(k=='t'||k=='T')trainRunning=!trainRunning;
    if(k=='p'||k=='P')pirateMoving=!pirateMoving;
    glutPostRedisplay();
}
void specialKeys(int k,int,int){
    if(k==GLUT_KEY_LEFT)angleY-=3;if(k==GLUT_KEY_RIGHT)angleY+=3;
    if(k==GLUT_KEY_UP)angleX=std::min(angleX+2,85.f);
    if(k==GLUT_KEY_DOWN)angleX=std::max(angleX-2,5.f);
    glutPostRedisplay();
}
void mouseBtn(int btn,int state,int x,int y){
    if(btn==GLUT_LEFT_BUTTON){mouseDown=(state==GLUT_DOWN);lastMX=x;lastMY=y;}
    if(btn==3){zoomDist-=3;glutPostRedisplay();}
    if(btn==4){zoomDist+=3;glutPostRedisplay();}
}
void mouseMove(int x,int y){
    if(!mouseDown)return;
    angleY+=(x-lastMX)*.4f;
    angleX=std::max(5.f,std::min(85.f,angleX-(y-lastMY)*.35f));
    lastMX=x;lastMY=y;glutPostRedisplay();
}
void updatePirateShip() {
    if (pirateMoving) {
        pirateAngle += 1.2f;   // speed (adjust if needed)
    }
}
void idle() {
    updateTrain();  
    updatePirateShip();
    updateFountain();   // 🔥 ADD THIS
    glutPostRedisplay();
}
/* ─────────────────────────────
   INIT + MAIN
───────────────────────────── */
void init(){
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearColor(.38f,.65f,.95f,1);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_NORMALIZE);

    setupLighting();

    int tw, th;

    // ===== GRASS TEXTURE =====
    unsigned char* td = loadBMP("grass.bmp", &tw, &th);
    if(td){
        glGenTextures(1, &groundTex);
        glBindTexture(GL_TEXTURE_2D, groundTex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, tw, th, GL_RGB, GL_UNSIGNED_BYTE, td);

        delete[] td;
        printf("grass.bmp loaded %dx%d\n", tw, th);
    } else {
        printf("Running without grass texture\n");
    }

    // ===== WATER TEXTURE =====
    unsigned char* wd = loadBMP("water.bmp", &tw, &th);
    if(wd){
        glGenTextures(1, &waterTex);
        glBindTexture(GL_TEXTURE_2D, waterTex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, tw, th, GL_RGB, GL_UNSIGNED_BYTE, wd);

        delete[] wd;
        printf("water.bmp loaded %dx%d\n", tw, th);
    } else {
        printf("Running without water texture\n");
    }
}


int main(int argc,char**argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(1280,800);
    glutCreateWindow("FUNLAND 3D Amusement Park");
    init();
    glutIdleFunc(idle);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseBtn);
    glutMotionFunc(mouseMove);
    glutTimerFunc(16,timerFunc,0);
    glutMainLoop();
    return 0;
}