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
float spotAngle=0;

bool wheelSpinning=true, coasterRunning=true, merryRunning=true;

GLuint groundTex=0;

/* ─────────────────────────────
   BMP LOADER
───────────────────────────── */
unsigned char* loadBMP(const char* fn,int*w,int*h)
{
    FILE*f=fopen(fn,"rb");
    if(!f){printf("grass.bmp not found\n");return nullptr;}
    unsigned char hdr[54]; fread(hdr,1,54,f);
    *w=*(int*)&hdr[18]; *h=*(int*)&hdr[22];
    int row=(*w*3+3)&~3;
    unsigned char*raw=new unsigned char[row*(*h)];
    fread(raw,1,row*(*h),f); fclose(f);
    unsigned char*out=new unsigned char[(*w)*(*h)*3];
    for(int i=0;i<*h;i++)for(int j=0;j<*w;j++)for(int k=0;k<3;k++)
        out[(i*(*w)+j)*3+k]=raw[i*row+j*3+(2-k)];
    delete[]raw; return out;
}

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
    drawZoneFloor( 20,-62, 65,-10, 0.18f,0.48f,0.92f, 0.13f); /* Ride zone - blue */
    drawZoneFloor(-65,-62,-20,-10, 0.92f,0.58f,0.18f, 0.13f); /* Food court - orange */
    drawZoneFloor(-65, 10,-20, 62, 0.18f,0.72f,0.32f, 0.10f); /* Game stalls - green */
    drawZoneFloor(-20,-62, 20,-25, 0.58f,0.18f,0.88f, 0.10f); /* Stage area - purple */
    drawZoneFloor(-20,-20, 20, 20, 0.92f,0.88f,0.72f, 0.18f); /* Central plaza - warm */
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
    float nx=-dz/len, nz=dx/len; /* normal to road direction */
    float rh=roadW*0.5f;
    float sh=rh+1.0f; /* sidewalk half-width */

    /* sidewalk */
    glColor3f(0.76f,0.72f,0.66f);
    glBegin(GL_QUADS);
    glVertex3f(x1+nx*sh, 0.01f, z1+nz*sh);
    glVertex3f(x2+nx*sh, 0.01f, z2+nz*sh);
    glVertex3f(x2-nx*sh, 0.01f, z2-nz*sh);
    glVertex3f(x1-nx*sh, 0.01f, z1-nz*sh);
    glEnd();

    /* asphalt */
    glColor3f(0.22f,0.22f,0.24f);
    glBegin(GL_QUADS);
    glVertex3f(x1+nx*rh, 0.03f, z1+nz*rh);
    glVertex3f(x2+nx*rh, 0.03f, z2+nz*rh);
    glVertex3f(x2-nx*rh, 0.03f, z2-nz*rh);
    glVertex3f(x1-nx*rh, 0.03f, z1-nz*rh);
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
        glVertex3f(cx0+nx*0.12f,0.05f,cz0+nz*0.12f);
        glVertex3f(cx1+nx*0.12f,0.05f,cz1+nz*0.12f);
        glVertex3f(cx1-nx*0.12f,0.05f,cz1-nz*0.12f);
        glVertex3f(cx0-nx*0.12f,0.05f,cz0-nz*0.12f);
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
   PALM TREE
