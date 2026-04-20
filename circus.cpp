/*  ================================================================
    CIRCUS TENT  —  Full 3D with Interior Camera System
    ================================================================
    CAMERA MODES  (press number keys):
      1  = Outside view (default, south-west angle)
      2  = Front entrance close-up
      3  = Inside — ring level (audience POV from seating)
      4  = Inside — center ring floor (performer's view, looking up)
      5  = Inside — aerial (trapeze rig height, looking down)
      6  = Inside — walk-through (animated orbit inside tent)
      7  = Outside orbit animation

    MANUAL CONTROLS (any mode):
      Arrow keys   = orbit / tilt
      +/-          = zoom in/out
      Mouse drag   = orbit
      Scroll wheel = zoom

    INTERACTIVE TOGGLE:
      L  = toggle spotlight animation
      F  = toggle flag/pennant sway
      A  = toggle acrobat swing
    ================================================================ */

#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif
#include <cmath>
#include <cstdio>
#include <algorithm>

/* ─────────────────────────────────
   GLOBALS
───────────────────────────────── */
int   camMode    = 1;     /* 1-7 camera presets */
float angleY     = 200.0f;
float angleX     = 22.0f;
float zoomDist   = 55.0f;

int   lastMX=-1, lastMY=-1;
bool  mouseDown=false;

/* Animation timers */
float windTime    = 0.0f;
float spotTime    = 0.0f;
float acrobatTime = 0.0f;
float orbitTime   = 0.0f;

bool  lightsOn    = true;
bool  flagsOn     = true;
bool  acrobatOn   = true;
bool  autoOrbit   = false;   /* cam mode 6/7 use this */

/* ─────────────────────────────────
   LIGHTING
───────────────────────────────── */
void setupLighting(){
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);   /* ambient sun  */
    glEnable(GL_LIGHT1);   /* fill         */
    glEnable(GL_LIGHT2);   /* ring spotlight 1 */
    glEnable(GL_LIGHT3);   /* ring spotlight 2 */
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    /* Sun */
    GLfloat a0[]={0.28f,0.26f,0.22f,1},d0[]={0.95f,0.90f,0.78f,1},p0[]={60,120,80,0};
    glLightfv(GL_LIGHT0,GL_AMBIENT,a0); glLightfv(GL_LIGHT0,GL_DIFFUSE,d0);
    glLightfv(GL_LIGHT0,GL_POSITION,p0);

    /* Sky fill */
    GLfloat d1[]={0.25f,0.32f,0.48f,1},p1[]={-40,30,-60,0};
    glLightfv(GL_LIGHT1,GL_DIFFUSE,d1); glLightfv(GL_LIGHT1,GL_POSITION,p1);

    /* Spotlights — positioned dynamically each frame */
    GLfloat sp[]={1,1,1,1};
    glLightfv(GL_LIGHT2,GL_DIFFUSE,sp); glLightfv(GL_LIGHT3,GL_DIFFUSE,sp);
    GLfloat spotexp[]={8.0f};
    glLightf(GL_LIGHT2,GL_SPOT_EXPONENT,spotexp[0]);
    glLightf(GL_LIGHT3,GL_SPOT_EXPONENT,spotexp[0]);
    glLightf(GL_LIGHT2,GL_SPOT_CUTOFF,30.0f);
    glLightf(GL_LIGHT3,GL_SPOT_CUTOFF,30.0f);
    GLfloat att[]={0.005f};
    glLightf(GL_LIGHT2,GL_QUADRATIC_ATTENUATION,att[0]);
    glLightf(GL_LIGHT3,GL_QUADRATIC_ATTENUATION,att[0]);

    GLfloat ga[]={0.15f,0.15f,0.18f,1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ga);
}

void updateSpotlights(){
    if(!lightsOn){glDisable(GL_LIGHT2);glDisable(GL_LIGHT3);return;}
    glEnable(GL_LIGHT2); glEnable(GL_LIGHT3);

    /* Spotlight 2 — sweeps around ring */
    float a2=spotTime*0.8f;
    float sp2x=sinf(a2)*5.0f, sp2z=cosf(a2)*5.0f;
    GLfloat pos2[]={sp2x, 18.5f, sp2z, 1};
    GLfloat dir2[]={-sp2x,-1,-sp2z};
    glLightfv(GL_LIGHT2,GL_POSITION,pos2);
    glLightfv(GL_LIGHT2,GL_SPOT_DIRECTION,dir2);
    GLfloat col2[]={0.5f+0.5f*sinf(spotTime*.5f),
                    0.5f+0.5f*sinf(spotTime*.5f+2.1f),
                    0.5f+0.5f*sinf(spotTime*.5f+4.2f),1};
    glLightfv(GL_LIGHT2,GL_DIFFUSE,col2);

    /* Spotlight 3 — counter-sweeps */
    float a3=-spotTime*0.6f+1.0f;
    float sp3x=sinf(a3)*4.0f, sp3z=cosf(a3)*4.0f;
    GLfloat pos3[]={sp3x,17.5f,sp3z,1};
    GLfloat dir3[]={-sp3x,-1,-sp3z};
    glLightfv(GL_LIGHT3,GL_POSITION,pos3);
    glLightfv(GL_LIGHT3,GL_SPOT_DIRECTION,dir3);
    GLfloat col3[]={0.5f+0.5f*sinf(spotTime*.4f+1.5f),
                    0.5f+0.5f*sinf(spotTime*.4f+3.6f),
                    0.5f+0.5f*sinf(spotTime*.4f+5.7f),1};
    glLightfv(GL_LIGHT3,GL_DIFFUSE,col3);
}