───────────────────────────── */
void drawPalmTree(float x,float z){
    glPushMatrix();glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.52f,0.34f,0.14f);
    for(int s=0;s<8;s++){
        float sy=(float)s/8,sy1=(float)(s+1)/8;
        float sx=sinf(sy*1.2f)*0.5f;
        glPushMatrix(); glTranslatef(sx,sy*7,0);
        glRotatef(-90+sy*8,1,0,0);
        gluCylinder(q,0.22f*(1-sy*0.3f),0.20f*(1-(sy+.125f)*0.3f),7.0f/8,8,1);
        glPopMatrix();
    }
    float trunkTop=7.2f;
    float tx=sinf(1.2f)*0.5f;
    glColor3f(0.14f,0.58f,0.18f);
    for(int f=0;f<7;f++){
        float fa=f*360.f/7*(float)M_PI/180;
        glPushMatrix(); glTranslatef(tx,trunkTop,0);
        glRotatef(f*360.f/7,0,1,0); glRotatef(-35,1,0,0);
        for(int s=0;s<8;s++){
            float t=(float)s/8;
            glPushMatrix();glTranslatef(0,0,t*3.5f);
            glScalef((1-t)*0.5f,(1-t)*0.3f,0.5f);
            glutSolidSphere(1,5,5); glPopMatrix();
        }
        glPopMatrix();
    }
    gluDeleteQuadric(q);glPopMatrix();
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
    glPushMatrix();glScalef(4.2f,1.1f,2.1f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(.80f,.70f,.52f);
    glPushMatrix();glTranslatef(0,1.16f,0);glScalef(4.4f,.12f,2.3f);glutSolidCube(1);glPopMatrix();
    float px[]={-1.9f,1.9f,-1.9f,1.9f},pzz[]={-.95f,-.95f,.95f,.95f};
    GLUquadric*q=gluNewQuadric();
    for(int i=0;i<4;i++){
        glColor3f(.78f,.10f,.10f);
        glPushMatrix();glTranslatef(px[i],1.15f,pzz[i]);glRotatef(-90,1,0,0);
        gluCylinder(q,.10,.10,3.8,8,1);glPopMatrix();
    }
    int S=10; float cw=4.0f/S,ry=5.1f;
    for(int s=0;s<S;s++){
        glColor3f(s%2==0?.92f:.98f,s%2==0?.10f:.98f,s%2==0?.10f:.98f);
        float xs=-2+s*cw;
        glBegin(GL_QUADS);
        glVertex3f(xs,ry,-1.15f);glVertex3f(xs+cw,ry,-1.15f);
        glVertex3f(xs+cw,ry-.55f,-1.7f);glVertex3f(xs,ry-.55f,-1.7f);glEnd();
        glBegin(GL_QUADS);
        glVertex3f(xs,ry,1.15f);glVertex3f(xs+cw,ry,1.15f);
        glVertex3f(xs+cw,ry-.55f,1.7f);glVertex3f(xs,ry-.55f,1.7f);glEnd();
        glBegin(GL_QUADS);
        glVertex3f(xs,ry,-1.15f);glVertex3f(xs+cw,ry,-1.15f);
        glVertex3f(xs+cw,ry,1.15f);glVertex3f(xs,ry,1.15f);glEnd();
    }
    float bc[][3]={{1,.18f,.18f},{.18f,.5f,1},{1,.88f,.08f},{.18f,.82f,.28f},{1,.42f,.05f},{.72f,.18f,.88f}};
    for(int b=0;b<6;b++){
        float bx=-1.6f+(b%3)*.58f,bz2=-.35f+(b/3)*.7f,by=1.15f+1.9f+(b%2)*.45f;
        float sw=sinf(windTime*1.2f+b)*.13f;
        drawBalloon(bx,by,bz2,bc[b][0],bc[b][1],bc[b][2],sw);
    }
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

    /* ── Pillar ground shadows (flat dark ellipses) ── */
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f,0.0f,0.0f,0.28f);
    for(int s=-1;s<=1;s+=2){
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(s*gap, 0.06f, 0);
        for(int i=0;i<=24;i++){
            float a=i*2*(float)M_PI/24;
            glVertex3f(s*gap+cosf(a)*2.2f, 0.06f, sinf(a)*1.2f);
        }
        glEnd();
    }
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);

    /* ── Arched beam (main horizontal) ── */
    glColor3f(0.90f,0.80f,0.55f);
    glPushMatrix();glTranslatef(0,ph+0.8f,0);glScalef(gap*2.0f,1.8f,2.0f);glutSolidCube(1);glPopMatrix();
    /* Beam underside darker for depth */
    glColor3f(0.70f,0.62f,0.38f);
    glPushMatrix();glTranslatef(0,ph+0.8f-0.82f,0);glScalef(gap*2.0f,0.12f,2.0f);glutSolidCube(1);glPopMatrix();

    /* ── Arch — thicker with front+back+side faces ── */
    float AR=gap+1.5f;
    float AT=0.72f;   /* thicker than before */
    float AZ=0.9f;    /* half depth of arch */
    int AS=28;
    float ac[][3]={{1,0,0},{1,.5f,0},{1,1,0},{0,.8f,0},{0,.5f,1},{.3f,0,1},{.8f,0,.8f}};
    for(int s=0;s<AS;s++){
        float a0=s*(float)M_PI/AS, a1=(s+1)*(float)M_PI/AS;
        glColor3f(ac[s%7][0],ac[s%7][1],ac[s%7][2]);
        /* Front face */
        glBegin(GL_QUADS);
        glVertex3f((AR-AT)*cosf(a0),ph+0.8f+(AR-AT)*sinf(a0),-AZ);
        glVertex3f(AR*cosf(a0),ph+0.8f+AR*sinf(a0),-AZ);
        glVertex3f(AR*cosf(a1),ph+0.8f+AR*sinf(a1),-AZ);
        glVertex3f((AR-AT)*cosf(a1),ph+0.8f+(AR-AT)*sinf(a1),-AZ);
        glEnd();
        /* Back face */
        glBegin(GL_QUADS);
        glVertex3f((AR-AT)*cosf(a0),ph+0.8f+(AR-AT)*sinf(a0),AZ);
        glVertex3f(AR*cosf(a0),ph+0.8f+AR*sinf(a0),AZ);
        glVertex3f(AR*cosf(a1),ph+0.8f+AR*sinf(a1),AZ);
        glVertex3f((AR-AT)*cosf(a1),ph+0.8f+(AR-AT)*sinf(a1),AZ);
        glEnd();
        /* Outer rim (connects front/back outer edge) */
        glColor3f(ac[s%7][0]*0.75f,ac[s%7][1]*0.75f,ac[s%7][2]*0.75f);
        glBegin(GL_QUADS);
        glVertex3f(AR*cosf(a0),ph+0.8f+AR*sinf(a0),-AZ);
        glVertex3f(AR*cosf(a0),ph+0.8f+AR*sinf(a0), AZ);
        glVertex3f(AR*cosf(a1),ph+0.8f+AR*sinf(a1), AZ);
        glVertex3f(AR*cosf(a1),ph+0.8f+AR*sinf(a1),-AZ);
        glEnd();
        /* Inner rim */
        glBegin(GL_QUADS);
        glVertex3f((AR-AT)*cosf(a0),ph+0.8f+(AR-AT)*sinf(a0),-AZ);
        glVertex3f((AR-AT)*cosf(a0),ph+0.8f+(AR-AT)*sinf(a0), AZ);
        glVertex3f((AR-AT)*cosf(a1),ph+0.8f+(AR-AT)*sinf(a1), AZ);
        glVertex3f((AR-AT)*cosf(a1),ph+0.8f+(AR-AT)*sinf(a1),-AZ);
        glEnd();
    }

    /* ── Hanging string lights between the two pillars ── */
    /* Two strands at slightly different heights */
    for(int strand=0;strand<2;strand++){
        float hy=ph*0.55f + strand*1.8f;  /* heights of the two strands */
        int NB=14;                          /* number of bulbs per strand */
        float bulbC[][3]={{1,0.2f,0.2f},{1,0.9f,0.1f},{0.2f,0.8f,0.2f},{0.2f,0.5f,1},{1,0.2f,0.8f}};
        glDisable(GL_LIGHTING);
        glColor3f(0.22f,0.22f,0.22f);
        glBegin(GL_LINE_STRIP);
        for(int i=0;i<=NB;i++){
            float t=(float)i/NB;
            float sag=-sinf(t*(float)M_PI)*2.5f;
            glVertex3f(-gap+1.0f+t*(gap*2-2), hy+sag, strand==0?-0.5f:0.5f);
        }
        glEnd();
        glEnable(GL_LIGHTING);
        for(int i=0;i<=NB;i++){
            float t=(float)i/NB;
            float sag=-sinf(t*(float)M_PI)*2.5f;
            int ci=i%5;
            glColor3f(bulbC[ci][0],bulbC[ci][1],bulbC[ci][2]);
            glPushMatrix();
            glTranslatef(-gap+1.0f+t*(gap*2-2), hy+sag, strand==0?-0.5f:0.5f);
            glutSolidSphere(0.22f,6,6);
            glPopMatrix();
        }
    }

    /* ── "FUNLAND" sign board on beam — faces OUTWARD (south, negative Z) ── */
    /* Background board */
    glColor3f(0.08f,0.06f,0.28f);
    glPushMatrix();glTranslatef(0,ph+0.8f+4.8f,-0.1f);glScalef(14.5f,3.2f,0.4f);glutSolidCube(1);glPopMatrix();

    /* Colored inner panel */
    glColor3f(0.12f,0.08f,0.45f);
    glPushMatrix();glTranslatef(0,ph+0.8f+4.8f,-0.35f);glScalef(13.8f,2.6f,0.15f);glutSolidCube(1);glPopMatrix();

    /* Gold border top/bottom */
    glColor3f(1.0f,0.85f,0.10f);
    glPushMatrix();glTranslatef(0,ph+0.8f+6.3f,-0.35f);glScalef(14.2f,0.32f,0.18f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0,ph+0.8f+3.3f,-0.35f);glScalef(14.2f,0.32f,0.18f);glutSolidCube(1);glPopMatrix();

    /* Letter F  (mirrored X for correct reading from outside) */
    glColor3f(1.0f,0.92f,0.12f);
    glPushMatrix();glTranslatef(5.8f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.38f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(5.22f,ph+0.8f+5.7f,-0.45f);
    glScalef(0.75f,0.32f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(5.32f,ph+0.8f+4.9f,-0.45f);
    glScalef(0.55f,0.30f,0.12f);glutSolidCube(1);glPopMatrix();

    /* Letter U */
    glColor3f(1.0f,0.40f,0.10f);
    glPushMatrix();glTranslatef(4.0f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(3.2f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(3.6f,ph+0.8f+3.85f,-0.45f);
    glScalef(1.1f,0.30f,0.12f);glutSolidCube(1);glPopMatrix();

    /* Letter N */
    glColor3f(0.18f,0.88f,0.28f);
    glPushMatrix();glTranslatef(2.15f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(1.35f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glBegin(GL_TRIANGLES);
    glVertex3f(2.15f,ph+0.8f+5.82f,-0.47f);
    glVertex3f(1.35f,ph+0.8f+5.82f,-0.47f);
    glVertex3f(2.15f,ph+0.8f+3.82f,-0.47f);
    glEnd();

    /* Letter L */
    glColor3f(0.18f,0.62f,1.0f);
    glPushMatrix();glTranslatef(0.35f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-0.15f,ph+0.8f+3.85f,-0.45f);
    glScalef(1.32f,0.30f,0.12f);glutSolidCube(1);glPopMatrix();

    /* Letter A */
    glColor3f(0.82f,0.18f,0.88f);
    glPushMatrix();glTranslatef(-1.05f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-1.85f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    /* A crossbar */
    glPushMatrix();glTranslatef(-1.45f,ph+0.8f+4.9f,-0.45f);
    glScalef(1.1f,0.28f,0.12f);glutSolidCube(1);glPopMatrix();
    /* A top */
    glBegin(GL_TRIANGLES);
    glVertex3f(-1.05f,ph+0.8f+5.82f,-0.47f);
    glVertex3f(-1.85f,ph+0.8f+5.82f,-0.47f);
    glVertex3f(-1.45f,ph+0.8f+6.6f,-0.47f);
    glEnd();

    /* Letter N (second) */
    glColor3f(1.0f,0.62f,0.08f);
    glPushMatrix();glTranslatef(-2.85f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-3.65f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    glBegin(GL_TRIANGLES);
    glVertex3f(-2.85f,ph+0.8f+5.82f,-0.47f);
    glVertex3f(-3.65f,ph+0.8f+5.82f,-0.47f);
    glVertex3f(-2.85f,ph+0.8f+3.82f,-0.47f);
    glEnd();

    /* Letter D */
    glColor3f(0.92f,0.10f,0.28f);
    glPushMatrix();glTranslatef(-4.65f,ph+0.8f+4.8f,-0.45f);
    glScalef(0.32f,2.0f,0.12f);glutSolidCube(1);glPopMatrix();
    /* D arc */
    for(int i=0;i<8;i++){
        float a0=i*180.f/8*(float)M_PI/180 - (float)M_PI/2;
        float a1=(i+1)*180.f/8*(float)M_PI/180 - (float)M_PI/2;
        glBegin(GL_QUADS);
        glVertex3f(-4.65f+cosf(a0)*0.85f,ph+0.8f+4.8f+sinf(a0)*1.05f,-0.47f);
        glVertex3f(-4.65f+cosf(a0)*0.55f,ph+0.8f+4.8f+sinf(a0)*0.75f,-0.47f);
        glVertex3f(-4.65f+cosf(a1)*0.55f,ph+0.8f+4.8f+sinf(a1)*0.75f,-0.47f);
        glVertex3f(-4.65f+cosf(a1)*0.85f,ph+0.8f+4.8f+sinf(a1)*1.05f,-0.47f);
        glEnd();
    }

    /* Star decorations around sign — outward face */
    float starC[][3]={{1,1,0},{1,0.5f,0},{0.5f,1,0},{0,0.8f,1},{1,0.2f,0.8f}};
    for(int st=0;st<8;st++){
        glColor3f(starC[st%5][0],starC[st%5][1],starC[st%5][2]);
        float sx=-6.2f+st*1.8f;
        float sy=ph+0.8f+6.8f+sinf(windTime*2+st)*0.25f;
        glPushMatrix();glTranslatef(sx,sy,-0.48f);glutSolidSphere(0.22f,8,8);glPopMatrix();
    }

    /* Bulb lights across beam bottom — outward face */
    for(int b=0;b<16;b++){
        float bx=-7.2f+b*0.96f;
        glColor3f(1,1,0.6f);
        glPushMatrix();glTranslatef(bx,ph+0.8f-0.12f,-0.6f);glutSolidSphere(0.18f,6,6);glPopMatrix();
        glDisable(GL_LIGHTING);
        glColor3f(0.3f,0.3f,0.3f);
        glBegin(GL_LINES);
        glVertex3f(bx,ph+0.8f,-0.6f);
        glVertex3f(bx,ph+0.8f-0.12f,-0.6f);
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
    glColor3f(.22f,.40f,.82f);
    glPushMatrix();glScalef(2.8f,4.0f,2.2f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(.88f,.12f,.12f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-1.6f,4.1f,1.2f);glVertex3f(1.6f,4.1f,1.2f);glVertex3f(0,5.6f,0);
    glVertex3f(-1.6f,4.1f,-1.2f);glVertex3f(1.6f,4.1f,-1.2f);glVertex3f(0,5.6f,0);
    glVertex3f(-1.6f,4.1f,-1.2f);glVertex3f(-1.6f,4.1f,1.2f);glVertex3f(0,5.6f,0);
    glVertex3f(1.6f,4.1f,-1.2f);glVertex3f(1.6f,4.1f,1.2f);glVertex3f(0,5.6f,0);
    glEnd();
    glColor3f(.82f,.94f,1.0f);
    glPushMatrix();glTranslatef(0,2.0f,1.12f);glScalef(1.3f,.9f,.06f);glutSolidCube(1);glPopMatrix();
    /* TICKETS sign board above window */
    glColor3f(0.08f,0.06f,0.28f);
    glPushMatrix();glTranslatef(0,3.4f,1.15f);glScalef(2.6f,0.75f,.08f);glutSolidCube(1);glPopMatrix();
    glColor3f(1.0f,0.90f,0.10f);
    glPushMatrix();glTranslatef(0,3.4f,1.2f);glScalef(2.3f,0.50f,.04f);glutSolidCube(1);glPopMatrix();
    /* T letter */
    glColor3f(0.08f,0.06f,0.28f);
    glPushMatrix();glTranslatef(-0.92f,3.4f,1.22f);glScalef(0.06f,0.38f,.04f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-0.92f,3.6f,1.22f);glScalef(0.28f,0.06f,.04f);glutSolidCube(1);glPopMatrix();
    /* I */
    glPushMatrix();glTranslatef(-0.62f,3.4f,1.22f);glScalef(0.06f,0.38f,.04f);glutSolidCube(1);glPopMatrix();
    /* C */
    glPushMatrix();glTranslatef(-0.32f,3.4f,1.22f);glScalef(0.06f,0.38f,.04f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-0.20f,3.6f,1.22f);glScalef(0.16f,0.06f,.04f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-0.20f,3.22f,1.22f);glScalef(0.16f,0.06f,.04f);glutSolidCube(1);glPopMatrix();
    /* K */
    glPushMatrix();glTranslatef(0.02f,3.4f,1.22f);glScalef(0.06f,0.38f,.04f);glutSolidCube(1);glPopMatrix();
    glBegin(GL_TRIANGLES);
    glVertex3f(0.05f,3.6f,1.24f);glVertex3f(0.28f,3.68f,1.24f);glVertex3f(0.05f,3.41f,1.24f);
    glVertex3f(0.05f,3.41f,1.24f);glVertex3f(0.28f,3.22f,1.24f);glVertex3f(0.05f,3.22f,1.24f);
    glEnd();
    /* E */
    glPushMatrix();glTranslatef(0.36f,3.4f,1.22f);glScalef(0.06f,0.38f,.04f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0.48f,3.6f,1.22f);glScalef(0.18f,0.06f,.04f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0.48f,3.41f,1.22f);glScalef(0.14f,0.06f,.04f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0.48f,3.22f,1.22f);glScalef(0.18f,0.06f,.04f);glutSolidCube(1);glPopMatrix();
    /* T (second, as S substitute) */
    glPushMatrix();glTranslatef(0.72f,3.4f,1.22f);glScalef(0.06f,0.38f,.04f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0.72f,3.6f,1.22f);glScalef(0.28f,0.06f,.04f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0.72f,3.41f,1.22f);glScalef(0.18f,0.06f,.04f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0.84f,3.22f,1.22f);glScalef(0.16f,0.06f,.04f);glutSolidCube(1);glPopMatrix();
    /* slide-out counter */
    glColor3f(.52f,.30f,.10f);
    glPushMatrix();glTranslatef(0,1.55f,1.38f);glScalef(1.8f,.14f,.55f);glutSolidCube(1);glPopMatrix();
    float bc[][3]={{1,.18f,.18f},{.18f,.5f,1},{1,.85f,.05f}};
    for(int b=0;b<3;b++){
        float sw=sinf(windTime*1.5f+b)*.15f;
        drawBalloon(-.65f+b*.65f,6.2f,0,bc[b][0],bc[b][1],bc[b][2],sw);
    }
    glPopMatrix();
}

/* ─────────────────────────────
   QUEUE BARRIER
───────────────────────────── */
void drawQueueBarrier(float x,float z,float ang){
    glPushMatrix();glTranslatef(x,0,z);glRotatef(ang,0,1,0);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.65f,0.55f,0.12f);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.07,.07,1.2f,8,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,1.22f,0);glutSolidSphere(.14f,8,8);glPopMatrix();
    glColor3f(.72f,.12f,.12f);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<=12;i++){
        float t=(float)i/12;
        float sag=sinf(t*(float)M_PI)*(-0.08f);
        glVertex3f(t*2.5f,1.1f+sag,0);
    }
    glEnd();
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   FOOD STALL  — 3 distinct types
───────────────────────────── */

/* Type A: Pizza / Italian — round roof, red/white */
void drawFoodStallPizza(float cx,float cz,float ang){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(ang,0,1,0);
    GLUquadric*q=gluNewQuadric();
    /* Counter */
    glColor3f(0.55f,0.25f,0.10f);
    glPushMatrix();glScalef(4.5f,1.1f,2.2f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(0.85f,0.80f,0.60f);
    glPushMatrix();glTranslatef(0,1.16f,0);glScalef(4.7f,.12f,2.4f);glutSolidCube(1);glPopMatrix();
    /* Corner posts */
    for(int s=-1;s<=1;s+=2) for(int t=-1;t<=1;t+=2){
        glColor3f(0.82f,0.12f,0.12f);
        glPushMatrix();glTranslatef(s*2.0f,1.1f,t*0.95f);glRotatef(-90,1,0,0);
        gluCylinder(q,.12,.12,4.2f,8,1);glPopMatrix();
    }
    /* Conical roof with red/white stripes */
    int RS=16;
    for(int r=0;r<RS;r++){
        float a0=r*2*(float)M_PI/RS, a1=(r+1)*2*(float)M_PI/RS;
        glColor3f(r%2==0?0.92f:0.96f, r%2==0?0.12f:0.96f, r%2==0?0.12f:0.96f);
        glBegin(GL_TRIANGLES);
        glVertex3f(0,7.5f,0);
        glVertex3f(2.4f*cosf(a0),5.3f,2.4f*sinf(a0));
        glVertex3f(2.4f*cosf(a1),5.3f,2.4f*sinf(a1));
        glEnd();
    }
    /* Roof base ring */
    glColor3f(0.85f,0.12f,0.12f);
    glPushMatrix();glTranslatef(0,5.3f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,2.55f,2.55f,0.35f,16,1);glPopMatrix();
    /* Spire on top */
    glColor3f(1.0f,0.88f,0.08f);
    glPushMatrix();glTranslatef(0,7.5f,0);glutSolidSphere(0.32f,8,8);glPopMatrix();
    /* Menu board */
    glColor3f(0.12f,0.12f,0.15f);
    glPushMatrix();glTranslatef(0,3.0f,1.12f);glScalef(3.2f,1.2f,.10f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.92f,0.18f,0.18f);
    glPushMatrix();glTranslatef(0,3.55f,1.18f);glScalef(3.0f,.42f,.04f);glutSolidCube(1);glPopMatrix();
    glColor3f(1.0f,0.92f,0.0f);
    glPushMatrix();glTranslatef(0,2.85f,1.18f);glScalef(2.8f,0.72f,.04f);glutSolidCube(1);glPopMatrix();
    /* Pizza on counter */
    glColor3f(0.82f,0.48f,0.18f);
    glPushMatrix();glTranslatef(-0.5f,1.18f,0.2f);glRotatef(-90,1,0,0);gluDisk(q,0,0.55f,12,2);glPopMatrix();
    glColor3f(0.92f,0.22f,0.18f);
    glPushMatrix();glTranslatef(-0.5f,1.19f,0.2f);glRotatef(-90,1,0,0);gluDisk(q,0,0.42f,12,2);glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

/* Type B: Burger/Grill — flat roof, yellow/red, big boards */
void drawFoodStallBurger(float cx,float cz,float ang){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(ang,0,1,0);
    GLUquadric*q=gluNewQuadric();
    /* Counter */
    glColor3f(0.42f,0.22f,0.08f);
    glPushMatrix();glScalef(5.0f,1.2f,2.3f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(0.96f,0.82f,0.22f);
    glPushMatrix();glTranslatef(0,1.28f,0);glScalef(5.2f,.14f,2.5f);glutSolidCube(1);glPopMatrix();
    /* Corner posts yellow with red stripe */
    for(int s=-1;s<=1;s+=2) for(int t=-1;t<=1;t+=2){
        glColor3f(0.98f,0.82f,0.08f);
        glPushMatrix();glTranslatef(s*2.2f,1.3f,t*1.0f);glRotatef(-90,1,0,0);
        gluCylinder(q,.13,.13,3.8f,8,1);glPopMatrix();
        glColor3f(0.92f,0.18f,0.18f);
        glPushMatrix();glTranslatef(s*2.2f,2.8f,t*1.0f);glRotatef(-90,1,0,0);
        gluCylinder(q,.14,.14,0.5f,8,1);glPopMatrix();
    }
    /* Flat roof with overhang — striped awning */
    for(int s=0;s<10;s++){
        glColor3f(s%2==0?0.98f:0.92f, s%2==0?0.82f:0.18f, s%2==0?0.08f:0.18f);
        float ax=-2.2f+s*.46f;
        glBegin(GL_QUADS);
        glVertex3f(ax,5.1f,-1.1f);glVertex3f(ax+.46f,5.1f,-1.1f);
        glVertex3f(ax+.46f,4.55f,-1.75f);glVertex3f(ax,4.55f,-1.75f);glEnd();
        glBegin(GL_QUADS);
        glVertex3f(ax,5.1f,-1.1f);glVertex3f(ax+.46f,5.1f,-1.1f);
        glVertex3f(ax+.46f,5.1f,1.1f);glVertex3f(ax,5.1f,1.1f);glEnd();
        glBegin(GL_QUADS);
        glVertex3f(ax,5.1f,1.1f);glVertex3f(ax+.46f,5.1f,1.1f);
        glVertex3f(ax+.46f,4.55f,1.75f);glVertex3f(ax,4.55f,1.75f);glEnd();
    }
    /* Big golden arches style sign */
    glColor3f(0.96f,0.78f,0.08f);
    for(int a=0;a<2;a++){
        for(int i=0;i<12;i++){
            float ai0=i*(float)M_PI/12 + (float)M_PI;
            float ai1=(i+1)*(float)M_PI/12 + (float)M_PI;
            glBegin(GL_QUADS);
            glVertex3f(-1.5f+a*1.6f+cosf(ai0)*0.85f,5.8f+sinf(ai0)*0.85f,1.2f);
            glVertex3f(-1.5f+a*1.6f+cosf(ai0)*0.52f,5.8f+sinf(ai0)*0.52f,1.2f);
            glVertex3f(-1.5f+a*1.6f+cosf(ai1)*0.52f,5.8f+sinf(ai1)*0.52f,1.2f);
            glVertex3f(-1.5f+a*1.6f+cosf(ai1)*0.85f,5.8f+sinf(ai1)*0.85f,1.2f);
            glEnd();
        }
    }
    /* Menu board */
    glColor3f(0.08f,0.08f,0.10f);
    glPushMatrix();glTranslatef(0,3.2f,1.22f);glScalef(3.8f,1.2f,.08f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.96f,0.78f,0.08f);
    glPushMatrix();glTranslatef(0,3.5f,1.28f);glScalef(3.5f,.5f,.04f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.18f,0.72f,0.28f);
    glPushMatrix();glTranslatef(0,2.95f,1.28f);glScalef(3.5f,.55f,.04f);glutSolidCube(1);glPopMatrix();
    /* Burger on counter */
    glColor3f(0.75f,0.45f,0.15f);
    glPushMatrix();glTranslatef(0.8f,1.32f,0.1f);glScalef(.52f,.22f,.52f);glutSolidSphere(1,8,8);glPopMatrix();
    glColor3f(0.88f,0.62f,0.18f);
    glPushMatrix();glTranslatef(0.8f,1.5f,0.1f);glScalef(.48f,.15f,.48f);glutSolidSphere(1,8,8);glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

/* Type C: Ice Cream / Drinks — pastel colors, fun */
void drawFoodStallIceCream(float cx,float cz,float ang){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(ang,0,1,0);
    GLUquadric*q=gluNewQuadric();
    /* Counter */
    glColor3f(0.82f,0.50f,0.75f);
    glPushMatrix();glScalef(4.0f,1.1f,2.0f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(0.95f,0.88f,0.92f);
    glPushMatrix();glTranslatef(0,1.16f,0);glScalef(4.2f,.12f,2.2f);glutSolidCube(1);glPopMatrix();
    /* Striped candy posts */
    for(int s=-1;s<=1;s+=2) for(int t=-1;t<=1;t+=2){
        for(int seg=0;seg<10;seg++){
            glColor3f(seg%2==0?0.95f:0.85f, seg%2==0?0.35f:0.72f, seg%2==0?0.65f:0.85f);
            glPushMatrix();glTranslatef(s*1.8f,1.1f+seg*0.42f,t*0.85f);
            glScalef(0.26f,0.42f,0.26f);glutSolidSphere(1,6,6);glPopMatrix();
        }
    }
    /* Curved dome roof — pastel pink */
    glColor3f(0.95f,0.72f,0.85f);
    glPushMatrix();glTranslatef(0,5.5f,0);glScalef(2.4f,1.0f,1.4f);
    GLUquadric*q2=gluNewQuadric();gluSphere(q2,1.8f,14,8);gluDeleteQuadric(q2);glPopMatrix();

    /* Big ice cream cone sign */
    glColor3f(0.88f,0.58f,0.22f);
    glPushMatrix();glTranslatef(0,8.5f,0.2f);
    /* cone */
    glRotatef(-90,1,0,0);gluCylinder(q,0.05f,0.6f,1.4f,10,1);glPopMatrix();
    /* scoop */
    glColor3f(0.98f,0.68f,0.75f);
    glPushMatrix();glTranslatef(0,9.95f,0.2f);glutSolidSphere(0.65f,12,12);glPopMatrix();
    /* second scoop */
    glColor3f(0.68f,0.82f,0.98f);
    glPushMatrix();glTranslatef(0.2f,10.58f,0.2f);glutSolidSphere(0.52f,10,10);glPopMatrix();

    /* Menu board */
    glColor3f(0.78f,0.35f,0.68f);
    glPushMatrix();glTranslatef(0,3.1f,1.08f);glScalef(3.0f,1.1f,.10f);glutSolidCube(1);glPopMatrix();
    glColor3f(1.0f,0.88f,0.92f);
    glPushMatrix();glTranslatef(0,3.1f,1.14f);glScalef(2.6f,.80f,.04f);glutSolidCube(1);glPopMatrix();

    /* Items on counter */
    float sc[][3]={{0.98f,0.72f,0.78f},{0.72f,0.82f,0.98f},{0.92f,0.98f,0.68f}};
    for(int i=0;i<3;i++){
        glColor3f(sc[i][0],sc[i][1],sc[i][2]);
        glPushMatrix();glTranslatef(-1.1f+i*1.1f,1.72f,0.1f);glutSolidSphere(0.28f,8,8);glPopMatrix();
    }
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   DINING TABLE WITH UMBRELLA (food court)
   Enhanced with better chairs
───────────────────────────── */
void drawDiningTable(float x,float z,float r,float g,float b){
    glPushMatrix();glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();

    /* Table legs — 4 turned legs */
    glColor3f(0.52f,0.38f,0.22f);
    float lx[]={-0.62f,0.62f,-0.62f,0.62f},lz[]={-0.62f,-0.62f,0.62f,0.62f};
    for(int i=0;i<4;i++){
        glPushMatrix();glTranslatef(lx[i],0,lz[i]);glRotatef(-90,1,0,0);
        gluCylinder(q,.055,.075,.85f,8,1);glPopMatrix();
        /* turned detail */
        glPushMatrix();glTranslatef(lx[i],.38f,lz[i]);glutSolidSphere(0.10f,6,6);glPopMatrix();
    }
    /* Cross brace */
    glColor3f(0.48f,0.34f,0.18f);
    glPushMatrix();glTranslatef(0,.25f,0);glScalef(1.3f,.06f,.06f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0,.25f,0);glScalef(.06f,.06f,1.3f);glutSolidCube(1);glPopMatrix();
    /* Table top — round */
    glColor3f(0.82f,0.66f,0.46f);
    glPushMatrix();glTranslatef(0,.87f,0);glRotatef(-90,1,0,0);gluDisk(q,0,1.05f,20,2);glPopMatrix();
    /* Table edge trim */
    glColor3f(0.65f,0.50f,0.30f);
    glPushMatrix();glTranslatef(0,.87f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,1.05f,1.05f,.06f,20,1);glPopMatrix();

    /* Umbrella pole */
    glColor3f(0.40f,0.40f,0.45f);
    glPushMatrix();glTranslatef(0,.87f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.055,.055,3.2f,8,1);glPopMatrix();
    /* Umbrella finial */
    glColor3f(r,g,b);
    glPushMatrix();glTranslatef(0,4.1f,0);glutSolidSphere(0.14f,8,8);glPopMatrix();

    /* Umbrella canopy — 12 segments, alternating color */
    int US=12;
    for(int s=0;s<US;s++){
        float a0=s*2*(float)M_PI/US;
        float a1=(s+1)*2*(float)M_PI/US;
        float am=(a0+a1)*0.5f;
        /* Main panel */
        glColor3f(s%2==0?r:1, s%2==0?g:1, s%2==0?b:1);
        glBegin(GL_TRIANGLES);
        glVertex3f(0,4.05f,0);
        glVertex3f(1.95f*cosf(a0),3.15f,1.95f*sinf(a0));
        glVertex3f(1.95f*cosf(a1),3.15f,1.95f*sinf(a1));
        glEnd();
        /* Scalloped edge */
        glColor3f(s%2==0?r*0.8f:0.9f, s%2==0?g*0.8f:0.9f, s%2==0?b*0.8f:0.9f);
        glBegin(GL_TRIANGLES);
        glVertex3f(1.95f*cosf(a0),3.15f,1.95f*sinf(a0));
        glVertex3f(2.22f*cosf(am),2.88f,2.22f*sinf(am));
        glVertex3f(1.95f*cosf(a1),3.15f,1.95f*sinf(a1));
        glEnd();
    }

    /* 4 Chairs around table — proper with back and armrests */
    for(int c=0;c<4;c++){
        float ca=c*90*(float)M_PI/180;
        float chx=1.55f*cosf(ca), chz=1.55f*sinf(ca);
        float faceAng=c*90.0f+180.0f;
        glPushMatrix();glTranslatef(chx,0,chz);glRotatef(faceAng,0,1,0);

        /* Chair legs */
        glColor3f(0.55f,0.38f,0.18f);
        float flx[]={-0.28f,0.28f,-0.28f,0.28f},flz[]={-0.28f,-0.28f,0.28f,0.28f};
        for(int l=0;l<4;l++){
            glPushMatrix();glTranslatef(flx[l],0,flz[l]);glRotatef(-90,1,0,0);
            gluCylinder(q,.04f,.04f,.5f,6,1);glPopMatrix();
        }
        /* Seat */
        glColor3f(0.72f,0.48f,0.22f);
        glPushMatrix();glTranslatef(0,.52f,0);glScalef(.62f,.08f,.62f);glutSolidCube(1);glPopMatrix();
        /* Back rest */
        glColor3f(0.65f,0.42f,0.18f);
        glPushMatrix();glTranslatef(0,.95f,-0.27f);glScalef(.60f,.72f,.07f);glutSolidCube(1);glPopMatrix();
        /* Back legs extension */
        glColor3f(0.55f,0.38f,0.18f);
        glPushMatrix();glTranslatef(-0.25f,.52f,-0.27f);glRotatef(-90,1,0,0);
        gluCylinder(q,.04f,.04f,.45f,6,1);glPopMatrix();
        glPushMatrix();glTranslatef(0.25f,.52f,-0.27f);glRotatef(-90,1,0,0);
        gluCylinder(q,.04f,.04f,.45f,6,1);glPopMatrix();
        glPopMatrix();
    }

    /* Items on table */
    glColor3f(0.88f,0.22f,0.22f);
    glPushMatrix();glTranslatef(0.35f,.92f,0.12f);glutSolidSphere(.09f,6,6);glPopMatrix(); /* ketchup */
    glColor3f(0.92f,0.88f,0.12f);
    glPushMatrix();glTranslatef(-0.28f,.92f,0.12f);glutSolidSphere(.09f,6,6);glPopMatrix(); /* mustard */

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

/* ─────────────────────────────
   STAGE
───────────────────────────── */
void drawStage(float cx,float cz){
    glPushMatrix();glTranslatef(cx,0,cz);
    glRotatef(180,0,1,0);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.58f,0.32f,0.12f);
    glPushMatrix();glTranslatef(0,.65f,0);glScalef(18.0f,1.3f,10.0f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.72f,0.44f,0.18f);
    glPushMatrix();glTranslatef(0,1.32f,0);glScalef(18.2f,.12f,10.2f);glutSolidCube(1);glPopMatrix();
    for(int s=0;s<4;s++){
        glColor3f(0.65f+(s*.03f),0.38f+(s*.02f),0.15f);
        glPushMatrix();glTranslatef(0,s*.38f+.19f,5.2f+s*.55f);
        glScalef(10.0f,.38f,.55f);glutSolidCube(1);glPopMatrix();
    }
    glColor3f(0.62f,0.08f,0.58f);
    glPushMatrix();glTranslatef(0,7.2f,-5.0f);glScalef(18.5f,12.5f,.5f);glutSolidCube(1);glPopMatrix();
    for(int f=0;f<10;f++){
        float fx=-8.5f+f*1.9f;
        glColor3f(f%2==0?.52f:.72f,f%2==0?.05f:.12f,f%2==0?.48f:.68f);
        glPushMatrix();glTranslatef(fx,7.2f,-4.78f);glScalef(.65f,12.2f,.28f);glutSolidCube(1);glPopMatrix();
    }
    for(int s=-1;s<=1;s+=2){
        glColor3f(0.25f,0.25f,0.28f);
        glPushMatrix();glTranslatef(s*10.0f,0,-3.5f);glRotatef(-90,1,0,0);
        gluCylinder(q,.25,.20,14.5f,12,1);glPopMatrix();
        float spC[][3]={{1,0.4f,0},{0.4f,1,0},{0,0.4f,1},{1,0,0.4f}};
        for(int l=0;l<4;l++){
            glColor3f(spC[l][0],spC[l][1],spC[l][2]);
            glPushMatrix();glTranslatef(s*10.0f,13.5f,-3.5f);
            glRotatef(sinf(spotAngle*(float)M_PI/180+l*0.8f)*22,0,1,0);
            glRotatef(-50,1,0,0);
            gluCylinder(q,.22,.48,2.2f,8,1);glPopMatrix();
        }
    }
    glColor3f(0.32f,0.32f,0.38f);
    for(int s=-1;s<=1;s+=2){
        glPushMatrix();glTranslatef(s*8.5f,0,-4.8f);glRotatef(-90,1,0,0);
        gluCylinder(q,.28,.24,16.0f,10,1);glPopMatrix();
    }
    glPushMatrix();glTranslatef(0,16.0f,-4.8f);glRotatef(90,0,1,0);
    gluCylinder(q,.24,.24,17.2f,10,1);glPopMatrix();
    for(int b=0;b<18;b++){
        float bx=-8.2f+b*0.95f;
        float sag=sinf((float)b/17*(float)M_PI)*(-0.7f);
        float bc[][3]={{1,.1f,.1f},{1,.7f,.0f},{1,1,.0f},{.0f,.8f,.2f},{.0f,.4f,1},{.6f,.0f,.8f}};
        glColor3f(bc[b%6][0],bc[b%6][1],bc[b%6][2]);
        glBegin(GL_TRIANGLES);
        glVertex3f(bx,15.8f+sag,-4.5f);
        glVertex3f(bx+.48f,15.8f+sag,-4.5f);
        glVertex3f(bx+.24f,14.9f+sag,-4.5f);
        glEnd();
    }
    glColor3f(0.18f,0.18f,0.18f);
    glPushMatrix();glTranslatef(0,1.35f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.05,.05,3.8f,8,1);glPopMatrix();
    glColor3f(0.28f,0.28f,0.28f);
    glPushMatrix();glTranslatef(0,5.18f,0);glutSolidSphere(.22f,10,10);glPopMatrix();
    for(int s=-1;s<=1;s+=2){
        glColor3f(0.12f,0.12f,0.15f);
        glPushMatrix();glTranslatef(s*7.2f,1.35f,3.5f);glScalef(2.0f,3.0f,1.4f);glutSolidCube(1);glPopMatrix();
        glColor3f(0.30f,0.30f,0.32f);
        for(int r=0;r<3;r++){
            glPushMatrix();glTranslatef(s*7.2f,2.0f+r*.8f,4.22f);
            gluDisk(q,0,.55f,12,2);glPopMatrix();
        }
    }
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   GRAND FOUNTAIN
───────────────────────────── */
void drawFountain(float cx,float cz){
    glPushMatrix();glTranslatef(cx,0,cz);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.68f,0.72f,0.80f);
    glPushMatrix();glRotatef(-90,1,0,0);gluDisk(q,0,7.0f,32,4);glPopMatrix();
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,7.0f,7.0f,.8f,32,1);glPopMatrix();
    glColor3f(0.78f,0.82f,0.90f);
    glPushMatrix();glTranslatef(0,.82f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,7.0f,7.4f,.28f,32,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,1.1f,0);glRotatef(-90,1,0,0);gluDisk(q,6.6f,7.4f,32,2);glPopMatrix();
    glColor3f(0.32f,0.60f,0.90f);
    glPushMatrix();glTranslatef(0,.75f,0);glRotatef(-90,1,0,0);gluDisk(q,0,6.6f,32,4);glPopMatrix();
    glColor3f(0.78f,0.80f,0.88f);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.7f,.55f,3.8f,18,2);glPopMatrix();
    glColor3f(0.88f,0.82f,0.62f);
    glPushMatrix();glTranslatef(0,.4f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,1.1f,1.0f,.32f,18,1);glPopMatrix();
    glColor3f(0.72f,0.76f,0.84f);
    glPushMatrix();glTranslatef(0,3.8f,0);glRotatef(-90,1,0,0);
    gluDisk(q,0,4.2f,24,3);glPopMatrix();
    glPushMatrix();glTranslatef(0,3.8f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,4.2f,4.2f,.55f,24,1);glPopMatrix();
    glColor3f(0.80f,0.84f,0.92f);
    glPushMatrix();glTranslatef(0,4.38f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,4.2f,4.55f,.22f,24,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,4.6f,0);glRotatef(-90,1,0,0);gluDisk(q,3.9f,4.55f,24,2);glPopMatrix();
    glColor3f(0.35f,0.62f,0.92f);
    glPushMatrix();glTranslatef(0,4.32f,0);glRotatef(-90,1,0,0);gluDisk(q,0,3.9f,24,3);glPopMatrix();
    glColor3f(0.80f,0.82f,0.90f);
    glPushMatrix();glTranslatef(0,3.8f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.48f,.36f,3.4f,14,2);glPopMatrix();
    glColor3f(0.75f,0.78f,0.86f);
    glPushMatrix();glTranslatef(0,7.2f,0);glRotatef(-90,1,0,0);
    gluDisk(q,0,2.0f,18,3);glPopMatrix();
    glPushMatrix();glTranslatef(0,7.2f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,2.0f,2.0f,.4f,18,1);glPopMatrix();
    glColor3f(0.82f,0.86f,0.94f);
    glPushMatrix();glTranslatef(0,7.62f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,2.0f,2.3f,.18f,18,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,7.8f,0);glRotatef(-90,1,0,0);gluDisk(q,1.8f,2.3f,18,2);glPopMatrix();
    glColor3f(0.38f,0.65f,0.95f);
    glPushMatrix();glTranslatef(0,7.55f,0);glRotatef(-90,1,0,0);gluDisk(q,0,1.8f,18,3);glPopMatrix();
    glColor3f(0.88f,0.78f,0.15f);
    glPushMatrix();glTranslatef(0,7.2f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.18f,.12f,2.8f,10,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,10.05f,0);glutSolidSphere(.42f,14,14);glPopMatrix();
    for(int t=0;t<6;t++){
        float ta=t*60*(float)M_PI/180;
        glColor3f(1.0f,0.90f,0.15f);
        glPushMatrix();glTranslatef(cosf(ta)*.55f,10.05f,sinf(ta)*.55f);
        glutSolidSphere(.16f,8,8);glPopMatrix();
    }
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(int j=0;j<12;j++){
        float ja=j*30*(float)M_PI/180;
        float jph=fountainTime*1.2f+j*.52f;
        float jh=sinf(jph)*0.6f+2.4f;
        for(int s=0;s<10;s++){
            float t=(float)s/10, t1=(float)(s+1)/10;
            float arc=jh*(1-(2*t-1)*(2*t-1));
            float arc1=jh*(1-(2*t1-1)*(2*t1-1));
            float r0=5.8f+t*1.4f, r1=5.8f+t1*1.4f;
            glColor4f(0.55f,0.82f,1.0f,0.55f);
            glBegin(GL_QUADS);
            glVertex3f(cosf(ja)*(r0-.07f),0.78f+arc,sinf(ja)*(r0-.07f));
            glVertex3f(cosf(ja)*(r0+.07f),0.78f+arc,sinf(ja)*(r0+.07f));
            glVertex3f(cosf(ja)*(r1+.07f),0.78f+arc1,sinf(ja)*(r1+.07f));
            glVertex3f(cosf(ja)*(r1-.07f),0.78f+arc1,sinf(ja)*(r1-.07f));
            glEnd();
        }
    }
    for(int j=0;j<8;j++){
        float ja=j*45*(float)M_PI/180;
        float jph=fountainTime+j*.8f;
        float jh=sinf(jph)*0.5f+3.0f;
        for(int s=0;s<12;s++){
            float t=(float)s/12, t1=(float)(s+1)/12;
            float arc=jh*(1-(2*t-1)*(2*t-1));
            float arc1=jh*(1-(2*t1-1)*(2*t1-1));
            float r0=3.2f+t*1.0f, r1=3.2f+t1*1.0f;
            glColor4f(0.60f,0.86f,1.0f,0.65f);
            glBegin(GL_QUADS);
            glVertex3f(cosf(ja)*(r0-.06f),4.35f+arc,sinf(ja)*(r0-.06f));
            glVertex3f(cosf(ja)*(r0+.06f),4.35f+arc,sinf(ja)*(r0+.06f));
            glVertex3f(cosf(ja)*(r1+.06f),4.35f+arc1,sinf(ja)*(r1+.06f));
            glVertex3f(cosf(ja)*(r1-.06f),4.35f+arc1,sinf(ja)*(r1-.06f));
            glEnd();
        }
    }
    glDisable(GL_BLEND);
    for(int s=0;s<16;s++){
        float sa=s*22.5f*(float)M_PI/180;
        glColor3f(0.70f,0.72f,0.78f);
        glPushMatrix();glTranslatef(cosf(sa)*7.8f,0.05f,sinf(sa)*7.8f);
        glScalef(.55f,.35f,.55f);glutSolidSphere(1,8,8);glPopMatrix();
    }
    gluDeleteQuadric(q);glPopMatrix();
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
    glPushMatrix();glTranslatef(cx,0,cz);
    GLUquadric*q=gluNewQuadric();
    glColor3f(.62f,.18f,.58f);
    glPushMatrix();glRotatef(-90,1,0,0);gluDisk(q,0,5.8f,32,2);glPopMatrix();
    glColor3f(.88f,.88f,.10f);
    glPushMatrix();glTranslatef(0,.08f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,5.7f,5.7f,.35f,32,1);glPopMatrix();
    glColor3f(.82f,.82f,.85f);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.28f,.28f,9.0f,14,1);glPopMatrix();
    glColor3f(1.0f,.88f,.0f);
    glPushMatrix();glTranslatef(0,9.3f,0);glutSolidSphere(.6f,14,14);glPopMatrix();
    glPushMatrix();glRotatef(spin,0,1,0);
    int CS=14; float iR=.3f,oR=5.5f,cY=8.2f;
    for(int s=0;s<CS;s++){
        float a0=s*(360.f/CS)*(float)M_PI/180,a1=(s+1)*(360.f/CS)*(float)M_PI/180;
        glColor3f(s%2==0?.95f:.98f,s%2==0?.12f:.98f,s%2==0?.45f:.98f);
        glBegin(GL_QUADS);
        glVertex3f(iR*cosf(a0),cY,iR*sinf(a0));
        glVertex3f(oR*cosf(a0),cY-.9f,oR*sinf(a0));
        glVertex3f(oR*cosf(a1),cY-.9f,oR*sinf(a1));
        glVertex3f(iR*cosf(a1),cY,iR*sinf(a1));
        glEnd();
    }
    for(int sp=0;sp<8;sp++){
        float sa=sp*45*(float)M_PI/180;
        glColor3f(.85f,.85f,.88f);
        glBegin(GL_LINES);glVertex3f(0,8.6f,0);glVertex3f(oR*cosf(sa),cY-.7f,oR*sinf(sa));glEnd();
    }
    float hc[][3]={{.95f,.90f,.85f},{.60f,.32f,.14f},{.18f,.18f,.18f},
                   {.95f,.72f,.48f},{.52f,.22f,.10f},{.85f,.85f,.80f}};
    for(int h=0;h<6;h++){
        float ha=h*60*(float)M_PI/180;
        float hx=3.9f*cosf(ha),hz=3.9f*sinf(ha);
        float hy=1.1f+.8f*sinf(spin*(float)M_PI/180*3+h*1.05f);
        glPushMatrix();glTranslatef(hx,hy,hz);
        glRotatef(-ha*180/(float)M_PI+90,0,1,0);
        glColor3f(.88f,.78f,.18f);
        glPushMatrix();glRotatef(-90,1,0,0);
        gluCylinder(q,.08,.08,7-hy,8,1);glPopMatrix();
        glColor3f(hc[h][0],hc[h][1],hc[h][2]);
        glPushMatrix();glScalef(.65f,.48f,1.15f);glutSolidSphere(1,12,12);glPopMatrix();
        glPopMatrix();
    }
    glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

/* ─────────────────────────────
   GIANT FERRIS WHEEL
───────────────────────────── */
void drawGiantWheel(float cx,float cz){
    glPushMatrix();glTranslatef(cx,0,cz);
    const float HH=22,WR=14,LS=8;
    GLUquadric*q=gluNewQuadric();
    float lC[][3]={{-LS,0,2},{LS,0,2},{-LS,0,-2},{LS,0,-2}};
    float lR[]={12,-12,12,-12};
    glColor3f(.52f,.52f,.58f);
    for(int i=0;i<4;i++){
        glPushMatrix();glTranslatef(lC[i][0],0,lC[i][2]);
        glRotatef(lR[i],0,0,1);glRotatef(-90,1,0,0);
        gluCylinder(q,.45,.32,HH+3,12,1);glPopMatrix();
    }
    glColor3f(.44f,.44f,.50f);
    float bh[]={HH*.45f,HH*.72f};
    for(int b=0;b<2;b++){
        glPushMatrix();glTranslatef(-LS,bh[b],0);glRotatef(90,0,1,0);
        gluCylinder(q,.22,.22,LS*2,8,1);glPopMatrix();
    }
    glColor3f(.28f,.28f,.32f);
    glPushMatrix();glTranslatef(0,HH,0);glScalef(2.2f,1.6f,2.2f);glutSolidCube(1);glPopMatrix();
    glColor3f(.72f,.72f,.78f);
    glPushMatrix();glTranslatef(0,HH,-2.5f);glRotatef(-90,1,0,0);
    gluCylinder(q,.38,.38,5,12,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,HH,0);glRotatef(wheelAngle,0,0,1);
    const int RS=48;
    for(int ring=0;ring<2;ring++){
        float rz=(ring==0)?-1.6f:1.6f;
        glColor3f(.88f,.18f,.18f);
        for(int s=0;s<RS;s++){
            float a0=s*2*(float)M_PI/RS,a1=(s+1)*2*(float)M_PI/RS;
            glBegin(GL_QUADS);
            glVertex3f(WR*cosf(a0),WR*sinf(a0),rz-.22f);
            glVertex3f(WR*cosf(a1),WR*sinf(a1),rz-.22f);
            glVertex3f(WR*cosf(a1),WR*sinf(a1),rz+.22f);
            glVertex3f(WR*cosf(a0),WR*sinf(a0),rz+.22f);
            glEnd();
        }
    }
    const int SP=16;
    for(int sp=0;sp<SP;sp++){
        float sa=sp*2*(float)M_PI/SP;
        float sx=WR*cosf(sa),sy=WR*sinf(sa);
        glColor3f(sp%2==0?.92f:.96f,sp%2==0?.88f:.96f,sp%2==0?.12f:.96f);
        glLineWidth(2.5f);
        glBegin(GL_LINES);
        glVertex3f(0,0,-1.4f);glVertex3f(sx,sy,-1.4f);
        glVertex3f(0,0,1.4f);glVertex3f(sx,sy,1.4f);
        glEnd();
        glLineWidth(1.0f);
    }
    glColor3f(.96f,.88f,.10f);
    glPushMatrix();glRotatef(-90,1,0,0);gluDisk(q,0,1.6f,16,2);glPopMatrix();
    glPushMatrix();glRotatef(90,1,0,0);gluDisk(q,0,1.6f,16,2);glPopMatrix();
    glutSolidSphere(1.1f,16,16);
    const int NG=14;
    float gc[][3]={{.90f,.18f,.18f},{.18f,.50f,.92f},{.18f,.82f,.22f},
                   {.92f,.72f,.10f},{.82f,.18f,.82f},{.10f,.82f,.82f},
                   {.92f,.48f,.10f},{.18f,.18f,.92f}};
    for(int g=0;g<NG;g++){
        float ga=g*2*(float)M_PI/NG;
        float gx=WR*cosf(ga),gy=WR*sinf(ga);
        glPushMatrix();glTranslatef(gx,gy,0);glRotatef(-wheelAngle,0,0,1);
        glColor3f(.42f,.42f,.48f);
        glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.07,.07,1.3f,6,1);glPopMatrix();
        glTranslatef(0,-1.3f,0);
        int ci=g%8;
        glColor3f(gc[ci][0],gc[ci][1],gc[ci][2]);
        glPushMatrix();glScalef(1.8f,1.4f,1.1f);glutSolidCube(1);glPopMatrix();
        glColor3f(gc[ci][0]*.68f,gc[ci][1]*.68f,gc[ci][2]*.68f);
        glPushMatrix();glTranslatef(0,.78f,0);glScalef(2.0f,.24f,1.3f);glutSolidCube(1);glPopMatrix();
        glColor3f(.72f,.92f,1.0f);
        glPushMatrix();glTranslatef(0,0,.58f);glScalef(1.3f,.55f,.05f);glutSolidCube(1);glPopMatrix();
        glPopMatrix();
    }
    glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
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

/* ─────────────────────────────
   PARKING LOT
───────────────────────────── */
void drawParkingLot(){
    glDisable(GL_LIGHTING);
    glColor3f(.18f,.18f,.18f);
    glBegin(GL_QUADS);
    glVertex3f(30,.02f,30);glVertex3f(68,.02f,30);
    glVertex3f(68,.02f,68);glVertex3f(30,.02f,68);
    glEnd();
    glColor3f(.90f,.90f,.90f);
    for(int b=0;b<=6;b++){float bx=33+b*5.5f;
        glBegin(GL_QUADS);glVertex3f(bx,.05f,34);glVertex3f(bx+.12f,.05f,34);glVertex3f(bx+.12f,.05f,43);glVertex3f(bx,.05f,43);glEnd();}
    glBegin(GL_QUADS);glVertex3f(33,.05f,34);glVertex3f(66,.05f,34);glVertex3f(66,.05f,34.12f);glVertex3f(33,.05f,34.12f);glEnd();
    glBegin(GL_QUADS);glVertex3f(33,.05f,43);glVertex3f(66,.05f,43);glVertex3f(66,.05f,43.12f);glVertex3f(33,.05f,43.12f);glEnd();
    for(int b=0;b<=6;b++){float bx=33+b*5.5f;
        glBegin(GL_QUADS);glVertex3f(bx,.05f,51);glVertex3f(bx+.12f,.05f,51);glVertex3f(bx+.12f,.05f,60);glVertex3f(bx,.05f,60);glEnd();}
    glBegin(GL_QUADS);glVertex3f(33,.05f,51);glVertex3f(66,.05f,51);glVertex3f(66,.05f,51.12f);glVertex3f(33,.05f,51.12f);glEnd();
    glBegin(GL_QUADS);glVertex3f(33,.05f,60);glVertex3f(66,.05f,60);glVertex3f(66,.05f,60.12f);glVertex3f(33,.05f,60.12f);glEnd();
    glEnable(GL_LIGHTING);
    float cr1[][3]={{.88f,.10f,.10f},{.14f,.34f,.82f},{.12f,.62f,.18f},{.88f,.75f,.10f},{.62f,.16f,.65f},{.82f,.82f,.82f}};
    for(int c=0;c<6;c++) drawCar(35.5f+c*5.5f,37.8f,0,cr1[c][0],cr1[c][1],cr1[c][2]);
    float cr2[][3]={{.92f,.48f,.10f},{.28f,.28f,.28f},{.10f,.56f,.76f},{.76f,.22f,.22f}};
    for(int c=0;c<4;c++) drawCar(35.5f+c*5.5f,54.8f,180,cr2[c][0],cr2[c][1],cr2[c][2]);
    drawLampPost(35,46.5f);drawLampPost(50,46.5f);drawLampPost(64.5f,46.5f);
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
    /* People only in zones — NOT in the entry corridor (Z < -35) */
    drawPerson(-17.5f,-5.5f,85,0,.85f,.28f,.58f,1,0);
    drawPerson(-15.8f,-5.8f,100,0,.20f,.34f,.76f,0,0);
    float k1a=walkTime*1.8f;
    drawPerson(-8+8.9f*cosf(k1a),-9+8.9f*sinf(k1a),90-k1a*180/(float)M_PI,1,1.0f,.52f,.10f,2,walkTime*4);
    drawPerson(24,-14.5f,200,1,.90f,.26f,.26f,3,walkTime*3);
    drawPerson(26,-13.8f,215,0,.58f,.18f,.18f,0,0);
    float k3x=-12.5f+sinf(walkTime*1.2f)*.4f;
    drawPerson(k3x,-2.5f,185,1,.88f,.88f,.16f,2,walkTime*3.5f);
    drawPerson(k3x+1.5f,-2.5f,175,1,.26f,.68f,.92f,2,walkTime*3.5f+1);
    /* walking person on boulevard — only visible after roundabout */
    float cz2=fmodf(walkTime*3.5f,40)-20;
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
    /* Stage crowd */
    for(int cp=0;cp<8;cp++){
        float cpx=-6+cp*1.8f, cpz=-46;
        drawPerson(cpx,cpz,0,cp%2,
            0.3f+(cp%3)*.2f, 0.4f+(cp%4)*.1f, 0.5f+(cp%2)*.3f,
            3,walkTime*2+cp*.5f);
    }
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

    /* ══ ROAD SYSTEM  ══
       Plan:
       1. Main N-S boulevard  X=-4..+4,  from gate Z=-65 to stage Z=+42
       2. E-W cross road at   Z=-22      (service/internal ring)
       3. E-W cross road at   Z=+22      (above fountain)
       4. N-S side road (east) X=+25     (ride zone access)
       5. N-S side road (west) X=-25     (food/game access)
       6. Circular roundabout around fountain at (0,0)
       No roads go into ride/stall structures.
    ══ */

    /* 1. Main N-S boulevard — plain paved pathway, no dashes */
    drawPathway(-3,-65, -3,-16,  8.0f); /* south half → roundabout south */
    drawPathway(-3, 16, -3, 42,  8.0f); /* north half → stage */

    /* 2. E-W connector at Z=-22 (inner ring) */
    drawRoadSegment(-62,-22, -8,-22,  5); /* west arm */
    drawRoadSegment(  8,-22, 62,-22,  5); /* east arm */

    /* 3. E-W connector at Z=+22 (above fountain) */
    drawRoadSegment(-62, 22, -8, 22,  5);
    drawRoadSegment(  8, 22, 62, 22,  5);

    /* 4. N-S east side road (ride zone access) X=+28 */
    drawRoadSegment(28,-62, 28, 62,  5);

    /* 5. N-S west side road (food/game access) X=-28 */
    drawRoadSegment(-28,-62, -28, 62, 5);

    /* 6. Perimeter service road just inside wall */
    /* (short connector segments at corners) */
    drawRoadSegment(-28,-22,  28,-22, 5); /* internal E-W at Z=-22 full */
    drawRoadSegment(-28, 22,  28, 22, 5); /* internal E-W at Z=+22 full */

    /* 7. Food court access spur at Z=-38 */
    drawRoadSegment(-28,-38, -22,-38, 4);
    drawRoadSegment(-62,-38, -28,-38, 4);

    /* 8. Ride zone spur */
    drawRoadSegment(28,-38,  60,-38, 4);

    /* 9. Circular roundabout at center (0,0) inner=9, outer=15 */
    drawRoundabout(0, 0, 9.5f, 15.0f);

    /* Radial connector stubs from roundabout to main roads */
    /* South: connects boulevard to roundabout */
    drawPathway(-3,-16, -3, -9.5f, 6);
    /* North */
    drawPathway(-3, 16,  -3,  9.5f, 6);
    /* East */
    drawPathway(9.5f, 0,  16, 0, 6);
    /* West */
    drawPathway(-9.5f, 0, -16, 0, 6);

    /* ── ENTRY PLAZA PATHWAY (clean, no road markings) ── */
    drawPathway(-8,-65,  8,-55, 12.0f); /* wide gate approach paving */
    drawPathway(-7,-55,  7,-16,  9.0f); /* main entry boulevard paving */

    /* ── PERIMETER TREES (inside wall, around edges only) ── */
    float T=61;
    /* East & West perimeter trees — full length */
    for(int i=-5;i<=5;i++){
        drawTree(-T,i*11);
        drawTree( T,i*11);
    }
    /* North & South perimeter trees — skip south gate area entirely (X=-15..+15) */
    for(int i=-5;i<=5;i++){
        float tx=i*11.0f;
        if(fabsf(tx)>16.0f){          /* skip the gate corridor width */
            drawTree(tx,-T);
        }
        drawTree(tx, T);              /* north side all ok */
    }
    /* Boulevard trees flanking main N-S path — ONLY from Z=-15 northward */
    for(int i=0;i<10;i++){
        float tz=-62+i*10.5f;
        /* Only draw if north of entry zone AND not in roundabout zone */
        if(tz > -14 && (tz < -17 || tz > 17)){
            drawTree(-8.5f,tz,1.1f);
            drawTree( 5.5f,tz,1.1f);
        }
    }
    /* Trees around roundabout plaza (placed OUTSIDE the roundabout ring, not blocking) */
    for(int i=0;i<8;i++){
        float ta=i*45*(float)M_PI/180+22.5f*(float)M_PI/180;
        drawTree(cosf(ta)*21.5f,sinf(ta)*21.5f,0.95f);
    }
    /* Zone divider trees — east side only, west side removed to not block food court view */
    for(int i=-2;i<=2;i++){
        drawTree( 26,i*10,0.85f);  /* east divider only */
    }
    /* Palm trees in food court */
    drawPalmTree(-50,-44);drawPalmTree(-35,-40);drawPalmTree(-50,-25);drawPalmTree(-38,-20);

    /* ── FUNLAND ENTRANCE GATE ── */
    drawFunlandGate(0,-65);

    /* ── TICKET BOOTH — single centered booth with TICKETS heading ── */
    drawTicketBooth(0,-54,180);
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
    drawStage(0, 42);
    drawLampPost(-14,28);drawLampPost(14,28);
    drawLampPost(-14,36);drawLampPost(14,36);
    drawBench(-16,24,0);drawBench(18,24,0);
    drawBench(-16,32,0);drawBench(18,32,0);
    drawTrashBin(-12,26,0.62f,0.18f,0.88f);drawTrashBin(14,26,0.62f,0.18f,0.88f);
    drawBalloonStall(24,28,270);
    drawBalloonStall(-22,28,90);

    /* ── RIDE ZONE (east, X=30-62) ── */
    drawGiantWheel(46,-32);
    drawMerryGoRound(32,-12,merryAngle);
    drawLampPost(30,-28);drawLampPost(42,-15);drawLampPost(57,-15);drawLampPost(57,-42);
    drawBench(34,-20,90);drawBench(57,-28,0);drawBench(46,-15,0);
    drawTrashBin(36,-18,0.28f,0.48f,0.78f);drawTrashBin(57,-38,0.28f,0.48f,0.78f);

    /* ── ROLLER COASTER (NW, away from roads) ── */
    drawRollerCoaster(-40,42);

    /* ══ FOOD COURT  (west/SW, X=-62..-22, Z=-58..-10) ══
       3 distinct stall types + 6 dining tables neatly arranged
       in a 3×2 grid with umbrella colors matching zone
    ══ */
    /* Stalls along west wall, facing east */
    drawFoodStallPizza  (-55,-48, 90);   /* Red/white cone roof pizza stall */
    drawFoodStallBurger (-53,-30, 90);   /* Yellow/red flat awning burger */
    drawFoodStallIceCream(-55,-14, 90);  /* Pink dome ice cream */

    /* Small additional stalls on north end, facing south */
    drawFoodStallPizza  (-35,-14,180);
    drawFoodStallBurger (-25,-50,  0);

    /* Dining area: 3 rows × 2 columns, neatly spaced */
    /* Row 1 (Z=-42) */
    drawDiningTable(-42,-42, 0.92f,0.22f,0.22f); /* red umbrella */
    drawDiningTable(-36,-42, 0.22f,0.52f,0.92f); /* blue */
    drawDiningTable(-30,-42, 0.22f,0.78f,0.28f); /* green */
    /* Row 2 (Z=-34) */
    drawDiningTable(-42,-34, 0.92f,0.72f,0.10f); /* yellow */
    drawDiningTable(-36,-34, 0.78f,0.18f,0.82f); /* purple */
    drawDiningTable(-30,-34, 0.92f,0.42f,0.18f); /* orange */
    /* Row 3 (Z=-26) */
    drawDiningTable(-42,-26, 0.18f,0.72f,0.82f); /* cyan */
    drawDiningTable(-36,-26, 0.92f,0.28f,0.48f); /* pink */
    drawDiningTable(-30,-26, 0.58f,0.82f,0.22f); /* lime */

    /* Food court lamp posts */
    drawLampPost(-50,-52);drawLampPost(-32,-52);
    drawLampPost(-50,-14);drawLampPost(-32,-14);
    drawLampPost(-42,-20);
    /* Food court benches along boundary */
    drawBench(-60,-44,0);drawBench(-60,-30,0);
    /* Trash bins */
    drawTrashBin(-60,-38,0.88f,0.48f,0.10f);
    drawTrashBin(-32,-20,0.88f,0.48f,0.10f);

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
    gluPerspective(52,(float)w/h,0.5f,800);
    glMatrixMode(GL_MODELVIEW);
}
void keyboard(unsigned char k,int,int){
    if(k=='+'||k=='=')zoomDist-=5;if(k=='-')zoomDist+=5;
    zoomDist=std::max(15.f,std::min(250.f,zoomDist));
    if(k=='w'||k=='W')wheelSpinning=!wheelSpinning;
    if(k=='r'||k=='R')coasterRunning=!coasterRunning;
    if(k=='c'||k=='C')merryRunning=!merryRunning;
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

/* ─────────────────────────────
   INIT + MAIN
───────────────────────────── */
void init(){
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);
    glClearColor(.38f,.65f,.95f,1);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_NORMALIZE);
    setupLighting();

    /* ── ATMOSPHERIC FOG (depth effect — far = lighter/hazy) ── */
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    /* Fog color matches sky horizon (light blue-grey) */
    GLfloat fogCol[] = {0.62f, 0.78f, 0.92f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogCol);
    glFogf(GL_FOG_START,  55.0f);   /* starts fading at ~55 units away */
    glFogf(GL_FOG_END,   190.0f);   /* fully hazy at 190 units */
    glHint(GL_FOG_HINT, GL_NICEST);

    int tw,th;
    unsigned char*td=loadBMP("grass.bmp",&tw,&th);
    if(td){
        glGenTextures(1,&groundTex);
        glBindTexture(GL_TEXTURE_2D,groundTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,tw,th,GL_RGB,GL_UNSIGNED_BYTE,td);
        delete[]td;
        printf("grass.bmp loaded %dx%d\n",tw,th);
    } else {
        printf("Running without grass texture\n");
    }
}

int main(int argc,char**argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(1280,800);
    glutCreateWindow("FUNLAND 3D Amusement Park");
    init();
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