/* ─────────────────────────────────
   SKY
───────────────────────────────── */
void drawSky(){
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(-1,1,-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3f(0.12f,0.40f,0.90f); glVertex2f(-1,1); glVertex2f(1,1);
    glColor3f(0.55f,0.78f,1.00f); glVertex2f(1,-1); glVertex2f(-1,-1);
    glEnd();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

/* ─────────────────────────────────
   GROUND
───────────────────────────────── */
void drawGround(){
    glDisable(GL_LIGHTING);
    /* dirt field */
    glColor3f(0.42f,0.32f,0.18f);
    glBegin(GL_QUADS);
    glVertex3f(-200,0,-200); glVertex3f(200,0,-200);
    glVertex3f(200,0, 200);  glVertex3f(-200,0, 200);
    glEnd();
    /* grass patches */
    for(int i=-8;i<=8;i++) for(int j=-8;j<=8;j++){
        float gx=i*14.0f+sinf(i*2.3f)*3, gz=j*14.0f+cosf(j*1.7f)*3;
        float gr=0.25f+sinf(i*.8f+j*.6f)*.06f;
        float gg=0.52f+cosf(i*.5f+j*.9f)*.08f;
        glColor3f(gr,gg,0.14f);
        glBegin(GL_QUADS);
        glVertex3f(gx-3,0.01f,gz-3); glVertex3f(gx+3,0.01f,gz-3);
        glVertex3f(gx+3,0.01f,gz+3); glVertex3f(gx-3,0.01f,gz+3);
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

/* ─────────────────────────────────
   HELPER: draw a rope/cable between two points
───────────────────────────────── */
void drawRope(float x0,float y0,float z0,float x1,float y1,float z1,float thick){
    float dx=x1-x0,dy=y1-y0,dz=z1-z0;
    float len=sqrtf(dx*dx+dy*dy+dz*dz);
    if(len<0.01f) return;
    glDisable(GL_LIGHTING);
    glLineWidth(thick);
    glBegin(GL_LINES);
    glVertex3f(x0,y0,z0); glVertex3f(x1,y1,z1);
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

/* ─────────────────────────────────
   INTERIOR ELEMENTS
───────────────────────────────── */

/* ── Sawdust ring floor ── */
void drawRingFloor(){
    glDisable(GL_LIGHTING);
    /* outer dirt/sawdust area inside tent */
    glColor3f(0.55f,0.42f,0.22f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0,0.02f,0);
    for(int i=0;i<=48;i++){float a=i*2*(float)M_PI/48;
        glVertex3f(cosf(a)*11.0f,0.02f,sinf(a)*11.0f);}
    glEnd();
    /* performance ring — packed earth, slightly raised */
    glColor3f(0.62f,0.50f,0.30f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0,0.06f,0);
    for(int i=0;i<=48;i++){float a=i*2*(float)M_PI/48;
        glVertex3f(cosf(a)*7.2f,0.06f,sinf(a)*7.2f);}
    glEnd();
    /* ring border — white chalk line */
    glColor3f(0.92f,0.92f,0.88f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<48;i++){float a=i*2*(float)M_PI/48;
        glVertex3f(cosf(a)*7.2f,0.07f,sinf(a)*7.2f);}
    glEnd();
    glLineWidth(1.0f);
    /* radial pattern on ring */
    glColor3f(0.68f,0.56f,0.34f);
    for(int i=0;i<16;i++){
        float a=i*2*(float)M_PI/16;
        glBegin(GL_LINES);
        glVertex3f(0,0.08f,0);
        glVertex3f(cosf(a)*7.0f,0.08f,sinf(a)*7.0f);
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

/* ── Tiered audience seating (bleachers) ── */
void drawBleachers(){
    GLUquadric*q=gluNewQuadric();
    const int TIERS=6;
    const float innerR=8.0f, stepW=1.4f, stepH=0.6f;
    /* bleacher support structure */
    for(int t=0;t<TIERS;t++){
        float r0=innerR+t*stepW;
        float r1=r0+stepW;
        float y0=t*stepH;
        float y1=y0+stepH*0.8f;
        /* tread */
        glColor3f(t%2==0?0.58f:0.52f, t%2==0?0.38f:0.34f, t%2==0?0.18f:0.14f);
        const int SS=56;
        glBegin(GL_TRIANGLE_STRIP);
        for(int i=0;i<=SS;i++){
            float a=i*2*(float)M_PI/SS;
            glVertex3f(cosf(a)*r1,y0,sinf(a)*r1);
            glVertex3f(cosf(a)*r0,y0,sinf(a)*r0);
        }
        glEnd();
        /* riser */
        glColor3f(0.45f,0.30f,0.12f);
        glBegin(GL_TRIANGLE_STRIP);
        for(int i=0;i<=SS;i++){
            float a=i*2*(float)M_PI/SS;
            glVertex3f(cosf(a)*r0,y0,sinf(a)*r0);
            glVertex3f(cosf(a)*r0,y0-stepH*0.8f,sinf(a)*r0);
        }
        glEnd();
    }
    /* guard rail at top tier */
    float topR=innerR+TIERS*stepW;
    float topY=TIERS*stepH;
    glColor3f(0.72f,0.60f,0.22f);
    for(int i=0;i<24;i++){
        float a=i*2*(float)M_PI/24;
        glPushMatrix();glTranslatef(cosf(a)*topR,topY,sinf(a)*topR);
        glRotatef(-90,1,0,0);gluCylinder(q,.06,.06,.8f,6,1);glPopMatrix();
    }
    glDisable(GL_LIGHTING);
    glColor3f(0.72f,0.60f,0.22f); glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<48;i++){float a=i*2*(float)M_PI/48;
        glVertex3f(cosf(a)*topR,topY+0.8f,sinf(a)*topR);}
    glEnd();
    glLineWidth(1.0f); glEnable(GL_LIGHTING);
    gluDeleteQuadric(q);
}

/* ── Audience people on bleachers ── */
void drawAudiencePerson(float x,float y,float z,float r,float g,float b,float rotY){
    glPushMatrix();glTranslatef(x,y,z);glRotatef(rotY,0,1,0);
    GLUquadric*q=gluNewQuadric();
    /* legs */
    glColor3f(0.22f,0.22f,0.42f);
    glPushMatrix();glTranslatef(0,0.3f,0);glScalef(0.4f,0.6f,0.3f);glutSolidSphere(1,6,6);glPopMatrix();
    /* torso */
    glColor3f(r,g,b);
    glPushMatrix();glTranslatef(0,0.8f,0);glScalef(0.38f,0.5f,0.28f);glutSolidSphere(1,8,8);glPopMatrix();
    /* head */
    glColor3f(0.82f,0.65f,0.50f);
    glPushMatrix();glTranslatef(0,1.2f,0);glutSolidSphere(0.22f,8,8);glPopMatrix();
    /* cheering arm */
    glColor3f(r,g,b);
    glPushMatrix();glTranslatef(0.28f,0.95f,0);glRotatef(-120+sinf(windTime)*15,1,0,0);
    glRotatef(-90,1,0,0);gluCylinder(q,.07,.05,.52f,6,1);glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

void drawAudience(){
    float shirtC[][3]={
        {0.88f,0.18f,0.18f},{0.18f,0.52f,0.88f},{0.18f,0.78f,0.28f},
        {0.88f,0.72f,0.10f},{0.72f,0.18f,0.82f},{0.88f,0.45f,0.12f},
        {0.18f,0.72f,0.82f},{0.88f,0.28f,0.52f},{0.55f,0.78f,0.18f},
        {0.28f,0.28f,0.78f},{0.88f,0.88f,0.18f},{0.18f,0.55f,0.55f}
    };
    /* Place people on tiers 1-5 */
    for(int tier=1;tier<=5;tier++){
        float r=8.0f+tier*1.4f+0.5f;
        float y=tier*0.6f+0.02f;
        int count=6+tier*4;
        for(int i=0;i<count;i++){
            float a=i*2*(float)M_PI/count+tier*0.15f;
            float px=cosf(a)*r, pz=sinf(a)*r;
            float rotY=-a*180/(float)M_PI+90; /* face center */
            int ci=(i+tier*3)%12;
            drawAudiencePerson(px,y,pz,shirtC[ci][0],shirtC[ci][1],shirtC[ci][2],rotY);
        }
    }
}

/* ── Central rig: 3 king poles + horizontal truss ── */
void drawRigStructure(){
    GLUquadric*q=gluNewQuadric();
    const float BH=2.8f;
    const float POLE_H=22.0f;   /* top of main pole above ground */

    /* 3 inner king poles at 120° */
    float poleR=6.5f;
    for(int p=0;p<3;p++){
        float pa=p*120*(float)M_PI/180;
        float px=cosf(pa)*poleR, pz=sinf(pa)*poleR;
        /* pole */
        glColor3f(0.28f,0.25f,0.20f);
        glPushMatrix();glTranslatef(px,0,pz);glRotatef(-90,1,0,0);
        gluCylinder(q,0.28f,0.22f,POLE_H,10,2);glPopMatrix();
        /* reinforced base collar */
        glColor3f(0.42f,0.38f,0.28f);
        glPushMatrix();glTranslatef(px,0,pz);glRotatef(-90,1,0,0);
        gluCylinder(q,0.45f,0.35f,1.2f,10,1);glPopMatrix();
        /* top cap */
        glColor3f(0.88f,0.72f,0.10f);
        glPushMatrix();glTranslatef(px,POLE_H+0.4f,pz);glutSolidSphere(0.40f,10,10);glPopMatrix();
    }

    /* Horizontal truss ring at trapeze height (y=16) */
    const float TR_Y=16.0f;
    glColor3f(0.32f,0.30f,0.25f);
    const int RS=32;
    for(int s=0;s<RS;s++){
        float a0=s*2*(float)M_PI/RS, a1=(s+1)*2*(float)M_PI/RS;
        float r2=5.5f;
        /* outer ring beam */
        glBegin(GL_QUADS);
        glVertex3f(cosf(a0)*r2,TR_Y-0.18f,sinf(a0)*r2);
        glVertex3f(cosf(a1)*r2,TR_Y-0.18f,sinf(a1)*r2);
        glVertex3f(cosf(a1)*r2,TR_Y+0.18f,sinf(a1)*r2);
        glVertex3f(cosf(a0)*r2,TR_Y+0.18f,sinf(a0)*r2);
        glEnd();
    }
    /* Radial spokes of truss to center */
    glColor3f(0.35f,0.32f,0.28f);
    for(int sp=0;sp<8;sp++){
        float sa=sp*45*(float)M_PI/180;
        drawRope(0,TR_Y,0, cosf(sa)*5.5f,TR_Y,sinf(sa)*5.5f, 2.0f);
    }
    /* Suspension cables from pole tops to truss ring */
    for(int p=0;p<3;p++){
        float pa=p*120*(float)M_PI/180;
        float px=cosf(pa)*poleR,pz=sinf(pa)*poleR;
        glDisable(GL_LIGHTING);
        glColor3f(0.42f,0.38f,0.28f); glLineWidth(1.8f);
        glBegin(GL_LINES);
        glVertex3f(px,POLE_H,pz);
        glVertex3f(cosf(pa)*5.5f,TR_Y,sinf(pa)*5.5f);
        glEnd();
        /* also to center top */
        glBegin(GL_LINES);
        glVertex3f(px,POLE_H,pz);
        glVertex3f(0,TR_Y,0);
        glEnd();
        glLineWidth(1.0f); glEnable(GL_LIGHTING);
    }

    /* ── Trapeze bars hanging from truss ── */
    for(int t=0;t<2;t++){
        float ta=t*180*(float)M_PI/180;
        float tx=cosf(ta)*3.5f, tz=sinf(ta)*3.5f;
        /* suspension ropes */
        glDisable(GL_LIGHTING);
        glColor3f(0.65f,0.55f,0.30f); glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex3f(tx-0.9f,TR_Y,tz); glVertex3f(tx-0.9f,TR_Y-3.5f,tz);
        glVertex3f(tx+0.9f,TR_Y,tz); glVertex3f(tx+0.9f,TR_Y-3.5f,tz);
        glEnd();
        glLineWidth(1.0f); glEnable(GL_LIGHTING);
        /* bar */
        glColor3f(0.28f,0.25f,0.20f);
        glPushMatrix();glTranslatef(tx,TR_Y-3.5f,tz);glRotatef(90,0,1,0);
        gluCylinder(q,0.07f,0.07f,1.8f,8,1);glPopMatrix();
    }

    /* ── Static trapeze nets below truss ── */
    glDisable(GL_LIGHTING);
    glColor3f(0.58f,0.50f,0.35f); glLineWidth(1.2f);
    /* horizontal grid */
    for(int i=-4;i<=4;i++){
        float fi=(float)i*1.1f;
        glBegin(GL_LINE_STRIP);
        glVertex3f(-4.5f,TR_Y-5.5f,fi); glVertex3f(4.5f,TR_Y-5.5f,fi);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(fi,TR_Y-5.5f,-4.5f); glVertex3f(fi,TR_Y-5.5f,4.5f);
        glEnd();
    }
    glLineWidth(1.0f); glEnable(GL_LIGHTING);

    gluDeleteQuadric(q);
}

/* ── Animated acrobat on trapeze ── */
void drawAcrobat(float t){
    /* swings on trapeze bar 0 */
    float swing=sinf(t*1.4f)*0.8f;   /* swing angle in rad */
    const float TR_Y=16.0f;
    float bx=cosf(0)*3.5f, bz=sinf(0)*3.5f;   /* bar 0 position */

    glPushMatrix();
    /* bar hangs at TR_Y-3.5, acrobat holds it and swings */
    glTranslatef(bx,TR_Y-3.5f,bz);
    glRotatef(swing*180/(float)M_PI,0,0,1);

    GLUquadric*q=gluNewQuadric();
    /* arms (holding bar) */
    glColor3f(0.88f,0.55f,0.22f);  /* leotard */
    glPushMatrix();glTranslatef(-0.9f,-0.2f,0);glScalef(0.18f,0.22f,0.18f);glutSolidSphere(1,6,6);glPopMatrix();
    glPushMatrix();glTranslatef( 0.9f,-0.2f,0);glScalef(0.18f,0.22f,0.18f);glutSolidSphere(1,6,6);glPopMatrix();
    /* torso */
    glColor3f(0.88f,0.30f,0.52f);
    glPushMatrix();glTranslatef(0,-0.8f,0);glScalef(0.32f,0.52f,0.28f);glutSolidSphere(1,10,10);glPopMatrix();
    /* legs */
    glColor3f(0.88f,0.30f,0.52f);
    float legSwing=sinf(t*1.4f+0.4f)*0.6f;
    for(int s=-1;s<=1;s+=2){
        glPushMatrix();glTranslatef(s*0.16f,-1.38f,0);
        glRotatef(s*legSwing*30,1,0,0);
        glScalef(0.18f,0.55f,0.18f);glutSolidSphere(1,8,8);glPopMatrix();
    }
    /* head */
    glColor3f(0.82f,0.62f,0.48f);
    glPushMatrix();glTranslatef(0,-0.28f,0);glutSolidSphere(0.24f,10,10);glPopMatrix();
    /* hair */
    glColor3f(0.25f,0.18f,0.08f);
    glPushMatrix();glTranslatef(0,-0.18f,0);glScalef(1,0.55f,1);glutSolidSphere(0.25f,8,8);glPopMatrix();

    gluDeleteQuadric(q);
    glPopMatrix();
}

/* ── Clown on ring floor ── */
void drawClown(float t){
    float cx=cosf(t*0.5f)*3.5f, cz=sinf(t*0.5f)*3.5f;
    float cy=0.08f;
    float face=-(t*0.5f)*180/(float)M_PI+90;

    glPushMatrix();glTranslatef(cx,cy,cz);glRotatef(face,0,1,0);
    GLUquadric*q=gluNewQuadric();

    /* big shoes */
    glColor3f(0.88f,0.12f,0.12f);
    glPushMatrix();glTranslatef(-0.22f,0,-0.5f);glScalef(0.22f,0.15f,0.62f);glutSolidSphere(1,8,8);glPopMatrix();
    glPushMatrix();glTranslatef( 0.22f,0,-0.5f);glScalef(0.22f,0.15f,0.62f);glutSolidSphere(1,8,8);glPopMatrix();
    /* baggy pants */
    glColor3f(0.10f,0.28f,0.88f);
    glPushMatrix();glTranslatef(0,0.65f,0);glScalef(0.62f,0.75f,0.52f);glutSolidSphere(1,10,10);glPopMatrix();
    /* big shirt */
    glColor3f(0.92f,0.72f,0.08f);
    glPushMatrix();glTranslatef(0,1.3f,0);glScalef(0.58f,0.62f,0.48f);glutSolidSphere(1,10,10);glPopMatrix();
    /* ruffled collar */
    glColor3f(0.92f,0.22f,0.78f);
    for(int r=0;r<8;r++){
        float ra=r*45*(float)M_PI/180;
        glPushMatrix();glTranslatef(cosf(ra)*0.42f,1.62f,sinf(ra)*0.42f);
        glutSolidSphere(0.18f,6,6);glPopMatrix();
    }
    /* head */
    glColor3f(0.88f,0.72f,0.60f);
    glPushMatrix();glTranslatef(0,1.95f,0);glutSolidSphere(0.32f,12,12);glPopMatrix();
    /* red nose */
    glColor3f(0.95f,0.10f,0.10f);
    glPushMatrix();glTranslatef(0,1.95f,0.30f);glutSolidSphere(0.10f,8,8);glPopMatrix();
    /* big hat */
    glColor3f(0.12f,0.08f,0.35f);
    glPushMatrix();glTranslatef(0,2.28f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,0.32f,0.28f,0.85f,12,1);glPopMatrix();
    glColor3f(0.18f,0.10f,0.45f);
    glPushMatrix();glTranslatef(0,2.28f,0);glRotatef(-90,1,0,0);
    gluDisk(q,0,0.52f,12,1);glPopMatrix();
    /* flower on hat */
    glColor3f(0.88f,0.22f,0.72f);
    glPushMatrix();glTranslatef(0,3.14f,0.28f);glutSolidSphere(0.14f,8,8);glPopMatrix();
    /* arms with prop (honk honk horn) */
    glColor3f(0.92f,0.72f,0.08f);
    glPushMatrix();glTranslatef(0.55f,1.32f,0);glRotatef(-70+sinf(t*2)*20,1,0,0);
    glRotatef(-90,1,0,0);gluCylinder(q,0.08f,0.07f,0.72f,6,1);
    /* horn */
    glColor3f(0.22f,0.22f,0.25f);
    glTranslatef(0,0,0.72f);gluCylinder(q,0.07f,0.14f,0.28f,8,1);
    glColor3f(0.92f,0.18f,0.18f);
    glTranslatef(0,0,0.28f);glutSolidSphere(0.16f,8,8);
    glPopMatrix();

    gluDeleteQuadric(q);glPopMatrix();
}

/* ── Lion tamer + lion ── */
void drawLionTamer(float t){
    float lx=cosf(t*0.2f+1.5f)*4.0f, lz=sinf(t*0.2f+1.5f)*4.0f;
    glPushMatrix();glTranslatef(lx,0.08f,lz);
    GLUquadric*q=gluNewQuadric();
    /* boots */
    glColor3f(0.25f,0.18f,0.08f);
    for(int s=-1;s<=1;s+=2){
        glPushMatrix();glTranslatef(s*0.18f,0,0);glRotatef(-90,1,0,0);
        gluCylinder(q,0.12f,0.10f,0.75f,8,1);glPopMatrix();
    }
    /* uniform trousers */
    glColor3f(0.12f,0.18f,0.55f);
    glPushMatrix();glTranslatef(0,0.72f,0);glScalef(0.42f,0.65f,0.35f);glutSolidSphere(1,8,8);glPopMatrix();
    /* red jacket */
    glColor3f(0.85f,0.12f,0.12f);
    glPushMatrix();glTranslatef(0,1.32f,0);glScalef(0.45f,0.60f,0.38f);glutSolidSphere(1,10,10);glPopMatrix();
    /* gold epaulettes */
    glColor3f(0.92f,0.78f,0.10f);
    glPushMatrix();glTranslatef(-0.42f,1.50f,0);glutSolidSphere(0.14f,8,8);glPopMatrix();
    glPushMatrix();glTranslatef( 0.42f,1.50f,0);glutSolidSphere(0.14f,8,8);glPopMatrix();
    /* head */
    glColor3f(0.82f,0.65f,0.48f);
    glPushMatrix();glTranslatef(0,1.88f,0);glutSolidSphere(0.24f,10,10);glPopMatrix();
    /* top hat */
    glColor3f(0.10f,0.08f,0.08f);
    glPushMatrix();glTranslatef(0,2.12f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,0.24f,0.22f,0.62f,12,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,2.12f,0);glRotatef(-90,1,0,0);
    gluDisk(q,0,0.38f,12,1);glPopMatrix();
    /* whip */
    glDisable(GL_LIGHTING);
    glColor3f(0.35f,0.22f,0.10f); glLineWidth(2.0f);
    float wa=sinf(t*1.8f)*0.6f;
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<=8;i++){
        float ft=(float)i/8;
        glVertex3f(lx+0.55f+ft*1.8f,0.08f+1.45f-ft*0.3f+sinf(wa+ft*2)*0.4f*ft,lz);
    }
    glEnd();
    glLineWidth(1.0f); glEnable(GL_LIGHTING);
    gluDeleteQuadric(q);glPopMatrix();

    /* ── Lion ── */
    float lionA=t*0.2f+1.5f+(float)M_PI*0.35f;
    float lionX=cosf(lionA)*3.0f, lionZ=sinf(lionA)*3.0f;
    float lionFace=(-lionA)*180/(float)M_PI;
    glPushMatrix();glTranslatef(lionX,0.06f,lionZ);glRotatef(lionFace,0,1,0);
    GLUquadric*q2=gluNewQuadric();
    /* body */
    glColor3f(0.78f,0.58f,0.22f);
    glPushMatrix();glScalef(0.7f,0.5f,1.2f);glutSolidSphere(1,10,10);glPopMatrix();
    /* mane */
    glColor3f(0.55f,0.35f,0.10f);
    glPushMatrix();glTranslatef(0,0.25f,0.95f);glutSolidSphere(0.65f,12,12);glPopMatrix();
    /* head */
    glColor3f(0.78f,0.58f,0.22f);
    glPushMatrix();glTranslatef(0,0.22f,1.35f);glutSolidSphere(0.42f,10,10);glPopMatrix();
    /* ears */
    glColor3f(0.68f,0.48f,0.18f);
    glPushMatrix();glTranslatef(-0.28f,0.58f,1.22f);glutSolidSphere(0.14f,6,6);glPopMatrix();
    glPushMatrix();glTranslatef( 0.28f,0.58f,1.22f);glutSolidSphere(0.14f,6,6);glPopMatrix();
    /* tail */
    glDisable(GL_LIGHTING);
    glColor3f(0.72f,0.52f,0.18f); glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<=6;i++){float ft=(float)i/6;
        glVertex3f(lionX,0.06f+0.3f+ft*0.4f,lionZ-0.9f-ft*1.0f+sinf(t+ft*2)*0.3f);}
    glEnd();
    glLineWidth(1.0f); glEnable(GL_LIGHTING);
    gluDeleteQuadric(q2);glPopMatrix();
}

/* ── Juggler ── */
void drawJuggler(float t){
    glPushMatrix();glTranslatef(-3.5f,0.08f,3.0f);
    GLUquadric*q=gluNewQuadric();
    /* body */
    glColor3f(0.28f,0.62f,0.18f);
    glPushMatrix();glTranslatef(0,0.75f,0);glScalef(0.38f,0.65f,0.32f);glutSolidSphere(1,8,8);glPopMatrix();
    glColor3f(0.78f,0.62f,0.28f);
    glPushMatrix();glTranslatef(0,1.32f,0);glScalef(0.38f,0.56f,0.32f);glutSolidSphere(1,8,8);glPopMatrix();
    /* head */
    glColor3f(0.82f,0.65f,0.50f);
    glPushMatrix();glTranslatef(0,1.88f,0);glutSolidSphere(0.24f,10,10);glPopMatrix();
    /* juggling balls */
    float ballC[][3]={{1,.2f,.2f},{.2f,.8f,.2f},{.2f,.2f,1},{1,.9f,.1f},{.9f,.2f,.8f}};
    for(int b=0;b<5;b++){
        float ba=b*72*(float)M_PI/180+t*2.2f;
        float bh=1.8f+sinf(ba)*1.4f;
        float bx=cosf(ba)*0.55f;
        glColor3f(ballC[b][0],ballC[b][1],ballC[b][2]);
        glPushMatrix();glTranslatef(bx,bh,-3.0f);glutSolidSphere(0.14f,8,8);glPopMatrix();
    }
    gluDeleteQuadric(q);glPopMatrix();
}

/* ── Spotlights visible beams (cones from rig) ── */
void drawSpotlightBeams(){
    if(!lightsOn) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    GLUquadric*q=gluNewQuadric();
    const float TR_Y=16.0f;

    /* spotlight 1 beam — sweeping */
    float a2=spotTime*0.8f;
    float sp2x=sinf(a2)*5.0f, sp2z=cosf(a2)*5.0f;
    float lr=0.5f+0.5f*sinf(spotTime*.5f);
    float lg=0.5f+0.5f*sinf(spotTime*.5f+2.1f);
    float lb=0.5f+0.5f*sinf(spotTime*.5f+4.2f);
    glColor4f(lr,lg,lb,0.10f);
    glPushMatrix();
    glTranslatef(sp2x,TR_Y,sp2z);
    /* orient cone down toward ring */
    float dx=-sp2x,dz=-sp2z,dy=-TR_Y;
    float ang=atan2f(sqrtf(dx*dx+dz*dz),fabsf(dy))*180/(float)M_PI;
    glRotatef(atan2f(dz,dx)*180/(float)M_PI,0,1,0);
    glRotatef(90+ang,0,0,1);
    gluCylinder(q,0.25f,3.0f,TR_Y+1,12,4);
    glPopMatrix();

    /* spotlight 2 beam */
    float a3=-spotTime*0.6f+1.0f;
    float sp3x=sinf(a3)*4.0f, sp3z=cosf(a3)*4.0f;
    float lr2=0.5f+0.5f*sinf(spotTime*.4f+1.5f);
    float lg2=0.5f+0.5f*sinf(spotTime*.4f+3.6f);
    float lb2=0.5f+0.5f*sinf(spotTime*.4f+5.7f);
    glColor4f(lr2,lg2,lb2,0.10f);
    glPushMatrix();
    glTranslatef(sp3x,TR_Y-1,sp3z);
    float dx2=-sp3x,dz2=-sp3z,dy2=-(TR_Y-1);
    float ang2=atan2f(sqrtf(dx2*dx2+dz2*dz2),fabsf(dy2))*180/(float)M_PI;
    glRotatef(atan2f(dz2,dx2)*180/(float)M_PI,0,1,0);
    glRotatef(90+ang2,0,0,1);
    gluCylinder(q,0.20f,2.5f,TR_Y,12,4);
    glPopMatrix();

    gluDeleteQuadric(q);
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);
}

/* ── Ring light fixtures on truss ring ── */
void drawRingLights(){
    const float TR_Y=16.0f;
    GLUquadric*q=gluNewQuadric();
    const int NL=12;
    for(int l=0;l<NL;l++){
        float la=l*2*(float)M_PI/NL;
        float lx=cosf(la)*5.2f, lz=sinf(la)*5.2f;
        /* fixture housing */
        glColor3f(0.22f,0.22f,0.25f);
        glPushMatrix();glTranslatef(lx,TR_Y-0.5f,lz);
        glScalef(0.35f,0.35f,0.35f);glutSolidSphere(1,6,6);glPopMatrix();
        /* glowing bulb — colour chase */
        float hue=spotTime*0.8f+l*0.52f;
        if(lightsOn){
            glColor3f(0.5f+0.5f*sinf(hue),0.5f+0.5f*sinf(hue+2.1f),0.5f+0.5f*sinf(hue+4.2f));
        } else {
            glColor3f(0.35f,0.32f,0.28f);
        }
        glPushMatrix();glTranslatef(lx,TR_Y-0.65f,lz);glutSolidSphere(0.18f,6,6);glPopMatrix();
        /* wire down to truss */
        glDisable(GL_LIGHTING);
        glColor3f(0.28f,0.28f,0.28f);
        glBegin(GL_LINES);
        glVertex3f(lx,TR_Y,lz); glVertex3f(lx,TR_Y-0.5f,lz);
        glEnd();
        glEnable(GL_LIGHTING);
    }
    gluDeleteQuadric(q);
}

/* ── Decorative bunting strings inside tent ── */
void drawBunting(){
    glDisable(GL_LIGHTING);
    float buntC[][3]={{1,.2f,.2f},{1,.9f,.1f},{.2f,.8f,.2f},{.2f,.5f,1},{.8f,.2f,.8f}};
    /* 4 diagonal bunting lines from top pole to base ring */
    for(int d=0;d<4;d++){
        float da=d*90*(float)M_PI/180+22.5f*(float)M_PI/180;
        float endX=cosf(da)*10.5f, endZ=sinf(da)*10.5f;
        float startH=12.5f;   /* mid-height of pole */
        const int NB=14;
        for(int b=0;b<NB;b++){
            float t=(float)b/NB, t1=(float)(b+1)/NB;
            /* lerp position with catenary sag */
            float bx=t*endX, bz=t*endZ;
            float bx1=t1*endX, bz1=t1*endZ;
            float by=startH+(2.8f-startH)*t - sinf(t*(float)M_PI)*1.5f;
            float by1=startH+(2.8f-startH)*t1 - sinf(t1*(float)M_PI)*1.5f;
            int ci=b%5;
            glColor3f(buntC[ci][0],buntC[ci][1],buntC[ci][2]);
            glBegin(GL_TRIANGLES);
            glVertex3f(bx,by,bz);
            glVertex3f((bx+bx1)*.5f+sinf(da)*0.2f,by-0.5f,(bz+bz1)*.5f+cosf(da)*0.2f);
            glVertex3f(bx1,by1,bz1);
            glEnd();
            /* string */
            glColor3f(0.30f,0.28f,0.22f);
            glBegin(GL_LINES);glVertex3f(bx,by,bz);glVertex3f(bx1,by1,bz1);glEnd();
        }
    }
    glEnable(GL_LIGHTING);
}

/* ─────────────────────────────────
   MAIN CIRCUS TENT  (bigger: TR=16, TH=13)
───────────────────────────────── */
void drawCircusTent(float cx,float cz){
    glPushMatrix();
    glTranslatef(cx,0,cz);
    GLUquadric*q=gluNewQuadric();

    const int   SEGS   = 56;
    const float TR     = 16.0f;    /* tent base radius  (was 11.5) */
    const float TH     = 14.5f;   /* tent height       (was 9.5)  */
    const float BR     = 17.0f;   /* base wall radius  (was 12.2) */
    const float BH     = 3.2f;   /* base wall height  */
    const int   PANELS = 24;      /* roof panels       */
    const float ENT_W  = 5.0f;   /* entrance half-width */

    /* ── Stamped-earth ground disc ── */
    glColor3f(0.58f,0.50f,0.40f);
    glPushMatrix();glRotatef(-90,1,0,0);gluDisk(q,0,BR+1.0f,SEGS,4);glPopMatrix();

    /* ── Decorative paving ring at entrance ── */
    glDisable(GL_LIGHTING);
    glColor3f(0.72f,0.65f,0.52f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(BR+4,0.01f,0);
    for(int i=-4;i<=4;i++){
        float a=i*(float)M_PI/24;
        glVertex3f(cosf(a)*(BR+5),0.01f,sinf(a)*(BR+5));
    }
    glEnd();
    glEnable(GL_LIGHTING);

    /* ── Cylindrical base wall — alternating wide stripes ── */
    /* skip the entrance sector (angle ±22° around +X) */
    float entAngle=22.0f*(float)M_PI/180.0f;
    for(int s=0;s<SEGS;s++){
        float a0=s      *2*(float)M_PI/SEGS;
        float a1=(s+1)  *2*(float)M_PI/SEGS;
        float aMid=(a0+a1)*0.5f;
        /* skip entrance gap */
        if(aMid< entAngle || aMid > 2*(float)M_PI-entAngle) continue;
        glColor3f(s%2==0?0.90f:0.98f, s%2==0?0.10f:0.98f, s%2==0?0.10f:0.98f);
        glBegin(GL_QUADS);
        glVertex3f(BR*cosf(a0),0,   BR*sinf(a0));
        glVertex3f(BR*cosf(a1),0,   BR*sinf(a1));
        glVertex3f(BR*cosf(a1),BH,  BR*sinf(a1));
        glVertex3f(BR*cosf(a0),BH,  BR*sinf(a0));
        glEnd();
    }
    /* base wall top ring */
    glColor3f(0.82f,0.75f,0.20f);
    glPushMatrix();glTranslatef(0,BH,0);glRotatef(-90,1,0,0);
    gluCylinder(q,BR,BR,0.28f,SEGS,1);glPopMatrix();

    /* ── Main conical roof with fat red/white panels ── */
    float panelC[2][3]={{0.90f,0.10f,0.10f},{0.98f,0.98f,0.98f}};
    for(int p=0;p<PANELS;p++){
        float a0=p      *2*(float)M_PI/PANELS;
        float a1=(p+1)  *2*(float)M_PI/PANELS;
        glColor3f(panelC[p%2][0],panelC[p%2][1],panelC[p%2][2]);
        glBegin(GL_TRIANGLES);
        glVertex3f(TR*cosf(a0),BH,    TR*sinf(a0));
        glVertex3f(TR*cosf(a1),BH,    TR*sinf(a1));
        glVertex3f(0,           BH+TH,0);
        glEnd();
        /* inner/back face */
        glColor3f(panelC[p%2][0]*0.55f,panelC[p%2][1]*0.55f,panelC[p%2][2]*0.55f);
        glBegin(GL_TRIANGLES);
        glVertex3f(TR*cosf(a1),BH,    TR*sinf(a1));
        glVertex3f(TR*cosf(a0),BH,    TR*sinf(a0));
        glVertex3f(0,           BH+TH,0);
        glEnd();
    }

    /* ── Outer valance skirt ── */
    for(int p=0;p<PANELS*2;p++){
        float a0=p      *2*(float)M_PI/(PANELS*2);
        float a1=(p+1)  *2*(float)M_PI/(PANELS*2);
        float am=(a0+a1)*0.5f;
        float sway=(flagsOn?sinf(windTime*1.2f+p*0.5f)*0.12f:0);
        glColor3f(p%2==0?0.90f:0.98f, p%2==0?0.10f:0.98f, p%2==0?0.10f:0.98f);
        float rI=TR-0.6f, rO=TR+1.5f;
        glBegin(GL_TRIANGLES);
        glVertex3f(rI*cosf(a0),BH,            rI*sinf(a0));
        glVertex3f(rI*cosf(a1),BH,            rI*sinf(a1));
        glVertex3f(rO*cosf(am)*(1+sway*0.05f),BH-1.3f,rO*sinf(am)*(1+sway*0.05f));
        glEnd();
    }

    /* ── Secondary mini-tent topper ── */
    const float MT_R=3.8f, MT_H=4.5f;
    for(int p=0;p<12;p++){
        float a0=p      *2*(float)M_PI/12;
        float a1=(p+1)  *2*(float)M_PI/12;
        glColor3f(p%2==0?0.92f:0.98f, p%2==0?0.18f:0.98f, p%2==0?0.18f:0.98f);
        glBegin(GL_TRIANGLES);
        glVertex3f(MT_R*cosf(a0),BH+TH-1.0f,MT_R*sinf(a0));
        glVertex3f(MT_R*cosf(a1),BH+TH-1.0f,MT_R*sinf(a1));
        glVertex3f(0,BH+TH+MT_H-1.0f,0);
        glEnd();
        /* inner */
        glColor3f(p%2==0?0.55f:0.62f, p%2==0?0.08f:0.62f, p%2==0?0.08f:0.62f);
        glBegin(GL_TRIANGLES);
        glVertex3f(MT_R*cosf(a1),BH+TH-1.0f,MT_R*sinf(a1));
        glVertex3f(MT_R*cosf(a0),BH+TH-1.0f,MT_R*sinf(a0));
        glVertex3f(0,BH+TH+MT_H-1.0f,0);
        glEnd();
    }
    /* mini-topper valance */
    for(int p=0;p<24;p++){
        float a0=p*2*(float)M_PI/24, a1=(p+1)*2*(float)M_PI/24, am=(a0+a1)*0.5f;
        glColor3f(p%2==0?0.90f:0.98f, p%2==0?0.10f:0.98f, p%2==0?0.10f:0.98f);
        glBegin(GL_TRIANGLES);
        glVertex3f((MT_R-.3f)*cosf(a0),BH+TH-1.0f,(MT_R-.3f)*sinf(a0));
        glVertex3f((MT_R-.3f)*cosf(a1),BH+TH-1.0f,(MT_R-.3f)*sinf(a1));
        glVertex3f((MT_R+1.0f)*cosf(am),BH+TH-2.0f,(MT_R+1.0f)*sinf(am));
        glEnd();
    }

    /* ── Centre main pole ── */
    glColor3f(0.30f,0.28f,0.24f);
    glPushMatrix();glRotatef(-90,1,0,0);
    gluCylinder(q,0.48f,0.38f,BH+TH+MT_H+0.5f,14,3);glPopMatrix();
    /* pole stripe */
    glDisable(GL_LIGHTING);
    glColor3f(0.88f,0.20f,0.20f); glLineWidth(3.0f);
    for(int st=0;st<20;st++){
        float t=(float)st/20, t1=(float)(st+1)/20;
        float y0=t*(BH+TH+MT_H), y1=t1*(BH+TH+MT_H);
        float r2=0.48f-(0.48f-0.38f)*t;
        float a=t*720*(float)M_PI/180;
        glBegin(GL_LINES);
        glVertex3f(cosf(a)*r2,y0,sinf(a)*r2);
        glVertex3f(cosf(a+0.6f)*(r2-.01f),y1,sinf(a+0.6f)*(r2-.01f));
        glEnd();
    }
    glLineWidth(1.0f); glEnable(GL_LIGHTING);

    /* ── 4 outer king poles ── */
    for(int op=0;op<4;op++){
        float oa=op*90*(float)M_PI/180+45*(float)M_PI/180;
        float opH=BH+5.5f;
        glColor3f(0.30f,0.26f,0.20f);
        glPushMatrix();glTranslatef(cosf(oa)*9.5f,0,sinf(oa)*9.5f);
        glRotatef(-90,1,0,0);gluCylinder(q,0.28f,0.22f,opH,10,1);glPopMatrix();
        /* small flag on outer pole */
        if(flagsOn){
            float sway=sinf(windTime*1.4f+op)*0.22f;
            float flagC[][3]={{0.92f,0.18f,0.18f},{0.18f,0.52f,0.92f},{0.18f,0.82f,0.22f},{0.92f,0.72f,0.10f}};
            glColor3f(flagC[op][0],flagC[op][1],flagC[op][2]);
            float bx=cosf(oa)*9.5f, bz=sinf(oa)*9.5f;
            glBegin(GL_TRIANGLES);
            glVertex3f(bx,opH,bz);
            glVertex3f(bx+cosf(oa)*2.2f+sway,opH-0.6f,bz+sinf(oa)*2.2f);
            glVertex3f(bx,opH-1.5f,bz);
            glEnd();
        }
    }

    /* ── Golden star apex ── */
    glColor3f(0.95f,0.88f,0.12f);
    glPushMatrix();glTranslatef(0,BH+TH+MT_H+0.7f,0);glutSolidSphere(0.65f,16,16);glPopMatrix();
    for(int i=0;i<8;i++){
        float ba=i*45*(float)M_PI/180;
        glColor3f(0.98f,0.85f,0.08f);
        glPushMatrix();glTranslatef(cosf(ba)*0.95f,BH+TH+MT_H+0.7f,sinf(ba)*0.95f);
        glutSolidSphere(0.22f,8,8);glPopMatrix();
    }

    /* ── ENTRANCE ARCH (big, faces +X / east) ── */
    float entA=0.0f;
    float archX=cosf(entA)*BR, archZ=sinf(entA)*BR;
    /* arch posts — thick decorated columns */
    for(int s=-1;s<=1;s+=2){
        float px=archX + s*sinf(entA)*ENT_W;
        float pz=archZ - s*cosf(entA)*ENT_W;
        /* column */
        glColor3f(0.92f,0.82f,0.62f);
        glPushMatrix();glTranslatef(px,0,pz);glRotatef(-90,1,0,0);
        gluCylinder(q,0.55f,0.48f,6.5f,14,2);glPopMatrix();
        /* capital */
        glColor3f(0.88f,0.72f,0.38f);
        glPushMatrix();glTranslatef(px,6.5f,pz);glutSolidSphere(0.65f,12,12);glPopMatrix();
        /* spiral stripe on column */
        glDisable(GL_LIGHTING);
        glColor3f(0.88f,0.18f,0.18f); glLineWidth(3.0f);
        for(int st=0;st<18;st++){
            float t=(float)st/18*6.5f, t1=(float)(st+1)/18*6.5f;
            float a=st*360.f/18*(float)M_PI/180;
            float r2=0.55f;
            glBegin(GL_LINES);
            glVertex3f(px+cosf(a)*r2,t,pz+sinf(a)*r2);
            glVertex3f(px+cosf(a+0.8f)*r2,t1,pz+sinf(a+0.8f)*r2);
            glEnd();
        }
        glLineWidth(1.0f); glEnable(GL_LIGHTING);
    }
    /* arch top beam */
    glColor3f(0.88f,0.78f,0.45f);
    glPushMatrix();glTranslatef(archX,6.5f,archZ);
    glRotatef(atan2f(sinf(entA),cosf(entA))*180/(float)M_PI+90,0,1,0);
    GLUquadric*qa=gluNewQuadric();
    gluCylinder(qa,0.22f,0.22f,ENT_W*2,10,1);
    gluDeleteQuadric(qa);glPopMatrix();
    /* rainbow arch over entrance */
    float archR=ENT_W+0.8f, archT=0.58f;
    float archCols[][3]={{1,0,0},{1,.5f,0},{1,1,0},{0,.8f,0},{0,.5f,1},{.3f,0,1},{.8f,0,.8f}};
    for(int as=0;as<14;as++){
        float aa0=as*(float)M_PI/14, aa1=(as+1)*(float)M_PI/14;
        glColor3f(archCols[as%7][0],archCols[as%7][1],archCols[as%7][2]);
        glBegin(GL_QUADS);
        glVertex3f(archX+(archR-archT)*cosf(aa0),6.5f+(archR-archT)*sinf(aa0),archZ-0.3f);
        glVertex3f(archX+archR*cosf(aa0),         6.5f+archR*sinf(aa0),         archZ-0.3f);
        glVertex3f(archX+archR*cosf(aa1),         6.5f+archR*sinf(aa1),         archZ-0.3f);
        glVertex3f(archX+(archR-archT)*cosf(aa1),6.5f+(archR-archT)*sinf(aa1),archZ-0.3f);
        glEnd();
        glBegin(GL_QUADS);
        glVertex3f(archX+(archR-archT)*cosf(aa0),6.5f+(archR-archT)*sinf(aa0),archZ+0.3f);
        glVertex3f(archX+archR*cosf(aa0),         6.5f+archR*sinf(aa0),         archZ+0.3f);
        glVertex3f(archX+archR*cosf(aa1),         6.5f+archR*sinf(aa1),         archZ+0.3f);
        glVertex3f(archX+(archR-archT)*cosf(aa1),6.5f+(archR-archT)*sinf(aa1),archZ+0.3f);
        glEnd();
    }
    /* "CIRCUS" sign on beam */
    glColor3f(0.52f,0.08f,0.52f);
    glPushMatrix();glTranslatef(archX,8.0f,archZ);glScalef(7.5f,1.2f,0.35f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.98f,0.90f,0.10f);
    glPushMatrix();glTranslatef(archX,8.0f,archZ+0.20f);glScalef(7.0f,0.85f,0.12f);glutSolidCube(1);glPopMatrix();
    /* animated light bulbs on sign */
    glDisable(GL_LIGHTING);
    for(int b=0;b<10;b++){
        float bx2=archX-3.4f+b*0.75f;
        float hue=spotTime+b*0.62f;
        glColor3f(0.5f+0.5f*sinf(hue),0.5f+0.5f*sinf(hue+2.1f),0.5f+0.5f*sinf(hue+4.2f));
        glPushMatrix();glTranslatef(bx2,7.2f,archZ+0.22f);glutSolidSphere(0.14f,6,6);glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    /* red carpet approach */
    glDisable(GL_LIGHTING);
    glColor3f(0.72f,0.10f,0.10f);
    glBegin(GL_QUADS);
    glVertex3f(archX+1,0.02f,archZ-ENT_W+0.3f);
    glVertex3f(archX+8,0.02f,archZ-ENT_W+0.3f);
    glVertex3f(archX+8,0.02f,archZ+ENT_W-0.3f);
    glVertex3f(archX+1,0.02f,archZ+ENT_W-0.3f);
    glEnd();
    /* carpet gold border */
    glColor3f(0.88f,0.72f,0.10f);
    for(int cb=0;cb<2;cb++){
        float cz2=archZ+(cb==0?-ENT_W+0.3f:ENT_W-0.3f);
        glBegin(GL_QUADS);
        glVertex3f(archX+1,0.025f,cz2);   glVertex3f(archX+8,0.025f,cz2);
        glVertex3f(archX+8,0.025f,cz2+(cb==0?0.22f:-0.22f));
        glVertex3f(archX+1,0.025f,cz2+(cb==0?0.22f:-0.22f));
        glEnd();
    }
    glEnable(GL_LIGHTING);

    /* ── Pennant flags around perimeter ── */
    float flagCol[][3]={
        {0.92f,0.18f,0.18f},{0.18f,0.52f,0.92f},{0.18f,0.82f,0.22f},
        {0.92f,0.72f,0.10f},{0.82f,0.18f,0.82f},{0.92f,0.48f,0.10f}
    };
    const int FLAGS=16;
    for(int f=0;f<FLAGS;f++){
        float fa=f*2*(float)M_PI/FLAGS;
        float fx=cosf(fa)*(TR+1.0f), fz=sinf(fa)*(TR+1.0f);
        float sway=(flagsOn?sinf(windTime*1.5f+f*0.8f)*0.22f:0);
        glColor3f(flagCol[f%6][0],flagCol[f%6][1],flagCol[f%6][2]);
        glBegin(GL_TRIANGLES);
        glVertex3f(fx,         BH+TH,      fz);
        glVertex3f(fx+0.65f+sway,BH+TH-0.6f,fz);
        glVertex3f(fx,         BH+TH-1.1f, fz);
        glEnd();
        glDisable(GL_LIGHTING);
        glColor3f(0.30f,0.28f,0.22f);
        glBegin(GL_LINES);
        glVertex3f(0,BH+TH+0.6f,0); glVertex3f(fx,BH+TH,fz);
        glEnd();
        glEnable(GL_LIGHTING);
    }

    /* ── Lamp posts flanking entrance ── */
    for(int s=-1;s<=1;s+=2){
        float lx=archX+2.5f;
        float lz2=archZ+s*(ENT_W+1.5f);
        glColor3f(0.22f,0.22f,0.30f);
        glPushMatrix();glTranslatef(lx,0,lz2);glRotatef(-90,1,0,0);
        gluCylinder(q,0.12f,0.08f,6.5f,10,1);glPopMatrix();
        glPushMatrix();glTranslatef(lx,6.5f,lz2);glRotatef(30,0,0,1);
        GLUquadric*ql=gluNewQuadric();gluCylinder(ql,.07,.07,1.3f,8,1);gluDeleteQuadric(ql);glPopMatrix();
        glColor3f(1.0f,0.96f,0.72f);
        glPushMatrix();glTranslatef(lx-1.1f,6.5f,lz2);glutSolidSphere(0.22f,8,8);glPopMatrix();
    }

    /* ── NOW DRAW INTERIOR (always visible through entrance gap) ── */
    drawRingFloor();
    drawBleachers();
    drawAudience();
    drawRigStructure();
    drawRingLights();
    drawBunting();
    drawSpotlightBeams();
    if(acrobatOn) drawAcrobat(acrobatTime);
    drawClown(acrobatTime);
    drawLionTamer(acrobatTime);
    drawJuggler(acrobatTime);

    gluDeleteQuadric(q);
    glPopMatrix();
}

/* ─────────────────────────────────
   CAMERA  —  set view for each mode
───────────────────────────────── */
struct CamPose{float ex,ey,ez,atx,aty,atz;};

CamPose getCamPose(){
    CamPose c;
    switch(camMode){

    case 1: { /* Outside south-west view */
        float rx=angleX*(float)M_PI/180, ry=angleY*(float)M_PI/180;
        c.ex=zoomDist*cosf(rx)*sinf(ry);
        c.ey=zoomDist*sinf(rx);
        c.ez=zoomDist*cosf(rx)*cosf(ry);
        c.atx=0;c.aty=8;c.atz=0; break;
    }
    case 2: { /* Front entrance close-up */
        float rx=angleX*(float)M_PI/180, ry=angleY*(float)M_PI/180;
        float ex=25+cosf(rx)*sinf(ry)*15;
        float ey=6+sinf(rx)*10;
        float ez=cosf(rx)*cosf(ry)*8;
        c.ex=ex;c.ey=ey;c.ez=ez;
        c.atx=17;c.aty=5;c.atz=0; break;
    }
    case 3: { /* Inside — audience POV */
        float rx=angleX*(float)M_PI/180, ry=angleY*(float)M_PI/180;
        /* orbit around center from inside */
        float r=12.0f;
        c.ex=cosf(ry)*r;c.ey=5.5f+sinf(rx)*4;c.ez=sinf(ry)*r;
        c.atx=0;c.aty=8;c.atz=0; break;
    }
    case 4: { /* Inside — center ring floor, looking up */
        float ry=angleY*(float)M_PI/180;
        c.ex=cosf(ry)*2;c.ey=0.8f;c.ez=sinf(ry)*2;
        c.atx=0;c.aty=18;c.atz=0; break;
    }
    case 5: { /* Inside — aerial (trapeze level, looking down) */
        float ry=angleY*(float)M_PI/180;
        c.ex=cosf(ry)*4;c.ey=15.5f;c.ez=sinf(ry)*4;
        c.atx=0;c.aty=3;c.atz=0; break;
    }
    case 6: { /* Inside walk-through — slow animated orbit at crowd level */
        float a=orbitTime*0.25f+angleY*(float)M_PI/180;
        float r=11.0f;
        c.ex=cosf(a)*r;c.ey=4.5f;c.ez=sinf(a)*r;
        c.atx=cosf(a+0.15f)*2;c.aty=7;c.atz=sinf(a+0.15f)*2; break;
    }
    case 7: { /* Outside animated orbit */
        float a=orbitTime*0.2f+angleY*(float)M_PI/180;
        float r=zoomDist*0.7f;
        c.ex=cosf(a)*r;c.ey=zoomDist*0.35f;c.ez=sinf(a)*r;
        c.atx=0;c.aty=10;c.atz=0; break;
    }
    default:
        c.ex=50;c.ey=30;c.ez=50;c.atx=0;c.aty=8;c.atz=0;
    }
    return c;
}

/* ─────────────────────────────────
   HUD  (on-screen mode indicator)
───────────────────────────────── */
void drawHUD(){
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    int vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    glOrtho(0,vp[2],0,vp[3],-1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    /* semi-transparent dark bar at bottom */
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.55f);
    glBegin(GL_QUADS);
    glVertex2f(0,0);glVertex2f(vp[2],0);glVertex2f(vp[2],38);glVertex2f(0,38);
    glEnd();
    glDisable(GL_BLEND);

    /* mode dots */
    const char* labels[]={"1:Outside","2:Entrance","3:Audience","4:Ring Flr","5:Aerial","6:Walk-in","7:Orbit"};
    for(int m=1;m<=7;m++){
        float bx=10+(m-1)*130.0f;
        if(m==camMode){glColor3f(1.0f,0.85f,0.08f);} else {glColor3f(0.55f,0.55f,0.58f);}
        glRasterPos2f(bx,14);
        const char*s=labels[m-1];
        while(*s){glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,*s++);s;}
    }

    /* status icons */
    glColor3f(lightsOn?1.0f:.4f, lightsOn?.9f:.4f, lightsOn?.1f:.4f);
    glRasterPos2f(930,14); {const char*s="[L]Lights"; while(*s)glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,*s++);}
    glColor3f(acrobatOn?0.3f:.4f,acrobatOn?0.8f:.4f,acrobatOn?0.3f:.4f);
    glRasterPos2f(1030,14); {const char*s="[A]Anim"; while(*s)glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,*s++);}

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

/* ─────────────────────────────────
   DISPLAY
───────────────────────────────── */
void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    CamPose c=getCamPose();
    gluLookAt(c.ex,c.ey,c.ez, c.atx,c.aty,c.atz, 0,1,0);

    /* Update spotlight positions AFTER camera */
    updateSpotlights();

    /* Sky only for outside-ish views */
    if(camMode<=3||camMode==7) drawSky();
    else { glDisable(GL_LIGHTING);glClearColor(0.05f,0.04f,0.08f,1);glEnable(GL_LIGHTING);}

    drawGround();
    drawCircusTent(0,0);
    drawHUD();

    glutSwapBuffers();
}

/* ─────────────────────────────────
   TIMER
───────────────────────────────── */
void timerFunc(int){
    windTime   += 0.038f;
    spotTime   += 0.055f;
    acrobatTime+= 0.022f;
    orbitTime  += 0.016f;
    glutPostRedisplay();
    glutTimerFunc(16,timerFunc,0);
}

/* ─────────────────────────────────
   INPUT
───────────────────────────────── */
void reshape(int w,int h){
    if(!h)h=1; glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(58,(float)w/h,0.3f,400);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char k,int,int){
    if(k>='1'&&k<='7') camMode=k-'0';
    if(k=='+'||k=='=') zoomDist=std::max(5.f,zoomDist-4.0f);
    if(k=='-')         zoomDist=std::min(180.f,zoomDist+4.0f);
    if(k=='l'||k=='L') lightsOn=!lightsOn;
    if(k=='f'||k=='F') flagsOn=!flagsOn;
    if(k=='a'||k=='A') acrobatOn=!acrobatOn;
    if(k==27) exit(0);
    glutPostRedisplay();
}

void specialKeys(int k,int,int){
    if(k==GLUT_KEY_LEFT)  angleY-=3;
    if(k==GLUT_KEY_RIGHT) angleY+=3;
    if(k==GLUT_KEY_UP)    angleX=std::min(angleX+2,85.f);
    if(k==GLUT_KEY_DOWN)  angleX=std::max(angleX-2,2.f);
    glutPostRedisplay();
}

void mouseBtn(int btn,int state,int x,int y){
    if(btn==GLUT_LEFT_BUTTON){mouseDown=(state==GLUT_DOWN);lastMX=x;lastMY=y;}
    if(btn==3){zoomDist=std::max(5.f,zoomDist-3.0f);glutPostRedisplay();}
    if(btn==4){zoomDist=std::min(180.f,zoomDist+3.0f);glutPostRedisplay();}
}

void mouseMove(int x,int y){
    if(!mouseDown)return;
    angleY+=(x-lastMX)*0.4f;
    angleX=std::max(2.f,std::min(85.f,angleX-(y-lastMY)*0.35f));
    lastMX=x;lastMY=y;
    glutPostRedisplay();
}

/* ─────────────────────────────────
   INIT + MAIN
───────────────────────────────── */
void init(){
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);
    glClearColor(0.12f,0.40f,0.90f,1);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_NORMALIZE);
    setupLighting();
}

int main(int argc,char**argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(1280,800);
    glutCreateWindow("CIRCUS TENT  —  Press 1-7 for camera views");
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