/*  ============================================================
    FUNLAND 3D  —  FINAL VERSION
    - Giant Ferris Wheel centered in its zone with LED lights
    - Premium entrance gate (one big ticket booth, left side)
    - Grass-only entry path (no white paving plates)
    - Food Court: pizza + burger + ice-cream stalls clustered,
      each with a 3D vendor figure behind counter
    - Large dining tables with chairs + coloured umbrellas
    - Boundary wall replacing fence
    - Roundabout at centre
    - Grand 3-tier fountain at centre
    - Stage (north, faces south toward gate)
    - Roller coaster (NW)
    - Merry-go-round (east area)
    - Parking lot (SE)
    Controls:
      Arrow / drag = orbit    +/- = zoom    W/R/C = toggle rides
    ============================================================ */

#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif
#include <cmath>
#include <cstdio>
#include <algorithm>

/* ───────── GLOBALS ───────── */
float angleY=15.0f, angleX=26.0f, zoomDist=105.0f;
int   lastMX=-1, lastMY=-1;
bool  mouseDown=false;

float merryAngle=0, wheelAngle=0, coasterProg=0;
float windTime=0, fountainTime=0, ledTime=0, spotAngle=0;
bool  wheelSpinning=true, coasterRunning=true, merryRunning=true;
GLuint groundTex=0;

/* ═══════════════════════════════════════════════════
   BMP LOADER  (for grass.bmp, can also load food BMPs)
═══════════════════════════════════════════════════ */
unsigned char* loadBMP(const char* fn,int*w,int*h){
    FILE*f=fopen(fn,"rb");
    if(!f){printf("BMP not found: %s\n",fn);return nullptr;}
    unsigned char hdr[54]; fread(hdr,1,54,f);
    *w=*(int*)&hdr[18]; *h=*(int*)&hdr[22];
    int row=(*w*3+3)&~3;
    unsigned char*raw=new unsigned char[row*(*h)];
    fread(raw,1,row*(*h),f); fclose(f);
    unsigned char*out=new unsigned char[(*w)*(*h)*3];
    for(int i=0;i<*h;i++) for(int j=0;j<*w;j++) for(int k=0;k<3;k++)
        out[(i*(*w)+j)*3+k]=raw[i*row+j*3+(2-k)];
    delete[]raw; return out;
}

GLuint createTexture(const char* file){
    int w,h;
    unsigned char* data = loadBMP(file,&w,&h);
    if(!data) return 0;

    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);

    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,data);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    delete[] data;
    return tex;
}

GLuint pizzaTex;

/* ─── GLUT bitmap string helper ─── */
void draw3DString(float x,float y,float z,const char* s,void* font=GLUT_BITMAP_HELVETICA_18){
    glDisable(GL_LIGHTING);
    glRasterPos3f(x,y,z);
    for(int i=0;s[i];i++) glutBitmapCharacter(font,s[i]);
    glEnable(GL_LIGHTING);
}

/* ───────── LIGHTING ───────── */
void setupLighting(){
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
    GLfloat a0[]={0.35f,0.32f,0.28f,1}, d0[]={1.0f,0.96f,0.84f,1}, p0[]={80,150,100,0};
    glLightfv(GL_LIGHT0,GL_AMBIENT,a0); glLightfv(GL_LIGHT0,GL_DIFFUSE,d0); glLightfv(GL_LIGHT0,GL_POSITION,p0);
    GLfloat d1[]={0.28f,0.36f,0.50f,1}, p1[]={-50,40,-70,0};
    glLightfv(GL_LIGHT1,GL_DIFFUSE,d1); glLightfv(GL_LIGHT1,GL_POSITION,p1);
    GLfloat ga[]={0.20f,0.20f,0.22f,1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ga);
}

/* ───────── SKY ───────── */
void drawSky(){
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(-1,1,-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3f(0.08f,0.32f,0.85f); glVertex2f(-1, 1); glVertex2f(1, 1);
    glColor3f(0.55f,0.80f,1.00f); glVertex2f(1,-1); glVertex2f(-1,-1);
    glEnd();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

/* ───────── CLOUDS ───────── */
void drawCloudPuff(float x,float y,float z,float sc){
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,0.88f);
    GLUquadric*q=gluNewQuadric();
    float ox[]={0,-1.4f,1.4f,0,0.8f}, oy[]={0,0.3f,0.2f,1.1f,-0.2f};
    float rad[]={1.6f,1.2f,1.3f,1.0f,0.9f};
    for(int i=0;i<5;i++){glPushMatrix();glTranslatef(x+ox[i]*sc,y+oy[i]*sc,z);gluSphere(q,rad[i]*sc,10,10);glPopMatrix();}
    gluDeleteQuadric(q); glDisable(GL_BLEND); glEnable(GL_LIGHTING);
}
void drawClouds(){
    drawCloudPuff(-80,55,-60,3.0f); drawCloudPuff(-20,65,-100,2.5f);
    drawCloudPuff(40,58,-80,3.5f);  drawCloudPuff(90,60,-50,2.8f);
    drawCloudPuff(-50,70,-130,2.2f);drawCloudPuff(20,48,-40,2.0f);
    drawCloudPuff(65,62,-110,3.2f);
}

/* ───────── HILLS ───────── */
void drawHills(){
    glDisable(GL_LIGHTING);
    for(int l=0;l<4;l++){
        float z=-210-l*45,h=38-l*7;
        glColor3f(0.18f+l*0.04f,0.38f+l*0.03f,0.12f+l*0.02f);
        glBegin(GL_TRIANGLE_STRIP);
        for(float x=-500;x<=500;x+=4){float y=h+9*sinf(x*0.018f+l*1.1f)+5*sinf(x*0.041f+l*0.7f);
            glVertex3f(x,0,z);glVertex3f(x,y,z);}
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

/* ───────── GROUND (grass only inside park) ───────── */
void drawGround(){
    const float F=66;
    glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);
    /* dirt outside */
    glColor3f(0.42f,0.28f,0.14f);
    glBegin(GL_QUADS);
    glVertex3f(-500,-0.05f,-500);glVertex3f(-F,-0.05f,-500);glVertex3f(-F,-0.05f,500);glVertex3f(-500,-0.05f,500);
    glVertex3f(F,-0.05f,-500);glVertex3f(500,-0.05f,-500);glVertex3f(500,-0.05f,500);glVertex3f(F,-0.05f,500);
    glVertex3f(-F,-0.05f,-500);glVertex3f(F,-0.05f,-500);glVertex3f(F,-0.05f,-F);glVertex3f(-F,-0.05f,-F);
    glVertex3f(-F,-0.05f,F);glVertex3f(F,-0.05f,F);glVertex3f(F,-0.05f,500);glVertex3f(-F,-0.05f,500);
    glEnd();
    /* grass inside — texture if available */
    if(groundTex){glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,groundTex);}
    glColor3f(1,1,1);
    float ts=4.0f,rep=0.10f;
    for(float x=-F;x<F;x+=ts) for(float z2=-F;z2<F;z2+=ts){
        float tx=(x+F)*rep,tz=(z2+F)*rep;
        glBegin(GL_QUADS);
        glNormal3f(0,1,0);
        glTexCoord2f(tx,tz);              glVertex3f(x,    -0.05f,z2    );
        glTexCoord2f(tx+ts*rep,tz);       glVertex3f(x+ts, -0.05f,z2    );
        glTexCoord2f(tx+ts*rep,tz+ts*rep);glVertex3f(x+ts, -0.05f,z2+ts );
        glTexCoord2f(tx,tz+ts*rep);       glVertex3f(x,    -0.05f,z2+ts );
        glEnd();
    }
    glDisable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
}

/* ───────── ROAD (used only for main external ring road) ───────── */
void drawRoadSegment(float x1,float z1,float x2,float z2,float roadW){
    glDisable(GL_LIGHTING);
    float dx=x2-x1,dz=z2-z1,len=sqrtf(dx*dx+dz*dz);
    if(len<0.01f){glEnable(GL_LIGHTING);return;}
    float nx=-dz/len,nz=dx/len,rh=roadW*0.5f,sh=rh+0.9f;
    glColor3f(0.76f,0.72f,0.66f);
    glBegin(GL_QUADS);
    glVertex3f(x1+nx*sh,0.01f,z1+nz*sh);glVertex3f(x2+nx*sh,0.01f,z2+nz*sh);
    glVertex3f(x2-nx*sh,0.01f,z2-nz*sh);glVertex3f(x1-nx*sh,0.01f,z1-nz*sh);
    glEnd();
    glColor3f(0.22f,0.22f,0.24f);
    glBegin(GL_QUADS);
    glVertex3f(x1+nx*rh,0.03f,z1+nz*rh);glVertex3f(x2+nx*rh,0.03f,z2+nz*rh);
    glVertex3f(x2-nx*rh,0.03f,z2-nz*rh);glVertex3f(x1-nx*rh,0.03f,z1-nz*rh);
    glEnd();
    glColor3f(0.96f,0.88f,0.0f);
    float dashLen=3.5f,gapLen=2.5f,total=dashLen+gapLen;
    int numDash=(int)(len/total);
    for(int d=0;d<numDash;d++){
        float t0=(d*total)/len,t1=((d*total)+dashLen)/len;if(t1>1)t1=1;
        float cx0=x1+dx*t0,cz0=z1+dz*t0,cx1=x1+dx*t1,cz1=z1+dz*t1;
        glBegin(GL_QUADS);
        glVertex3f(cx0+nx*0.12f,0.05f,cz0+nz*0.12f);glVertex3f(cx1+nx*0.12f,0.05f,cz1+nz*0.12f);
        glVertex3f(cx1-nx*0.12f,0.05f,cz1-nz*0.12f);glVertex3f(cx0-nx*0.12f,0.05f,cz0-nz*0.12f);
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

/* Stone pathway — for main boulevard only */
void drawStonePath(float x1,float z1,float x2,float z2,float width){
    glDisable(GL_LIGHTING);
    float dx=x2-x1,dz=z2-z1,len=sqrtf(dx*dx+dz*dz);
    if(len<0.01f){glEnable(GL_LIGHTING);return;}
    float nx=-dz/len,nz=dx/len,hw=width*0.5f;
    /* warm stone base */
    glColor3f(0.80f,0.74f,0.62f);
    glBegin(GL_QUADS);
    glVertex3f(x1+nx*hw,0.02f,z1+nz*hw);glVertex3f(x2+nx*hw,0.02f,z2+nz*hw);
    glVertex3f(x2-nx*hw,0.02f,z2-nz*hw);glVertex3f(x1-nx*hw,0.02f,z1-nz*hw);
    glEnd();
    /* tile joint lines */
    glColor3f(0.62f,0.56f,0.44f);
    int ns=(int)(len/2.8f);
    for(int i=0;i<=ns;i++){
        float t=(float)i/ns,px=x1+dx*t,pz=z1+dz*t;
        glBegin(GL_LINES);
        glVertex3f(px+nx*hw,0.025f,pz+nz*hw);
        glVertex3f(px-nx*hw,0.025f,pz-nz*hw);
        glEnd();
    }
    /* longitudinal joint */
    glBegin(GL_LINES);
    glVertex3f(x1,0.025f,z1); glVertex3f(x2,0.025f,z2);
    glEnd();
    glEnable(GL_LIGHTING);
}

/* Octagonal roundabout */
void drawRoundabout(float cx,float cz,float innerR,float outerR){
    glDisable(GL_LIGHTING);
    const int SEG=64;
    glColor3f(0.76f,0.72f,0.66f);
    float sOuter=outerR+1.0f,sInner=innerR-1.0f;if(sInner<0)sInner=0;
    glBegin(GL_TRIANGLE_STRIP);
    for(int i=0;i<=SEG;i++){float a=i*2*(float)M_PI/SEG;
        glVertex3f(cx+cosf(a)*sOuter,0.01f,cz+sinf(a)*sOuter);
        glVertex3f(cx+cosf(a)*sInner,0.01f,cz+sinf(a)*sInner);}
    glEnd();
    glColor3f(0.22f,0.22f,0.24f);
    glBegin(GL_TRIANGLE_STRIP);
    for(int i=0;i<=SEG;i++){float a=i*2*(float)M_PI/SEG;
        glVertex3f(cx+cosf(a)*outerR,0.03f,cz+sinf(a)*outerR);
        glVertex3f(cx+cosf(a)*innerR,0.03f,cz+sinf(a)*innerR);}
    glEnd();
    float midR=(innerR+outerR)*0.5f;
    glColor3f(0.96f,0.88f,0.0f);
    for(int i=0;i<SEG;i+=2){
        float a0=i*2*(float)M_PI/SEG,a1=(i+1)*2*(float)M_PI/SEG;
        glBegin(GL_QUADS);
        glVertex3f(cx+cosf(a0)*(midR-.12f),0.05f,cz+sinf(a0)*(midR-.12f));
        glVertex3f(cx+cosf(a1)*(midR-.12f),0.05f,cz+sinf(a1)*(midR-.12f));
        glVertex3f(cx+cosf(a1)*(midR+.12f),0.05f,cz+sinf(a1)*(midR+.12f));
        glVertex3f(cx+cosf(a0)*(midR+.12f),0.05f,cz+sinf(a0)*(midR+.12f));
        glEnd();
    }
    glColor3f(0.62f,0.56f,0.46f);
    glBegin(GL_TRIANGLE_FAN);glVertex3f(cx,0.02f,cz);
    for(int i=0;i<=SEG;i++){float a=i*2*(float)M_PI/SEG;
        glVertex3f(cx+cosf(a)*sInner,0.02f,cz+sinf(a)*sInner);}
    glEnd();
    glEnable(GL_LIGHTING);
}

/* ───────── TREE ───────── */
void drawTree(float x,float z,float sc=1.0f){
    glPushMatrix();glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.40f,0.25f,0.10f);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.22*sc,.15*sc,3.5*sc,10,1);glPopMatrix();
    float gc[][3]={{.10f,.48f,.14f},{.08f,.42f,.12f},{.12f,.52f,.16f}};
    for(int t=0;t<3;t++){glColor3f(gc[t][0],gc[t][1],gc[t][2]);
        glPushMatrix();glTranslatef(0,(2.8f+t*1.8f)*sc,0);glRotatef(-90,1,0,0);
        gluCylinder(q,(1.6f-t*.38f)*sc,0,2.2f*sc,10,1);glPopMatrix();}
    gluDeleteQuadric(q);glPopMatrix();
}

/* ───────── PALM ───────── */
void drawPalmTree(float x,float z){
    glPushMatrix();glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.52f,0.34f,0.14f);
    for(int s=0;s<8;s++){float sy=(float)s/8;
        glPushMatrix();glTranslatef(sinf(sy*1.2f)*0.5f,sy*7,0);
        glRotatef(-90+sy*8,1,0,0);gluCylinder(q,0.22f*(1-sy*0.3f),0.20f,7.0f/8,8,1);glPopMatrix();}
    glColor3f(0.14f,0.60f,0.18f);
    for(int f=0;f<7;f++){glPushMatrix();glTranslatef(sinf(1.2f)*0.5f,7.2f,0);
        glRotatef(f*360.f/7,0,1,0);glRotatef(-35,1,0,0);
        for(int s=0;s<8;s++){float t=(float)s/8;
            glPushMatrix();glTranslatef(0,0,t*3.5f);glScalef((1-t)*0.5f,(1-t)*0.3f,0.5f);
            glutSolidSphere(1,5,5);glPopMatrix();}
        glPopMatrix();}
    gluDeleteQuadric(q);glPopMatrix();
}

/* ───────── LAMP POST ───────── */
void drawLampPost(float x,float z){
    glPushMatrix();glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.22f,0.22f,0.30f);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.12,.08,7.5,10,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,7.5f,0);glRotatef(60,0,0,1);gluCylinder(q,.07,.07,1.5,8,1);glPopMatrix();
    glPushMatrix();glTranslatef(-1.3f,7.5f,0);
    glColor3f(0.22f,0.22f,0.28f);glutSolidSphere(.3,8,8);
    glColor3f(1.0f,0.96f,0.72f);glTranslatef(0,0,.22f);glutSolidSphere(.15,6,6);glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

/* ───────── BENCH ───────── */
void drawBench(float x,float z,float ry){
    glPushMatrix();glTranslatef(x,0,z);glRotatef(ry,0,1,0);
    glColor3f(0.35f,0.22f,0.08f);
    float lx[]={-.85f,.85f,-.85f,.85f},lzz[]={-.32f,-.32f,.32f,.32f};
    for(int i=0;i<4;i++){glPushMatrix();glTranslatef(lx[i],.55f,lzz[i]);glScalef(.12f,1,.12f);glutSolidCube(1);glPopMatrix();}
    glColor3f(0.62f,0.40f,0.15f);
    glPushMatrix();glTranslatef(0,1.1f,0);glScalef(1.8f,.1f,.68f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0,1.72f,-.3f);glScalef(1.8f,.55f,.1f);glutSolidCube(1);glPopMatrix();
    glPopMatrix();
}

/* ───────── TRASH BIN ───────── */
void drawTrashBin(float x,float z,float r,float g,float b){
    glPushMatrix();glTranslatef(x,0,z);
    GLUquadric*q=gluNewQuadric();
    glColor3f(r,g,b);glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.28,.32,1.1f,12,1);glPopMatrix();
    glColor3f(r*.7f,g*.7f,b*.7f);
    glPushMatrix();glTranslatef(0,1.12f,0);glRotatef(-90,1,0,0);gluDisk(q,0,.32f,12,1);glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

/* ───────── BALLOON ───────── */
void drawBalloon(float x,float y,float z,float r,float g,float b,float sway){
    glPushMatrix();glTranslatef(x+sway,y,z);
    glDisable(GL_LIGHTING);glColor3f(.75f,.75f,.75f);
    glBegin(GL_LINES);glVertex3f(0,0,0);glVertex3f(0,-2.5f,0);glEnd();
    glEnable(GL_LIGHTING);
    glColor3f(r*.7f,g*.7f,b*.7f);glutSolidSphere(.09,6,6);
    glColor3f(r,g,b);glPushMatrix();glScalef(1.0f,1.25f,1.0f);glutSolidSphere(.60,14,14);glPopMatrix();
    glColor3f(std::min(r+.45f,1.f),std::min(g+.45f,1.f),std::min(b+.45f,1.f));
    glPushMatrix();glTranslatef(-.14f,.28f,.38f);glutSolidSphere(.11,6,6);glPopMatrix();
    glPopMatrix();
}

/* ═══════════════════════════════════════════════════════
   BOUNDARY WALL
═══════════════════════════════════════════════════════ */
void drawBoundaryWall(){
    const float F=66.0f,WH=2.8f,WT=0.9f,GW=9.0f;
    glColor3f(0.88f,0.84f,0.74f);
    glPushMatrix();glTranslatef((-F-GW)*0.5f,WH*0.5f,-F);glScalef(F-GW,WH,WT);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef((GW+F)*0.5f,WH*0.5f,-F);glScalef(F-GW,WH,WT);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0,WH*0.5f,F);glScalef(F*2,WH,WT);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(F,WH*0.5f,0);glScalef(WT,WH,F*2);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-F,WH*0.5f,0);glScalef(WT,WH,F*2);glutSolidCube(1);glPopMatrix();
    glColor3f(0.70f,0.60f,0.45f);
    float cH=0.30f,cE=0.18f;
    glPushMatrix();glTranslatef((-F-GW)*0.5f,WH+cH*0.5f,-F);glScalef(F-GW+cE,cH,WT+cE);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef((GW+F)*0.5f,WH+cH*0.5f,-F);glScalef(F-GW+cE,cH,WT+cE);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0,WH+cH*0.5f,F);glScalef(F*2+cE,cH,WT+cE);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(F,WH+cH*0.5f,0);glScalef(WT+cE,cH,F*2+cE);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-F,WH+cH*0.5f,0);glScalef(WT+cE,cH,F*2+cE);glutSolidCube(1);glPopMatrix();
    GLUquadric*q=gluNewQuadric();
    float corners[][2]={{-F,-F},{F,-F},{F,F},{-F,F}};
    for(int c=0;c<4;c++){
        glColor3f(0.80f,0.68f,0.48f);
        glPushMatrix();glTranslatef(corners[c][0],0,corners[c][1]);glRotatef(-90,1,0,0);
        gluCylinder(q,1.4f,1.1f,WH+1.4f,14,1);glPopMatrix();
        glColor3f(0.95f,0.25f,0.15f);
        glPushMatrix();glTranslatef(corners[c][0],WH+1.5f,corners[c][1]);glutSolidSphere(1.4f,14,14);glPopMatrix();
    }
    gluDeleteQuadric(q);
}

/* ═══════════════════════════════════════════════════════
   FUNLAND ENTRANCE GATE
═══════════════════════════════════════════════════════ */
void drawFunlandGate(float cx,float cz){
    glPushMatrix();glTranslatef(cx,0,cz);
    GLUquadric*q=gluNewQuadric();
    float gap=11.0f,ph=14.0f;
    for(int s=-1;s<=1;s+=2){
        /* base plinth */
        glColor3f(0.90f,0.82f,0.60f);
        glPushMatrix();glTranslatef(s*gap,0,0);glScalef(3.2f,0.9f,3.2f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
        /* main pillar */
        glColor3f(0.95f,0.88f,0.68f);
        glPushMatrix();glTranslatef(s*gap,0.9f,0);glRotatef(-90,1,0,0);
        gluCylinder(q,1.45f,1.2f,ph,20,2);glPopMatrix();
        /* gold globe on top */
        glColor3f(1.0f,0.92f,0.42f);
        glPushMatrix();glTranslatef(s*gap,ph+0.9f+1.8f,0);glutSolidSphere(1.9f,18,18);glPopMatrix();
        /* LED rings on pillar */
        glDisable(GL_LIGHTING);
        for(int r=0;r<6;r++){
            float ry=2.0f+r*1.8f,hue=ledTime+r*0.5f+s*1.0f;
            float lr=0.5f+0.5f*sinf(hue),lg=0.5f+0.5f*sinf(hue+2.1f),lb=0.5f+0.5f*sinf(hue+4.2f);
            glColor3f(lr,lg,lb);
            glBegin(GL_LINE_LOOP);
            for(int i=0;i<16;i++){float a=i*2*(float)M_PI/16;
                glVertex3f(s*gap+cosf(a)*1.5f,ry,sinf(a)*1.5f);}
            glEnd();
        }
        glEnable(GL_LIGHTING);
        /* red star decorations on pillar */
        for(int st=0;st<16;st++){
            float t=(float)st/16*ph,a=st*360.f/16*(float)M_PI/180;
            glColor3f(0.92f,0.18f,0.18f);
            glPushMatrix();glTranslatef(s*gap+cosf(a)*1.5f,0.9f+t,sinf(a)*1.5f);
            glScalef(0.20f,0.42f,0.20f);glutSolidSphere(1,6,6);glPopMatrix();
        }
    }
    /* horizontal beam */
    glColor3f(0.90f,0.80f,0.55f);
    glPushMatrix();glTranslatef(0,ph+0.9f,0);glScalef(gap*2.2f,2.0f,2.2f);glutSolidCube(1);glPopMatrix();
    /* rainbow arch */
    float AR=gap+2.0f,AT=0.80f,AZ=1.0f; int AS=32;
    float ac[][3]={{1,0,0},{1,.5f,0},{1,1,0},{0,.8f,0},{0,.4f,1},{.3f,0,1},{.8f,0,.8f}};
    for(int s=0;s<AS;s++){
        float a0=s*(float)M_PI/AS,a1=(s+1)*(float)M_PI/AS;
        glColor3f(ac[s%7][0],ac[s%7][1],ac[s%7][2]);
        glBegin(GL_QUADS);
        glVertex3f((AR-AT)*cosf(a0),ph+0.9f+(AR-AT)*sinf(a0),-AZ);
        glVertex3f(AR*cosf(a0),ph+0.9f+AR*sinf(a0),-AZ);
        glVertex3f(AR*cosf(a1),ph+0.9f+AR*sinf(a1),-AZ);
        glVertex3f((AR-AT)*cosf(a1),ph+0.9f+(AR-AT)*sinf(a1),-AZ);
        glVertex3f((AR-AT)*cosf(a0),ph+0.9f+(AR-AT)*sinf(a0),AZ);
        glVertex3f(AR*cosf(a0),ph+0.9f+AR*sinf(a0),AZ);
        glVertex3f(AR*cosf(a1),ph+0.9f+AR*sinf(a1),AZ);
        glVertex3f((AR-AT)*cosf(a1),ph+0.9f+(AR-AT)*sinf(a1),AZ);
        glEnd();
    }
    /* sign board */
    glColor3f(0.06f,0.04f,0.26f);
    glPushMatrix();glTranslatef(0,ph+0.9f+5.5f,-0.2f);glScalef(16.0f,3.5f,0.5f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.10f,0.06f,0.45f);
    glPushMatrix();glTranslatef(0,ph+0.9f+5.5f,-0.45f);glScalef(15.2f,2.9f,0.18f);glutSolidCube(1);glPopMatrix();
    /* gold border bars */
    glColor3f(1.0f,0.82f,0.08f);
    glPushMatrix();glTranslatef(0,ph+0.9f+7.3f,-0.46f);glScalef(15.4f,0.32f,0.14f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0,ph+0.9f+3.7f,-0.46f);glScalef(15.4f,0.32f,0.14f);glutSolidCube(1);glPopMatrix();
    /* FUNLAND coloured letter blocks */
    float lc[][3]={{0.92f,0.18f,0.18f},{0.18f,0.62f,0.92f},{0.18f,0.88f,0.28f},
                   {0.92f,0.82f,0.10f},{0.82f,0.18f,0.88f},{0.92f,0.48f,0.10f},{0.10f,0.82f,0.82f}};
    for(int L=0;L<7;L++){
        glColor3f(lc[L][0],lc[L][1],lc[L][2]);
        glPushMatrix();glTranslatef(-4.2f+L*1.40f,ph+0.9f+5.5f,-0.52f);
        glScalef(0.88f,1.8f,0.14f);glutSolidCube(1);glPopMatrix();
    }
    /* animated LED dots on beam */
    glDisable(GL_LIGHTING);
    for(int b=0;b<14;b++){
        float hue=ledTime*1.8f+b*0.45f;
        glColor3f(0.5f+0.5f*sinf(hue),0.5f+0.5f*sinf(hue+2.1f),0.5f+0.5f*sinf(hue+4.2f));
        glPushMatrix();glTranslatef(-6.2f+b*0.95f,ph+0.9f,-0.45f);glutSolidSphere(0.18f,6,6);glPopMatrix();
    }
    /* hanging catenary string lights */
    glColor3f(0.20f,0.20f,0.20f); glLineWidth(1.2f);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<=20;i++){float t=(float)i/20;
        glVertex3f(-10.0f+t*20.0f,ph+0.5f-sinf(t*(float)M_PI)*2.0f,-0.3f);}
    glEnd();
    glLineWidth(1.0f); glEnable(GL_LIGHTING);
    gluDeleteQuadric(q);glPopMatrix();
}

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
/* ═══════════════════════════════════════════════════════
   GRAND FOUNTAIN  (3-tier, big, centre plaza)
═══════════════════════════════════════════════════════ */
void drawFountain(float cx,float cz){
    glPushMatrix();glTranslatef(cx,0,cz);
    GLUquadric*q=gluNewQuadric();
    /* outer basin */
    glColor3f(0.68f,0.72f,0.80f);
    glPushMatrix();glRotatef(-90,1,0,0);gluDisk(q,0,7.2f,32,4);glPopMatrix();
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,7.2f,7.2f,.9f,32,1);glPopMatrix();
    glColor3f(0.78f,0.82f,0.90f);
    glPushMatrix();glTranslatef(0,.92f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,7.2f,7.6f,.32f,32,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,1.24f,0);glRotatef(-90,1,0,0);gluDisk(q,6.8f,7.6f,32,2);glPopMatrix();
    glColor3f(0.32f,0.60f,0.90f);
    glPushMatrix();glTranslatef(0,.78f,0);glRotatef(-90,1,0,0);gluDisk(q,0,6.8f,32,4);glPopMatrix();
    /* outer basin wall decorative border stones */
    for(int s=0;s<20;s++){float sa=s*18*(float)M_PI/180;
        glColor3f(0.70f,0.72f,0.78f);
        glPushMatrix();glTranslatef(cosf(sa)*7.95f,0.05f,sinf(sa)*7.95f);
        glScalef(.60f,.40f,.60f);glutSolidSphere(1,8,8);glPopMatrix();}
    /* center pillar tier 1 */
    glColor3f(0.80f,0.82f,0.90f);
    glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.75f,.60f,4.0f,18,2);glPopMatrix();
    glColor3f(0.90f,0.84f,0.62f);
    glPushMatrix();glTranslatef(0,.42f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,1.15f,1.05f,.36f,18,1);glPopMatrix();
    /* middle basin */
    glColor3f(0.72f,0.76f,0.84f);
    glPushMatrix();glTranslatef(0,4.0f,0);glRotatef(-90,1,0,0);
    gluDisk(q,0,4.4f,24,3);glPopMatrix();
    glPushMatrix();glTranslatef(0,4.0f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,4.4f,4.4f,.60f,24,1);glPopMatrix();
    glColor3f(0.80f,0.84f,0.92f);
    glPushMatrix();glTranslatef(0,4.62f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,4.4f,4.75f,.24f,24,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,4.86f,0);glRotatef(-90,1,0,0);gluDisk(q,4.1f,4.75f,24,2);glPopMatrix();
    glColor3f(0.35f,0.62f,0.92f);
    glPushMatrix();glTranslatef(0,4.55f,0);glRotatef(-90,1,0,0);gluDisk(q,0,4.1f,24,3);glPopMatrix();
    /* pillar tier 2 */
    glColor3f(0.80f,0.82f,0.90f);
    glPushMatrix();glTranslatef(0,4.0f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.52f,.40f,3.6f,14,2);glPopMatrix();
    /* top basin */
    glColor3f(0.76f,0.80f,0.88f);
    glPushMatrix();glTranslatef(0,7.6f,0);glRotatef(-90,1,0,0);
    gluDisk(q,0,2.2f,20,3);glPopMatrix();
    glPushMatrix();glTranslatef(0,7.6f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,2.2f,2.2f,.45f,20,1);glPopMatrix();
    glColor3f(0.82f,0.86f,0.94f);
    glPushMatrix();glTranslatef(0,8.07f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,2.2f,2.5f,.20f,20,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,8.27f,0);glRotatef(-90,1,0,0);gluDisk(q,2.0f,2.5f,20,2);glPopMatrix();
    glColor3f(0.38f,0.65f,0.95f);
    glPushMatrix();glTranslatef(0,8.0f,0);glRotatef(-90,1,0,0);gluDisk(q,0,2.0f,20,3);glPopMatrix();
    /* golden ornament spire */
    glColor3f(0.92f,0.80f,0.15f);
    glPushMatrix();glTranslatef(0,7.6f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.20f,.12f,3.2f,10,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,10.84f,0);glutSolidSphere(.48f,16,16);glPopMatrix();
    for(int t=0;t<6;t++){float ta=t*60*(float)M_PI/180;
        glColor3f(1.0f,0.90f,0.18f);
        glPushMatrix();glTranslatef(cosf(ta)*.62f,10.84f,sinf(ta)*.62f);
        glutSolidSphere(.18f,8,8);glPopMatrix();}
    /* ── water jets ── */
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    /* outer ring */
    for(int j=0;j<14;j++){
        float ja=j*360.f/14*(float)M_PI/180;
        float jph=fountainTime*1.2f+j*.45f,jh=sinf(jph)*0.65f+2.5f;
        for(int s=0;s<10;s++){float t=(float)s/10,t1=(float)(s+1)/10;
            float arc=jh*(1-(2*t-1)*(2*t-1)),arc1=jh*(1-(2*t1-1)*(2*t1-1));
            float r0=6.0f+t*1.5f,r1=6.0f+t1*1.5f;
            glColor4f(0.55f,0.82f,1.0f,0.55f);
            glBegin(GL_QUADS);
            glVertex3f(cosf(ja)*(r0-.08f),0.80f+arc,sinf(ja)*(r0-.08f));
            glVertex3f(cosf(ja)*(r0+.08f),0.80f+arc,sinf(ja)*(r0+.08f));
            glVertex3f(cosf(ja)*(r1+.08f),0.80f+arc1,sinf(ja)*(r1+.08f));
            glVertex3f(cosf(ja)*(r1-.08f),0.80f+arc1,sinf(ja)*(r1-.08f));
            glEnd();}
    }
    /* mid ring */
    for(int j=0;j<9;j++){
        float ja=j*40*(float)M_PI/180;
        float jph=fountainTime+j*.8f,jh=sinf(jph)*0.5f+3.2f;
        for(int s=0;s<12;s++){float t=(float)s/12,t1=(float)(s+1)/12;
            float arc=jh*(1-(2*t-1)*(2*t-1)),arc1=jh*(1-(2*t1-1)*(2*t1-1));
            float r0=3.4f+t*1.1f,r1=3.4f+t1*1.1f;
            glColor4f(0.60f,0.86f,1.0f,0.65f);
            glBegin(GL_QUADS);
            glVertex3f(cosf(ja)*(r0-.06f),4.58f+arc,sinf(ja)*(r0-.06f));
            glVertex3f(cosf(ja)*(r0+.06f),4.58f+arc,sinf(ja)*(r0+.06f));
            glVertex3f(cosf(ja)*(r1+.06f),4.58f+arc1,sinf(ja)*(r1+.06f));
            glVertex3f(cosf(ja)*(r1-.06f),4.58f+arc1,sinf(ja)*(r1-.06f));
            glEnd();}
    }
    /* top jets */
    for(int j=0;j<6;j++){
        float ja=j*60*(float)M_PI/180;
        float jph=fountainTime*1.4f+j*1.05f,jh=sinf(jph)*0.4f+2.4f;
        for(int s=0;s<10;s++){float t=(float)s/10,t1=(float)(s+1)/10;
            float arc=jh*(1-(2*t-1)*(2*t-1)),arc1=jh*(1-(2*t1-1)*(2*t1-1));
            float r0=1.3f+t*.85f,r1=1.3f+t1*.85f;
            glColor4f(0.68f,0.90f,1.0f,0.72f);
            glBegin(GL_QUADS);
            glVertex3f(cosf(ja)*(r0-.05f),8.02f+arc,sinf(ja)*(r0-.05f));
            glVertex3f(cosf(ja)*(r0+.05f),8.02f+arc,sinf(ja)*(r0+.05f));
            glVertex3f(cosf(ja)*(r1+.05f),8.02f+arc1,sinf(ja)*(r1+.05f));
            glVertex3f(cosf(ja)*(r1-.05f),8.02f+arc1,sinf(ja)*(r1-.05f));
            glEnd();}
    }
    glDisable(GL_BLEND);
    gluDeleteQuadric(q);glPopMatrix();
}

/* ═══════════════════════════════════════════════════════
   STAGE  (faces south = -Z toward gate)
═══════════════════════════════════════════════════════ */
void drawStage(float cx,float cz){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(180,0,1,0);
    GLUquadric*q=gluNewQuadric();
    /* platform */
    glColor3f(0.55f,0.30f,0.10f);
    glPushMatrix();glTranslatef(0,.68f,0);glScalef(18.0f,1.35f,10.0f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.70f,0.44f,0.18f);
    glPushMatrix();glTranslatef(0,1.38f,0);glScalef(18.2f,.14f,10.2f);glutSolidCube(1);glPopMatrix();
    /* 4 steps facing south (local +Z after 180 rotation = world -Z) */
    for(int s=0;s<4;s++){
        glColor3f(0.62f+(s*.03f),0.36f+(s*.02f),0.13f);
        glPushMatrix();glTranslatef(0,s*.40f+.20f,5.2f+s*.58f);
        glScalef(10.0f,.40f,.58f);glutSolidCube(1);glPopMatrix();}
    /* back curtain */
    glColor3f(0.60f,0.06f,0.56f);
    glPushMatrix();glTranslatef(0,7.5f,-5.2f);glScalef(18.5f,13.0f,.5f);glutSolidCube(1);glPopMatrix();
    /* curtain folds */
    for(int f=0;f<10;f++){float fx=-8.5f+f*1.9f;
        glColor3f(f%2==0?.50f:.72f,f%2==0?.04f:.12f,f%2==0?.46f:.68f);
        glPushMatrix();glTranslatef(fx,7.5f,-4.95f);glScalef(.65f,12.8f,.28f);glutSolidCube(1);glPopMatrix();}
    /* side lighting towers */
    for(int s=-1;s<=1;s+=2){
        glColor3f(0.22f,0.22f,0.26f);
        glPushMatrix();glTranslatef(s*10.2f,0,-3.8f);glRotatef(-90,1,0,0);
        gluCylinder(q,.28,.22,15.5f,12,1);glPopMatrix();
        float spC[][3]={{1.0f,0.4f,0},{0.4f,1,0},{0,0.4f,1},{1,0,0.4f}};
        for(int l=0;l<4;l++){
            glColor3f(spC[l][0],spC[l][1],spC[l][2]);
            glPushMatrix();glTranslatef(s*10.2f,14.5f,-3.8f);
            glRotatef(sinf(spotAngle*(float)M_PI/180+l*.8f)*22,0,1,0);
            glRotatef(-50,1,0,0);gluCylinder(q,.22,.50,2.4f,8,1);glPopMatrix();}
    }
    /* top arch */
    glColor3f(0.30f,0.30f,0.36f);
    for(int s=-1;s<=1;s+=2){
        glPushMatrix();glTranslatef(s*8.8f,0,-5.0f);glRotatef(-90,1,0,0);
        gluCylinder(q,.30,.25,17.0f,10,1);glPopMatrix();}
    glPushMatrix();glTranslatef(0,17.0f,-5.0f);glRotatef(90,0,1,0);
    gluCylinder(q,.26,.26,17.8f,10,1);glPopMatrix();
    /* bunting */
    for(int b=0;b<18;b++){float bx=-8.4f+b*0.98f;
        float sag=sinf((float)b/17*(float)M_PI)*(-0.7f);
        float bc[][3]={{1,.1f,.1f},{1,.7f,.0f},{1,1,.0f},{.0f,.8f,.2f},{.0f,.4f,1},{.6f,.0f,.8f}};
        glColor3f(bc[b%6][0],bc[b%6][1],bc[b%6][2]);
        glBegin(GL_TRIANGLES);
        glVertex3f(bx,16.8f+sag,-4.8f);glVertex3f(bx+.49f,16.8f+sag,-4.8f);
        glVertex3f(bx+.245f,15.9f+sag,-4.8f);glEnd();}
    /* mic stand */
    glColor3f(0.18f,0.18f,0.18f);
    glPushMatrix();glTranslatef(0,1.40f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.055,.055,3.9f,8,1);glPopMatrix();
    glColor3f(0.28f,0.28f,0.28f);
    glPushMatrix();glTranslatef(0,5.35f,0);glutSolidSphere(.23f,10,10);glPopMatrix();
    /* speakers on stage */
    for(int s=-1;s<=1;s+=2){
        glColor3f(0.12f,0.12f,0.16f);
        glPushMatrix();glTranslatef(s*7.4f,1.42f,3.6f);glScalef(2.2f,3.4f,1.5f);glutSolidCube(1);glPopMatrix();
        glColor3f(0.32f,0.32f,0.36f);
        for(int r=0;r<3;r++){
            glPushMatrix();glTranslatef(s*7.4f,2.0f+r*.9f,4.38f);
            gluDisk(q,0,.58f,14,2);glPopMatrix();}
        /* second stack */
        glColor3f(0.12f,0.12f,0.16f);
        glPushMatrix();glTranslatef(s*7.4f,4.85f,3.6f);glScalef(2.0f,2.8f,1.4f);glutSolidCube(1);glPopMatrix();
        for(int r=0;r<2;r++){
            glColor3f(0.32f,0.32f,0.36f);
            glPushMatrix();glTranslatef(s*7.4f,5.4f+r*.9f,4.32f);
            gluDisk(q,0,.55f,12,2);glPopMatrix();}
    }
    gluDeleteQuadric(q);glPopMatrix();
}

/* ═══════════════════════════════════════════════════════════════
   2.  PIRATE SHIP RIDE
       Centre: (22, 0, 15)
       Footprint: ~12 × 5 units
       A pendulum ship that swings back and forth on a pivot frame
   ═══════════════════════════════════════════════════════════════ */
void drawPirateShip(float cx, float cz) {
    glPushMatrix();
    glTranslatef(cx, 0.0f, cz);
    GLUquadric* q = gluNewQuadric();
 
    extern float merryAngle;          /* reuse for pendulum angle */
    float swing = sinf(merryAngle * (float)M_PI / 180.0f * 1.6f) * 38.0f; /* ±38° */
 
    /* ── stone/concrete platform ── */
    glColor3f(0.62f, 0.58f, 0.50f);
    glPushMatrix();
    glTranslatef(0, 0.12f, 0);
    glScalef(14.0f, 0.24f, 5.5f);
    glutSolidCube(1);
    glPopMatrix();
 
    /* ── pivot A-frame left ── */
    glColor3f(0.28f, 0.28f, 0.34f);
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(s * 6.5f, 0, 0);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.30f, 0.24f, 8.5f, 10, 1);
        glPopMatrix();
        /* diagonal brace */
        glPushMatrix();
        glTranslatef(s * 6.5f, 0, 1.6f);
        glRotatef(-78.0f, 1, 0, 0);
        gluCylinder(q, 0.18f, 0.14f, 8.8f, 8, 1);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(s * 6.5f, 0, -1.6f);
        glRotatef(-102.0f, 1, 0, 0);
        gluCylinder(q, 0.18f, 0.14f, 8.8f, 8, 1);
        glPopMatrix();
    }
    /* horizontal pivot axle */
    glColor3f(0.52f, 0.52f, 0.60f);
    glPushMatrix();
    glTranslatef(-6.5f, 8.5f, 0);
    glRotatef(90, 0, 1, 0);
    gluCylinder(q, 0.28f, 0.28f, 13.0f, 10, 1);
    glPopMatrix();
 
    /* ── SWINGING SHIP ── */
    glPushMatrix();
    glTranslatef(0, 8.5f, 0);
    glRotatef(swing, 0, 0, 1);         /* pendulum swing around pivot */
 
    /* suspension arm */
    glColor3f(0.32f, 0.32f, 0.38f);
    glPushMatrix();
    glTranslatef(0, -4.2f, 0);
    glScalef(0.40f, 8.4f, 0.40f);
    glutSolidCube(1);
    glPopMatrix();
 
    /* ship hull — pointed bow/stern using scaled sphere + cube */
    glTranslatef(0, -8.5f, 0);
 
    /* hull base */
    glColor3f(0.40f, 0.22f, 0.08f);
    glPushMatrix();
    glScalef(5.5f, 0.85f, 1.55f);
    glutSolidCube(1);
    glPopMatrix();
    /* hull sides (darker) */
    glColor3f(0.30f, 0.16f, 0.06f);
    glPushMatrix();
    glTranslatef(0, -0.50f, 0);
    glScalef(5.2f, 0.65f, 1.45f);
    glutSolidCube(1);
    glPopMatrix();
    /* bow cap */
    glColor3f(0.40f, 0.22f, 0.08f);
    glPushMatrix();
    glTranslatef(2.9f, 0, 0);
    glScalef(1.0f, 0.82f, 1.40f);
    glutSolidSphere(0.6f, 10, 8);
    glPopMatrix();
    /* stern cap */
    glPushMatrix();
    glTranslatef(-2.9f, 0, 0);
    glScalef(1.0f, 0.82f, 1.40f);
    glutSolidSphere(0.6f, 10, 8);
    glPopMatrix();
 
    /* deck planks (colour bands) */
    for (int p = 0; p < 5; p++) {
        glColor3f(p % 2 == 0 ? 0.68f : 0.55f, p % 2 == 0 ? 0.44f : 0.34f, 0.18f);
        glPushMatrix();
        glTranslatef(0, 0.44f, -0.55f + p * 0.28f);
        glScalef(5.0f, 0.05f, 0.26f);
        glutSolidCube(1);
        glPopMatrix();
    }
 
    /* passenger railings */
    glColor3f(0.78f, 0.62f, 0.18f);
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(0, 0.78f, s * 0.82f);
        glRotatef(90, 0, 1, 0);
        gluCylinder(q, 0.06f, 0.06f, 5.2f, 6, 1);
        glPopMatrix();
    }
    /* railing posts */
    for (int p = 0; p < 5; p++) {
        for (int s = -1; s <= 1; s += 2) {
            glColor3f(0.70f, 0.55f, 0.15f);
            glPushMatrix();
            glTranslatef(-2.2f + p * 1.1f, 0.45f, s * 0.82f);
            glRotatef(-90, 1, 0, 0);
            gluCylinder(q, 0.05f, 0.05f, 0.38f, 6, 1);
            glPopMatrix();
        }
    }
 
    /* skull & crossbones flag */
    glColor3f(0.08f, 0.08f, 0.08f);
    glPushMatrix();
    glTranslatef(0, 0.5f, 0);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.06f, 0.05f, 3.2f, 6, 1);
    glPopMatrix();
    glColor3f(0.12f, 0.12f, 0.14f);
    glPushMatrix();
    glTranslatef(0.9f, 3.7f, 0);
    glScalef(1.5f, 1.0f, 0.10f);
    glutSolidCube(1);
    glPopMatrix();
    /* skull */
    glColor3f(0.92f, 0.90f, 0.86f);
    glPushMatrix();
    glTranslatef(0.9f, 3.72f, 0);
    glutSolidSphere(0.22f, 10, 10);
    glPopMatrix();
 
    glPopMatrix(); /* end swinging ship */
 
    /* ── safety barrier ── */
    glColor3f(0.88f, 0.72f, 0.10f);
    glDisable(GL_LIGHTING);
    glLineWidth(2.2f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 32; i++) {
        float a = i * 2.0f * (float)M_PI / 32.0f;
        glVertex3f(cosf(a) * 6.8f, 0.30f, sinf(a) * 2.6f);
    }
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
 
    /* bollard posts */
    glColor3f(0.88f, 0.72f, 0.10f);
    for (int i = 0; i < 8; i++) {
        float a = i * 45.0f * (float)M_PI / 180.0f;
        glPushMatrix();
        glTranslatef(cosf(a) * 6.8f, 0, sinf(a) * 2.6f);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.10f, 0.10f, 1.1f, 6, 1);
        glPopMatrix();
    }
 
    /* sign post "PIRATE SHIP" */
    glColor3f(0.22f, 0.22f, 0.28f);
    glPushMatrix();
    glTranslatef(7.5f, 0, 0);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.08f, 0.08f, 2.8f, 6, 1);
    glPopMatrix();
    glColor3f(0.52f, 0.12f, 0.08f);
    glPushMatrix();
    glTranslatef(7.5f, 2.9f, 0);
    glScalef(2.8f, 0.75f, 0.22f);
    glutSolidCube(1);
    glPopMatrix();
    glColor3f(1.0f, 0.88f, 0.08f);
    glPushMatrix();
    glTranslatef(7.5f, 2.9f, 0.12f);
    glScalef(2.5f, 0.55f, 0.08f);
    glutSolidCube(1);
    glPopMatrix();
 
    gluDeleteQuadric(q);
    glPopMatrix();
}
 

/* ═══════════════════════════════════════════════════════════════
   3.  BUMPER CARS  ARENA
       Centre: (40, 0, 45)
       Footprint: 18 × 14 units
       Low-walled electrified arena with 8 bumper cars
   ═══════════════════════════════════════════════════════════════ */
void drawBumperCars(float cx, float cz) {
    glPushMatrix();
    glTranslatef(cx, 0.0f, cz);
    GLUquadric* q = gluNewQuadric();
 
    /* ── arena floor ── */
    glColor3f(0.20f, 0.20f, 0.24f);
    glPushMatrix();
    glTranslatef(0, 0.04f, 0);
    glScalef(18.0f, 0.08f, 14.0f);
    glutSolidCube(1);
    glPopMatrix();
 
    /* floor checker pattern */
    glDisable(GL_LIGHTING);
    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 9; c++) {
            glColor3f((r + c) % 2 == 0 ? 0.18f : 0.28f,
                      (r + c) % 2 == 0 ? 0.18f : 0.28f,
                      (r + c) % 2 == 0 ? 0.22f : 0.34f);
            glBegin(GL_QUADS);
            glVertex3f(-9.0f + c * 2.0f,        0.09f, -7.0f + r * 2.0f);
            glVertex3f(-9.0f + (c+1) * 2.0f,    0.09f, -7.0f + r * 2.0f);
            glVertex3f(-9.0f + (c+1) * 2.0f,    0.09f, -7.0f + (r+1) * 2.0f);
            glVertex3f(-9.0f + c * 2.0f,        0.09f, -7.0f + (r+1) * 2.0f);
            glEnd();
        }
    }
    glEnable(GL_LIGHTING);
 
    /* ── perimeter wall ── */
    const float AW = 9.0f, AL = 7.0f, WH = 1.0f, WT = 0.35f;
    glColor3f(0.88f, 0.28f, 0.08f);   /* red rubber bumper wall */
    /* N wall */
    glPushMatrix();
    glTranslatef(0, WH * 0.5f, -AL);
    glScalef(AW * 2, WH, WT);
    glutSolidCube(1);
    glPopMatrix();
    /* S wall */
    glPushMatrix();
    glTranslatef(0, WH * 0.5f, AL);
    glScalef(AW * 2, WH, WT);
    glutSolidCube(1);
    glPopMatrix();
    /* W wall */
    glPushMatrix();
    glTranslatef(-AW, WH * 0.5f, 0);
    glScalef(WT, WH, AL * 2);
    glutSolidCube(1);
    glPopMatrix();
    /* E wall */
    glPushMatrix();
    glTranslatef(AW, WH * 0.5f, 0);
    glScalef(WT, WH, AL * 2);
    glutSolidCube(1);
    glPopMatrix();
    /* wall top rail (yellow) */
    glColor3f(0.95f, 0.88f, 0.08f);
    const float RE = 0.12f;
    glPushMatrix();
    glTranslatef(0, WH + RE * 0.5f, -AL);
    glScalef(AW * 2 + RE, RE, WT + RE);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, WH + RE * 0.5f, AL);
    glScalef(AW * 2 + RE, RE, WT + RE);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-AW, WH + RE * 0.5f, 0);
    glScalef(WT + RE, RE, AL * 2 + RE);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(AW, WH + RE * 0.5f, 0);
    glScalef(WT + RE, RE, AL * 2 + RE);
    glutSolidCube(1);
    glPopMatrix();
 
    /* ── overhead electric grid poles ── */
    glColor3f(0.30f, 0.30f, 0.36f);
    float corners[4][2] = {{-8.0f,-6.0f},{8.0f,-6.0f},{8.0f,6.0f},{-8.0f,6.0f}};
    for (int c = 0; c < 4; c++) {
        glPushMatrix();
        glTranslatef(corners[c][0], 0, corners[c][1]);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.18f, 0.15f, 4.5f, 8, 1);
        glPopMatrix();
    }
    /* horizontal grid cables */
    glDisable(GL_LIGHTING);
    glColor3f(0.55f, 0.55f, 0.60f);
    glLineWidth(1.5f);
    /* longitudinal */
    for (int row = 0; row < 3; row++) {
        float zv = -4.0f + row * 4.0f;
        glBegin(GL_LINES);
        glVertex3f(-8.0f, 4.5f, zv);
        glVertex3f(8.0f,  4.5f, zv);
        glEnd();
    }
    /* lateral */
    for (int col = 0; col < 5; col++) {
        float xv = -8.0f + col * 4.0f;
        glBegin(GL_LINES);
        glVertex3f(xv, 4.5f, -6.0f);
        glVertex3f(xv, 4.5f,  6.0f);
        glEnd();
    }
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
 
    /* ── 8 bumper cars ── */
    extern float merryAngle;
    float carPos[8][2] = {
        {-6.2f,-4.5f},{-2.5f,-4.5f},{ 2.5f,-4.5f},{ 6.2f,-4.5f},
        {-6.2f, 4.5f},{-2.5f, 4.5f},{ 2.5f, 4.5f},{ 6.2f, 4.5f}
    };
    float carFace[8]  = { 20.0f,-15.0f,25.0f,-10.0f,170.0f,160.0f,175.0f,155.0f };
    /* add slight animated drift */
    float drift[8]    = { 0.06f,-0.04f, 0.05f,-0.07f,-0.05f, 0.06f,-0.06f, 0.04f };
 
    float carColors[8][3] = {
        {0.90f,0.18f,0.18f},{0.18f,0.50f,0.92f},{0.18f,0.82f,0.22f},{0.92f,0.72f,0.10f},
        {0.82f,0.18f,0.82f},{0.10f,0.82f,0.82f},{0.92f,0.48f,0.10f},{0.88f,0.88f,0.18f}
    };
 
    for (int ci = 0; ci < 8; ci++) {
        float animX = carPos[ci][0] + sinf(merryAngle * drift[ci] + ci) * 0.8f;
        float animZ = carPos[ci][1] + cosf(merryAngle * drift[ci] + ci * 0.7f) * 0.5f;
        /* clamp inside arena */
        animX = std::max(-7.8f, std::min(7.8f, animX));
        animZ = std::max(-5.8f, std::min(5.8f, animZ));
 
        glPushMatrix();
        glTranslatef(animX, 0.10f, animZ);
        glRotatef(carFace[ci] + merryAngle * drift[ci] * 30.0f, 0, 1, 0);
 
        /* car body */
        glColor3f(carColors[ci][0], carColors[ci][1], carColors[ci][2]);
        glPushMatrix();
        glTranslatef(0, 0.32f, 0);
        glScalef(1.55f, 0.50f, 1.10f);
        glutSolidCube(1);
        glPopMatrix();
        /* cabin */
        glColor3f(carColors[ci][0] * 0.65f, carColors[ci][1] * 0.65f, carColors[ci][2] * 0.65f);
        glPushMatrix();
        glTranslatef(0, 0.70f, -0.05f);
        glScalef(1.20f, 0.40f, 0.85f);
        glutSolidCube(1);
        glPopMatrix();
        /* rubber bumper ring */
        glColor3f(0.15f, 0.15f, 0.15f);
        glPushMatrix();
        glTranslatef(0, 0.30f, 0);
        glRotatef(90, 1, 0, 0);
        gluCylinder(q, 0.82f, 0.82f, 0.12f, 16, 1);
        glPopMatrix();
        /* antenna pole (electric contact) */
        glColor3f(0.42f, 0.42f, 0.50f);
        glPushMatrix();
        glTranslatef(0.25f, 0.92f, 0.30f);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.04f, 0.03f, 3.5f, 6, 1);
        glPopMatrix();
        /* 4 wheels */
        glColor3f(0.15f, 0.15f, 0.18f);
        float wx[4] = {-0.68f, 0.68f,-0.68f, 0.68f};
        float wz[4] = {-0.42f,-0.42f, 0.42f, 0.42f};
        for (int w = 0; w < 4; w++) {
            glPushMatrix();
            glTranslatef(wx[w], 0.16f, wz[w]);
            glRotatef(90, 1, 0, 0);
            gluDisk(q, 0, 0.18f, 10, 2);
            glPopMatrix();
        }
        glPopMatrix();
    }
 
    /* ── sign "BUMPER CARS" ── */
    glColor3f(0.22f, 0.22f, 0.28f);
    glPushMatrix();
    glTranslatef(0, 0, -8.5f);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.10f, 0.10f, 3.2f, 6, 1);
    glPopMatrix();
    glColor3f(0.88f, 0.28f, 0.08f);
    glPushMatrix();
    glTranslatef(0, 3.3f, -8.5f);
    glScalef(5.0f, 0.90f, 0.25f);
    glutSolidCube(1);
    glPopMatrix();
    glColor3f(0.95f, 0.88f, 0.08f);
    glPushMatrix();
    glTranslatef(0, 3.3f, -8.62f);
    glScalef(4.6f, 0.65f, 0.08f);
    glutSolidCube(1);
    glPopMatrix();
 
    gluDeleteQuadric(q);
    glPopMatrix();
}


/* ═══════════════════════════════════════════════════════════════
   4.  CIRCUS TENT  (Big Top)
       Centre: (-50, 0, 48)
       Footprint radius: ~12 units
       Classic red-and-white striped big top with pennant flags
   ═══════════════════════════════════════════════════════════════ */
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
 
    /* ── entrance arch ── */
    float entA = 0.0f;   /* entrance faces +X (east) */
    glColor3f(0.92f, 0.78f, 0.12f);
    /* two arch posts */
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(cosf(entA) * BR + s * sinf(entA) * 2.2f,
                     0,
                     sinf(entA) * BR + s * cosf(entA) * 2.2f);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.25f, 0.20f, 4.2f, 10, 1);
        glPopMatrix();
    }
    /* arch beam */
    glPushMatrix();
    glTranslatef(cosf(entA) * BR, 4.2f, sinf(entA) * BR);
    glRotatef(atan2f(sinf(entA), cosf(entA)) * 180.0f / (float)M_PI + 90.0f, 0, 1, 0);
    gluCylinder(q, 0.18f, 0.18f, 4.4f, 8, 1);
    glPopMatrix();
    /* "CIRCUS" sign */
    glColor3f(0.52f, 0.08f, 0.52f);
    glPushMatrix();
    glTranslatef(cosf(entA) * BR, 4.5f, sinf(entA) * BR);
    glScalef(3.8f, 0.85f, 0.22f);
    glutSolidCube(1);
    glPopMatrix();
    glColor3f(0.95f, 0.88f, 0.08f);
    glPushMatrix();
    glTranslatef(cosf(entA) * BR, 4.5f, sinf(entA) * BR + 0.12f);
    glScalef(3.4f, 0.60f, 0.08f);
    glutSolidCube(1);
    glPopMatrix();
 
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
 
    /* ── lamp posts flanking entrance ── */
    for (int s = -1; s <= 1; s += 2) {
        float lx = cosf(entA) * (BR + 1.5f) + s * 2.8f;
        float lz = sinf(entA) * (BR + 1.5f);
        glColor3f(0.22f, 0.22f, 0.30f);
        glPushMatrix();
        glTranslatef(lx, 0, lz);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.12f, 0.08f, 5.5f, 10, 1);
        glPopMatrix();
        glColor3f(1.0f, 0.96f, 0.72f);
        glPushMatrix();
        glTranslatef(lx, 5.5f, lz);
        glutSolidSphere(0.22f, 8, 8);
        glPopMatrix();
    }
 
    /* ── crowd benches just outside entrance ── */
    for (int b = 0; b < 3; b++) {
        float bx = cosf(entA) * (BR + 3.5f) + (-1.0f + b) * 3.5f;
        float bz = sinf(entA) * (BR + 3.5f) + 2.5f;
        glColor3f(0.42f, 0.28f, 0.10f);
        glPushMatrix();
        glTranslatef(bx, 0.5f, bz);
        glScalef(2.8f, 0.30f, 0.8f);
        glutSolidCube(1);
        glPopMatrix();
        for (int l = -1; l <= 1; l += 2) {
            glPushMatrix();
            glTranslatef(bx + l * 1.2f, 0, bz);
            glRotatef(-90, 1, 0, 0);
            gluCylinder(q, 0.08f, 0.08f, 0.55f, 6, 1);
            glPopMatrix();
        }
    }
 
    gluDeleteQuadric(q);
    glPopMatrix();
}
/* ═══════════════════════════════════════════════════════════════
   1.  SWING RIDE  —  Chair Swing / Wave Swinger
       Centre: (48, 0, 20)
       Footprint radius: ~8 units
       Chairs hang from rotating top disc and swing outward
   ═══════════════════════════════════════════════════════════════ */
void drawSwingRide(float cx, float cz) {
    glPushMatrix();
    glTranslatef(cx, 0.0f, cz);
    GLUquadric* q = gluNewQuadric();
 
    /* ── concrete base platform ── */
    glColor3f(0.70f, 0.68f, 0.62f);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluDisk(q, 0, 7.2f, 32, 2);
    glPopMatrix();
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 7.2f, 7.2f, 0.28f, 32, 1);
    glPopMatrix();
 
    /* ── central mast ── */
    glColor3f(0.22f, 0.22f, 0.28f);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.45f, 0.38f, 10.5f, 14, 2);
    glPopMatrix();
 
    /* ── conical top cap ── */
    glColor3f(0.88f, 0.14f, 0.14f);
    glPushMatrix();
    glTranslatef(0, 10.5f, 0);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 3.8f, 0.0f, 2.2f, 20, 1);
    glPopMatrix();
    /* top disc */
    glColor3f(0.95f, 0.88f, 0.08f);
    glPushMatrix();
    glTranslatef(0, 10.5f, 0);
    glRotatef(-90, 1, 0, 0);
    gluDisk(q, 0, 3.8f, 20, 3);
    glPopMatrix();
 
    /* ── spinning assembly (uses global merryAngle for motion) ──
       External code has  extern float merryAngle;
       We reuse the same variable for visual spinning.            */
    extern float merryAngle;
    glPushMatrix();
    glTranslatef(0, 10.0f, 0);
    glRotatef(merryAngle * 0.8f, 0, 1, 0);   /* spin slightly slower than merry */
 
    const int CHAIRS = 12;
    const float CHAIN_LEN = 3.5f;
    const float SWING_OUT = 2.8f;             /* how far chairs swing outward */
 
    for (int i = 0; i < CHAIRS; i++) {
        float angle = i * (360.0f / CHAIRS) * ((float)M_PI / 180.0f);
        float armX  = 3.4f * cosf(angle);
        float armZ  = 3.4f * sinf(angle);
 
        /* radial arm from disc to chain attach point */
        glColor3f(0.40f, 0.40f, 0.48f);
        glPushMatrix();
        glTranslatef(armX * 0.5f, 0, armZ * 0.5f);
        glRotatef(atan2f(armZ, armX) * 180.0f / (float)M_PI, 0, 1, 0);
        glRotatef(90, 0, 1, 0);
        gluCylinder(q, 0.06f, 0.06f, 3.4f, 6, 1);
        glPopMatrix();
 
        /* chain — tilted outward (simulate centrifugal swing) */
        float swX = armX + SWING_OUT * cosf(angle);
        float swZ = armZ + SWING_OUT * sinf(angle);
        glColor3f(0.55f, 0.55f, 0.55f);
        glDisable(GL_LIGHTING);
        glLineWidth(1.4f);
        glBegin(GL_LINE_STRIP);
        for (int s = 0; s <= 6; s++) {
            float t  = (float)s / 6.0f;
            float px = armX + t * SWING_OUT * cosf(angle);
            float py = -t * CHAIN_LEN;
            float pz = armZ + t * SWING_OUT * sinf(angle);
            glVertex3f(px, py, pz);
        }
        glEnd();
        glLineWidth(1.0f);
        glEnable(GL_LIGHTING);
 
        /* seat / chair */
        float sc[][3] = {
            {0.92f,0.18f,0.18f},{0.18f,0.52f,0.92f},{0.18f,0.82f,0.22f},
            {0.92f,0.72f,0.10f},{0.82f,0.18f,0.82f},{0.10f,0.82f,0.82f},
            {0.92f,0.48f,0.10f},{0.18f,0.18f,0.92f},{0.92f,0.18f,0.58f},
            {0.28f,0.78f,0.28f},{0.88f,0.88f,0.18f},{0.18f,0.58f,0.88f}
        };
        glColor3f(sc[i % CHAIRS][0], sc[i % CHAIRS][1], sc[i % CHAIRS][2]);
        glPushMatrix();
        glTranslatef(swX, -CHAIN_LEN, swZ);
        glScalef(0.60f, 0.40f, 0.45f);
        glutSolidCube(1.0f);
        glPopMatrix();
        /* seat back */
        glPushMatrix();
        glTranslatef(swX, -CHAIN_LEN + 0.22f, swZ - 0.28f);
        glScalef(0.60f, 0.58f, 0.10f);
        glutSolidCube(1.0f);
        glPopMatrix();
        /* tiny rider head (decorative) */
        glColor3f(0.88f, 0.72f, 0.54f);
        glPushMatrix();
        glTranslatef(swX, -CHAIN_LEN + 0.48f, swZ);
        glutSolidSphere(0.18f, 8, 8);
        glPopMatrix();
    }
    glPopMatrix(); /* end spinning assembly */
 
    /* ── LED ring on disc ── */
    extern float ledTime;
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(0, 10.52f, 0);
    for (int i = 0; i < 20; i++) {
        float hue = ledTime * 2.0f + i * 0.31f;
        glColor3f(0.5f + 0.5f * sinf(hue),
                  0.5f + 0.5f * sinf(hue + 2.1f),
                  0.5f + 0.5f * sinf(hue + 4.2f));
        float a = i * 2.0f * (float)M_PI / 20.0f;
        glPushMatrix();
        glTranslatef(cosf(a) * 3.85f, 0, sinf(a) * 3.85f);
        glutSolidSphere(0.14f, 5, 5);
        glPopMatrix();
    }
    glPopMatrix();
    glEnable(GL_LIGHTING);
 
    /* ── three support legs ── */
    glColor3f(0.30f, 0.30f, 0.36f);
    for (int l = 0; l < 3; l++) {
        float la = l * 120.0f * (float)M_PI / 180.0f;
        glPushMatrix();
        glTranslatef(cosf(la) * 4.5f, 0, sinf(la) * 4.5f);
        glRotatef(-l * 120.0f, 0, 1, 0);
        glRotatef(15.0f, 0, 0, 1);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.22f, 0.18f, 10.8f, 10, 1);
        glPopMatrix();
    }
 
    gluDeleteQuadric(q);
    glPopMatrix();
}
/* ═══════════════════════════════════════════════════════
   MERRY-GO-ROUND
═══════════════════════════════════════════════════════ */
void drawMerryGoRound(float cx,float cz,float spin){
    glPushMatrix();glTranslatef(cx,0,cz);
    GLUquadric*q=gluNewQuadric();
    glColor3f(.62f,.18f,.58f);glPushMatrix();glRotatef(-90,1,0,0);gluDisk(q,0,5.8f,32,2);glPopMatrix();
    glColor3f(.88f,.88f,.10f);glPushMatrix();glTranslatef(0,.08f,0);glRotatef(-90,1,0,0);gluCylinder(q,5.7f,5.7f,.35f,32,1);glPopMatrix();
    glColor3f(.82f,.82f,.85f);glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.28f,.28f,9.0f,14,1);glPopMatrix();
    glColor3f(1.0f,.88f,.0f);glPushMatrix();glTranslatef(0,9.3f,0);glutSolidSphere(.6f,14,14);glPopMatrix();
    glPushMatrix();glRotatef(spin,0,1,0);
    int CS=14;float iR=.3f,oR=5.5f,cY=8.2f;
    for(int s=0;s<CS;s++){
        float a0=s*(360.f/CS)*(float)M_PI/180,a1=(s+1)*(360.f/CS)*(float)M_PI/180;
        glColor3f(s%2==0?.95f:.98f,s%2==0?.12f:.98f,s%2==0?.45f:.98f);
        glBegin(GL_QUADS);
        glVertex3f(iR*cosf(a0),cY,iR*sinf(a0));glVertex3f(oR*cosf(a0),cY-.9f,oR*sinf(a0));
        glVertex3f(oR*cosf(a1),cY-.9f,oR*sinf(a1));glVertex3f(iR*cosf(a1),cY,iR*sinf(a1));
        glEnd();
    }
    float hc[][3]={{.95f,.90f,.85f},{.60f,.32f,.14f},{.18f,.18f,.18f},{.95f,.72f,.48f},{.52f,.22f,.10f},{.85f,.85f,.80f}};
    for(int h=0;h<6;h++){
        float ha=h*60*(float)M_PI/180,hx=3.9f*cosf(ha),hz=3.9f*sinf(ha);
        float hy=1.1f+.8f*sinf(spin*(float)M_PI/180*3+h*1.05f);
        glPushMatrix();glTranslatef(hx,hy,hz);glRotatef(-ha*180/(float)M_PI+90,0,1,0);
        glColor3f(.88f,.78f,.18f);glPushMatrix();glRotatef(-90,1,0,0);gluCylinder(q,.08,.08,7-hy,8,1);glPopMatrix();
        glColor3f(hc[h][0],hc[h][1],hc[h][2]);
        glPushMatrix();glScalef(.65f,.48f,1.15f);glutSolidSphere(1,12,12);glPopMatrix();
        glPopMatrix();
    }
    glPopMatrix();gluDeleteQuadric(q);glPopMatrix();
}

/* ═══════════════════════════════════════════════════════
   GIANT FERRIS WHEEL  — PREMIUM LED VERSION
   Centred at (cx, 0, cz)
═══════════════════════════════════════════════════════ */



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

/* ═══════════════════════════════════════════════════════
   VENDOR FIGURE  (behind food counter)
═══════════════════════════════════════════════════════ */
void drawVendor(float x,float z,float face,float sr,float sg,float sb){
    glPushMatrix();glTranslatef(x,0,z);glRotatef(face,0,1,0);
    GLUquadric*q=gluNewQuadric();
    float sc=1.0f;
    /* legs */
    for(int s=-1;s<=1;s+=2){
        glColor3f(0.25f,0.25f,0.55f);
        glPushMatrix();glTranslatef(s*.19f,.92f,0);glRotatef(-90,1,0,0);
        gluCylinder(q,.11,.09,.92f,7,1);glPopMatrix();
        glColor3f(.15f,.10f,.08f);
        glPushMatrix();glTranslatef(s*.19f,.06f,.78f);glutSolidSphere(.14f,6,6);glPopMatrix();}
    /* apron body */
    glColor3f(0.92f,0.92f,0.92f);
    glPushMatrix();glTranslatef(0,1.32f,0);glScalef(.46f,.72f,.34f);glutSolidSphere(1,10,10);glPopMatrix();
    /* shirt under apron */
    glColor3f(sr,sg,sb);
    glPushMatrix();glTranslatef(0,1.32f,.12f);glScalef(.38f,.65f,.20f);glutSolidSphere(1,8,8);glPopMatrix();
    /* arms */
    for(int s=-1;s<=1;s+=2){
        glColor3f(0.88f,0.72f,0.54f);
        glPushMatrix();glTranslatef(s*.38f,1.78f,0);glRotatef(s*25,1,0,0);glRotatef(-90,1,0,0);
        gluCylinder(q,.085,.075,.68f,7,1);glPopMatrix();
        glPushMatrix();glTranslatef(s*.38f,1.78f+.52f,.44f);glutSolidSphere(.11f,6,6);glPopMatrix();}
    /* chef hat */
    glColor3f(0.95f,0.95f,0.95f);
    glPushMatrix();glTranslatef(0,2.58f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.32f,.28f,.38f,10,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,2.98f,0);glRotatef(-90,1,0,0);
    gluCylinder(q,.18f,.10f,.55f,10,1);glPopMatrix();
    /* head */
    glColor3f(0.88f,0.72f,0.54f);
    glPushMatrix();glTranslatef(0,2.54f,0);glutSolidSphere(.28f,12,12);glPopMatrix();
    /* eyes */
    glColor3f(.08f,.06f,.06f);
    glPushMatrix();glTranslatef(-.10f,2.58f,.24f);glutSolidSphere(.042f,5,5);glPopMatrix();
    glPushMatrix();glTranslatef(.10f,2.58f,.24f);glutSolidSphere(.042f,5,5);glPopMatrix();
    gluDeleteQuadric(q);glPopMatrix();
}

/* ═══════════════════════════════════════════════════════
   FOOD COURT  — big attractive stalls + vendor + big tables
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

    GLUquadric*q=gluNewQuadric();

    /* ===== BIGGER COUNTER ===== */
    glColor3f(0.55f,0.30f,0.12f);
    glPushMatrix();
    glScalef(8.0f,1.8f,4.0f);
    glTranslatef(0,.5f,0);
    glutSolidCube(1);
    glPopMatrix();

    /* top slab */
    glColor3f(0.82f,0.78f,0.65f);
    glPushMatrix();
    glTranslatef(0,1.8f,0);
    glScalef(8.2f,.2f,4.2f);
    glutSolidCube(1);
    glPopMatrix();

    /* ===== POSTS ===== */
    float px[]={-3.8f,3.8f,-3.8f,3.8f};
    float pz[]={-2.0f,-2.0f,2.0f,2.0f};
    for(int i=0;i<4;i++){
        for(int st=0;st<5;st++){
            glColor3f(st%2?1.0f:0.9f, st%2?1.0f:0.15f, st%2?1.0f:0.15f);
            glPushMatrix();
            glTranslatef(px[i],1.8f+st*1.0f,pz[i]);
            glRotatef(-90,1,0,0);
            gluCylinder(q,.18,.18,1.0f,8,1);
            glPopMatrix();
        }
    }

    /* ===== AWNING ===== */
    drawStallAwning(4.0f,4.0f,7.5f,14, 0.92f,0.18f,0.18f);

    /* ===== REMOVE OLD BOARD ===== */
    // (don’t draw any solid cube behind logo)

    /* ===== FRONT LOGO (BIG & CLEAR) ===== */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pizzaTex);
    glColor3f(1,1,1);

    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-3.5f,7.5f,-2.6f);
        glTexCoord2f(1,0); glVertex3f( 3.5f,7.5f,-2.6f);
        glTexCoord2f(1,1); glVertex3f( 3.5f,9.5f,-2.6f);
        glTexCoord2f(0,1); glVertex3f(-3.5f,9.5f,-2.6f);
    glEnd();

    /* ===== BACK LOGO (FIXED MIRROR) ===== */
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f( 3.5f,7.5f,2.6f);
        glTexCoord2f(1,0); glVertex3f(-3.5f,7.5f,2.6f);
        glTexCoord2f(1,1); glVertex3f(-3.5f,9.5f,2.6f);
        glTexCoord2f(0,1); glVertex3f( 3.5f,9.5f,2.6f);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    /* ===== PIZZA DISPLAY ON COUNTER ===== */
    for(int i=-2;i<=2;i++){
        float x = i*1.2f;

        /* base crust */
        glColor3f(0.95f,0.75f,0.25f);
        glPushMatrix();
        glTranslatef(x,1.9f,0.5f);
        glRotatef(-90,1,0,0);
        gluDisk(q,0,0.7f,16,1);
        glPopMatrix();

        /* cheese */
        glColor3f(1.0f,0.9f,0.3f);
        glPushMatrix();
        glTranslatef(x,1.92f,0.5f);
        glRotatef(-90,1,0,0);
        gluDisk(q,0,0.6f,16,1);
        glPopMatrix();

        /* pepperoni */
        glColor3f(0.8f,0.1f,0.1f);
        glPushMatrix();
        glTranslatef(x+0.2f,2.0f,0.5f);
        glutSolidSphere(0.08,8,8);
        glPopMatrix();
    }

    /* ===== VENDOR ===== */
    drawVendor(0,-1.0f,180, 0.85f,0.25f,0.2f);

    gluDeleteQuadric(q);
    glPopMatrix();
}


/* BURGER STALL */
void drawBurgerStall(float cx,float cz,float ang){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(ang,0,1,0);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.44f,0.24f,0.08f);
    glPushMatrix();glScalef(6.0f,1.3f,2.8f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(0.78f,0.68f,0.48f);
    glPushMatrix();glTranslatef(0,1.38f,0);glScalef(6.2f,.14f,3.0f);glutSolidCube(1);glPopMatrix();
    float px[]={-2.8f,2.8f,-2.8f,2.8f},pz[]={-1.35f,-1.35f,1.35f,1.35f};
    for(int i=0;i<4;i++){
        for(int st=0;st<5;st++){
            glColor3f(st%2==0?0.92f:0.98f,st%2==0?0.75f:0.98f,st%2==0?0.0f:0.98f);
            glPushMatrix();glTranslatef(px[i],1.38f+st*0.8f,pz[i]);glRotatef(-90,1,0,0);
            gluCylinder(q,.14,.14,.8f,8,1);glPopMatrix();}
    }
    drawStallAwning(3.0f,2.8f,5.45f,10, 0.92f,0.62f,0.0f);
    glColor3f(0.06f,0.04f,0.26f);
    glPushMatrix();glTranslatef(0,5.8f,-1.5f);glScalef(5.4f,1.2f,.25f);glutSolidCube(1);glPopMatrix();
    glColor3f(1.0f,0.72f,0.0f);
    glPushMatrix();glTranslatef(0,5.8f,-1.62f);glScalef(5.0f,.85f,.08f);glutSolidCube(1);glPopMatrix();
    /* burger bun on counter */
    glColor3f(0.88f,0.72f,0.28f);
    glPushMatrix();glTranslatef(-.5f,1.46f,.2f);glScalef(1.0f,0.5f,1.0f);glutSolidSphere(0.55f,12,8);glPopMatrix();
    glColor3f(0.65f,0.28f,0.12f);
    glPushMatrix();glTranslatef(-.5f,1.42f,.2f);glScalef(0.9f,0.22f,0.9f);glutSolidSphere(0.5f,10,6);glPopMatrix();
    drawVendor(0,-.6f,180, 0.92f,0.62f,0.10f);
    gluDeleteQuadric(q);glPopMatrix();
}

/* ICE CREAM STALL */
void drawIceCreamStall(float cx,float cz,float ang){
    glPushMatrix();glTranslatef(cx,0,cz);glRotatef(ang,0,1,0);
    GLUquadric*q=gluNewQuadric();
    glColor3f(0.82f,0.50f,0.75f);
    glPushMatrix();glScalef(6.0f,1.3f,2.8f);glTranslatef(0,.5f,0);glutSolidCube(1);glPopMatrix();
    glColor3f(0.95f,0.82f,0.90f);
    glPushMatrix();glTranslatef(0,1.38f,0);glScalef(6.2f,.14f,3.0f);glutSolidCube(1);glPopMatrix();
    float px[]={-2.8f,2.8f,-2.8f,2.8f},pz[]={-1.35f,-1.35f,1.35f,1.35f};
    for(int i=0;i<4;i++){
        for(int st=0;st<5;st++){
            glColor3f(st%2==0?0.92f:0.98f,st%2==0?0.42f:0.98f,st%2==0?0.72f:0.98f);
            glPushMatrix();glTranslatef(px[i],1.38f+st*0.8f,pz[i]);glRotatef(-90,1,0,0);
            gluCylinder(q,.14,.14,.8f,8,1);glPopMatrix();}
    }
    drawStallAwning(3.0f,2.8f,5.45f,10, 0.92f,0.42f,0.72f);
    glColor3f(0.06f,0.04f,0.26f);
    glPushMatrix();glTranslatef(0,5.8f,-1.5f);glScalef(5.4f,1.2f,.25f);glutSolidCube(1);glPopMatrix();
    glColor3f(1.0f,0.52f,0.82f);
    glPushMatrix();glTranslatef(0,5.8f,-1.62f);glScalef(5.0f,.85f,.08f);glutSolidCube(1);glPopMatrix();
    /* ice cream cone on counter */
    glColor3f(0.88f,0.75f,0.52f);
    glPushMatrix();glTranslatef(.4f,1.40f,.2f);glRotatef(180,1,0,0);
    gluCylinder(q,.0f,.35f,.65f,10,1);glPopMatrix();
    glColor3f(0.98f,0.62f,0.80f);
    glPushMatrix();glTranslatef(.4f,1.42f,.2f);glutSolidSphere(.38f,12,12);glPopMatrix();
    drawVendor(0,-.6f,180, 0.92f,0.42f,0.72f);
    gluDeleteQuadric(q);glPopMatrix();
}

/* ── BIG DINING TABLE + 4 CHAIRS + UMBRELLA ── */
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
    /* 4 chairs */
    for(int c=0;c<4;c++){
        float ca=c*90*(float)M_PI/180,chx=1.85f*cosf(ca),chz=1.85f*sinf(ca);
        glPushMatrix();glTranslatef(chx,0,chz);glRotatef(c*90+180,0,1,0);
        float clx[]={-.32f,.32f,-.32f,.32f},clz[]={-.32f,-.32f,.32f,.32f};
        glColor3f(0.52f,0.36f,0.16f);
        for(int l=0;l<4;l++){glPushMatrix();glTranslatef(clx[l],0,clz[l]);glRotatef(-90,1,0,0);
            gluCylinder(q,.05f,.05f,.55f,6,1);glPopMatrix();}
        glColor3f(0.72f,0.50f,0.22f);
        glPushMatrix();glTranslatef(0,.58f,0);glScalef(.72f,.10f,.72f);glutSolidCube(1);glPopMatrix();
        /* backrest posts */
        for(int s=-1;s<=1;s+=2){
            glColor3f(0.52f,0.36f,0.16f);
            glPushMatrix();glTranslatef(s*.28f,.58f,-.30f);glRotatef(-90,1,0,0);
            gluCylinder(q,.04f,.04f,.68f,6,1);glPopMatrix();}
        glColor3f(0.68f,0.46f,0.18f);
        glPushMatrix();glTranslatef(0,.98f,-.30f);glScalef(.64f,.62f,.08f);glutSolidCube(1);glPopMatrix();
        glPopMatrix();
    }
    gluDeleteQuadric(q);glPopMatrix();
}

/* ── BALLOON STALL ── */
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

/* ═══════════════════════════════════════════════════════
   ROLLER COASTER
═══════════════════════════════════════════════════════ */
void getTrackPt(float t,float&ox,float&oy,float&oz){
    t=fmodf(t,1);if(t<0)t+=1;
    float a=t*2*(float)M_PI,d=1+sinf(a)*sinf(a);
    ox=20*cosf(a)/d; oz=20*sinf(a)*cosf(a)/d;
    oy=std::max(3.0f,9+8*(sinf(a)+1)*.5f+6*sinf(a*2));
}
void drawRollerCoaster(float cx,float cz){
    glPushMatrix();glTranslatef(cx,0,cz);
    GLUquadric*q=gluNewQuadric();
    const int SEG=360; const float RG=2.4f;
    glColor3f(.32f,.32f,.36f);
    for(float tt=0;tt<1;tt+=.06f){float tx,ty,tz;getTrackPt(tt,tx,ty,tz);
        glPushMatrix();glTranslatef(tx,0,tz);glRotatef(-90,1,0,0);
        gluCylinder(q,.42,.52,ty,12,1);glPopMatrix();}
    for(int seg=0;seg<SEG;seg++){
        float t0=(float)seg/SEG,t1=(float)(seg+1)/SEG;
        float x0,y0,z0,x1,y1,z1;getTrackPt(t0,x0,y0,z0);getTrackPt(t1,x1,y1,z1);
        float dx=x1-x0,dy=y1-y0,dz2=z1-z0,tl=sqrtf(dx*dx+dy*dy+dz2*dz2);
        if(tl<.001f)continue;dx/=tl;dy/=tl;dz2/=tl;
        float px=-dz2,pz=dx,pl=sqrtf(px*px+pz*pz);if(pl>.01f){px/=pl;pz/=pl;}
        if(seg%8==0){glColor3f(.75f,.10f,.10f);
            glBegin(GL_QUADS);
            glVertex3f(x0-px*RG/2,y0+.18f,z0-pz*RG/2);glVertex3f(x0+px*RG/2,y0+.18f,z0+pz*RG/2);
            glVertex3f(x1+px*RG/2,y1+.18f,z1+pz*RG/2);glVertex3f(x1-px*RG/2,y1+.18f,z1-pz*RG/2);glEnd();}
        float yaw=atan2f(dz2,dx)*180/(float)M_PI,pit=atan2f(dy,sqrtf(dx*dx+dz2*dz2))*180/(float)M_PI;
        glColor3f(.88f,.14f,.14f);
        glPushMatrix();glTranslatef(x0-px*RG/2,y0,z0-pz*RG/2);glRotatef(yaw,0,1,0);glRotatef(-pit,0,0,1);gluCylinder(q,.28,.28,tl,8,1);glPopMatrix();
        glPushMatrix();glTranslatef(x0+px*RG/2,y0,z0+pz*RG/2);glRotatef(yaw,0,1,0);glRotatef(-pit,0,0,1);gluCylinder(q,.28,.28,tl,8,1);glPopMatrix();
    }
    float carC[][3]={{.18f,.58f,.96f},{.96f,.28f,.18f},{.18f,.88f,.28f},{.96f,.82f,.10f}};
    for(int ci=0;ci<4;ci++){
        float ct=fmodf(coasterProg/360+ci*2.2f/360,1);
        float cx2,cy,cz2;getTrackPt(ct,cx2,cy,cz2);
        float nx,ny,nz;getTrackPt(fmodf(ct+.004f,1),nx,ny,nz);
        float dx=nx-cx2,dy=ny-cy,dz2=nz-cz2;
        glPushMatrix();glTranslatef(cx2,cy,cz2);
        if(sqrtf(dx*dx+dy*dy+dz2*dz2)>.001f){glRotatef(atan2f(dz2,dx)*180/(float)M_PI,0,1,0);glRotatef(atan2f(dy,sqrtf(dx*dx+dz2*dz2))*180/(float)M_PI,0,0,1);}
        glColor3f(carC[ci][0],carC[ci][1],carC[ci][2]);
        glPushMatrix();glScalef(1.9f,.95f,1.1f);glutSolidCube(1);glPopMatrix();
        glColor3f(.10f,.10f,.10f);
        for(int w=0;w<2;w++){
            glPushMatrix();glTranslatef(-.40f,-.38f,(w==0)?-.62f:.62f);glutSolidSphere(.38f,10,10);glPopMatrix();
            glPushMatrix();glTranslatef(.40f,-.38f,(w==0)?-.62f:.62f);glutSolidSphere(.38f,10,10);glPopMatrix();}
        glPopMatrix();}
    gluDeleteQuadric(q);glPopMatrix();
}

/* ═══════════════════════════════════════════════════════
   PARKING LOT
═══════════════════════════════════════════════════════ */
void drawParkingLot(){
    glDisable(GL_LIGHTING);
    glColor3f(.18f,.18f,.18f);
    glBegin(GL_QUADS);
    glVertex3f(32,.02f,32);glVertex3f(68,.02f,32);glVertex3f(68,.02f,68);glVertex3f(32,.02f,68);glEnd();
    glColor3f(.90f,.90f,.90f);
    for(int b=0;b<=6;b++){float bx=35+b*5.5f;
        glBegin(GL_QUADS);glVertex3f(bx,.05f,36);glVertex3f(bx+.12f,.05f,36);glVertex3f(bx+.12f,.05f,45);glVertex3f(bx,.05f,45);glEnd();}
    glBegin(GL_QUADS);glVertex3f(35,.05f,36);glVertex3f(66,.05f,36);glVertex3f(66,.05f,36.12f);glVertex3f(35,.05f,36.12f);glEnd();
    glBegin(GL_QUADS);glVertex3f(35,.05f,45);glVertex3f(66,.05f,45);glVertex3f(66,.05f,45.12f);glVertex3f(35,.05f,45.12f);glEnd();
    for(int b=0;b<=6;b++){float bx=35+b*5.5f;
        glBegin(GL_QUADS);glVertex3f(bx,.05f,53);glVertex3f(bx+.12f,.05f,53);glVertex3f(bx+.12f,.05f,62);glVertex3f(bx,.05f,62);glEnd();}
    glBegin(GL_QUADS);glVertex3f(35,.05f,53);glVertex3f(66,.05f,53);glVertex3f(66,.05f,53.12f);glVertex3f(35,.05f,53.12f);glEnd();
    glBegin(GL_QUADS);glVertex3f(35,.05f,62);glVertex3f(66,.05f,62);glVertex3f(66,.05f,62.12f);glVertex3f(35,.05f,62.12f);glEnd();
    glEnable(GL_LIGHTING);
    /* cars */
    float cr1[][3]={{.88f,.10f,.10f},{.14f,.34f,.82f},{.12f,.62f,.18f},{.88f,.75f,.10f},{.62f,.16f,.65f},{.82f,.82f,.82f}};
    for(int c=0;c<6;c++){
        glPushMatrix();glTranslatef(37.5f+c*5.5f,0,39.8f);
        glColor3f(cr1[c][0],cr1[c][1],cr1[c][2]);
        glPushMatrix();glTranslatef(0,.62f,0);glScalef(2.1f,.75f,4.0f);glutSolidCube(1);glPopMatrix();
        glColor3f(cr1[c][0]*.72f,cr1[c][1]*.72f,cr1[c][2]*.72f);
        glPushMatrix();glTranslatef(0,1.18f,.1f);glScalef(1.65f,.58f,2.3f);glutSolidCube(1);glPopMatrix();
        glPopMatrix();}
    float cr2[][3]={{.92f,.48f,.10f},{.28f,.28f,.28f},{.10f,.56f,.76f},{.76f,.22f,.22f}};
    for(int c=0;c<4;c++){
        glPushMatrix();glTranslatef(37.5f+c*5.5f,0,56.8f);glRotatef(180,0,1,0);
        glColor3f(cr2[c][0],cr2[c][1],cr2[c][2]);
        glPushMatrix();glTranslatef(0,.62f,0);glScalef(2.1f,.75f,4.0f);glutSolidCube(1);glPopMatrix();
        glColor3f(cr2[c][0]*.72f,cr2[c][1]*.72f,cr2[c][2]*.72f);
        glPushMatrix();glTranslatef(0,1.18f,.1f);glScalef(1.65f,.58f,2.3f);glutSolidCube(1);glPopMatrix();
        glPopMatrix();}
    drawLampPost(36,48.5f);drawLampPost(51,48.5f);drawLampPost(65,48.5f);
}

/* ═══════════════════════════════════════════════════════
   DISPLAY
═══════════════════════════════════════════════════════ */
void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    float rx=angleX*(float)M_PI/180,ry2=angleY*(float)M_PI/180;
    float ex=zoomDist*cosf(rx)*sinf(ry2);
    float ey=zoomDist*sinf(rx);
    float ez=zoomDist*cosf(rx)*cosf(ry2);
    gluLookAt(ex,ey,ez,0,0,0,0,1,0);
    GLfloat lp0[]={80,150,100,0};
    glLightfv(GL_LIGHT0,GL_POSITION,lp0);


    pizzaTex = createTexture("pizza.bmp");

    drawSky();drawHills();drawClouds();drawGround();
    drawBoundaryWall();


        // ── NEW RIDES ──
        drawSwingRide(48, 20);
        drawPirateShip(22, 15);
        drawBumperCars(40, 45);
        drawCircusTent(-50, 48);
    /* ── INTERNAL ROADS (ring road + parking access only) ── */
    drawRoadSegment(-66,-24,-8,-24,5);drawRoadSegment(8,-24,66,-24,5);
    drawRoadSegment(-66,24,-8,24,5); drawRoadSegment(8,24,66,24,5);
    drawRoadSegment(28,-66,28,66,5);
    drawRoadSegment(-28,-66,-28,66,5);

    /* ── MAIN ENTRY BOULEVARD (stone path, no whiteplates) ── */
    drawStonePath(-8,-67,8,-67,14.0f);   /* gate mouth */
    drawStonePath(-8,-67,8, -9,10.0f);   /* gate → roundabout */
    drawStonePath(-8,-9, 8,  9, 9.0f);   /* through roundabout (grass fills rest) */
    drawStonePath(-8, 9, 8, 44, 9.0f);   /* roundabout → stage */

    /* cross paths from roundabout */
    drawStonePath(-9.5f,-3,  -22,-3, 6.0f); /* W arm → food court */
    drawStonePath( 9.5f,-3,   22,-3, 6.0f); /* E arm → rides/wheel */
    drawStonePath(-9.5f, 3,  -22, 3, 6.0f);
    drawStonePath( 9.5f, 3,   22, 3, 6.0f);

    /* ── ROUNDABOUT at centre ── */
    drawRoundabout(0,0,9.5f,15.0f);

    /* ── PERIMETER + INTERIOR TREES ── */
    float T=62;
    for(int i=-5;i<=5;i++){drawTree(-T,i*11);drawTree(T,i*11);
        if(fabsf((float)i*11)>18){drawTree(i*11,-T);drawTree(i*11,T);}}
    /* Boulevard trees (flanking stone path) */
    for(int i=0;i<9;i++){float tz=-62+i*12.0f;
        if(tz<-14||tz>14){drawTree(-9.5f,tz,1.1f);drawTree(8.5f,tz,1.1f);}}
    /* Around fountain roundabout */
    for(int i=0;i<8;i++){float ta=i*45*(float)M_PI/180+22.5f*(float)M_PI/180;
        drawTree(cosf(ta)*22.0f,sinf(ta)*22.0f,0.9f);}
    /* Food court palms */
    drawPalmTree(-54,-48);drawPalmTree(-38,-44);drawPalmTree(-54,-28);drawPalmTree(-40,-22);

    /* ── GATE ── */
    drawFunlandGate(0,-66);
    drawLampPost(-14,-60);drawLampPost(14,-60);
    drawLampPost(-9,-50);drawLampPost(9,-50);
    drawTrashBin(-12,-48,0.22f,0.62f,0.18f);drawTrashBin(12,-48,0.22f,0.62f,0.18f);

    /* ── TICKET BOOTH — ONE, LEFT/WEST SIDE, BIG ── */
    drawTicketBooth(-18,-52,90);  /* left of path, faces east toward visitors */

    /* ── GRAND FOUNTAIN (centre) ── */
    drawFountain(0,0);
    for(int l=0;l<4;l++){float la=l*90*(float)M_PI/180;drawLampPost(cosf(la)*21.5f,sinf(la)*21.5f);}
    for(int b=0;b<8;b++){float ba=b*45*(float)M_PI/180,br=12.5f;drawBench(cosf(ba)*br,sinf(ba)*br,b*45.0f+90);}
    drawTrashBin(11,4,0.68f,0.68f,0.68f);drawTrashBin(-9,4,0.68f,0.68f,0.68f);

    /* ── STAGE (north, Z=+44, faces south) ── */
    drawStage(0,44);
    drawLampPost(-14,30);drawLampPost(14,30);drawLampPost(-14,38);drawLampPost(14,38);
    drawBench(-14,26,0);drawBench(14,26,0);drawBench(-14,32,0);drawBench(14,32,0);
    drawBalloonStall(24,30,270);drawBalloonStall(-22,30,90);
    drawTrashBin(-11,28,0.62f,0.18f,0.88f);drawTrashBin(12,28,0.62f,0.18f,0.88f);

    /* ── GIANT FERRIS WHEEL  — centred at (46, 0, -32) ── */
    drawGiantWheel(47,-48);   /* WCX=46, WCZ=-32 — dead centre of its zone box X=22..65,Z=-58..-6 */

    /* Wheel zone access path (from E cross road to wheel) */
    drawStonePath(22,-3,44,-3,5.0f);
    drawStonePath(22, 3,44, 3,5.0f);

    drawLampPost(30,-22);drawLampPost(44,-14);drawLampPost(58,-14);drawLampPost(58,-44);
    drawBench(32,-18,90);drawBench(56,-28,0);drawBench(44,-14,0);
    drawTrashBin(34,-16,0.28f,0.48f,0.78f);drawTrashBin(58,-42,0.28f,0.48f,0.78f);

    /* ── MERRY-GO-ROUND (NE, away from wheel) ── */
    drawMerryGoRound(32,-8,merryAngle);
    drawLampPost(30,-18);drawLampPost(42,-4);
    drawBench(26,-10,0);drawBench(40,-4,90);

    /* ── ROLLER COASTER (NW) ── */
    drawRollerCoaster(-40,44);
    drawLampPost(-22,38);drawLampPost(-55,38);
    drawBench(-22,28,90);drawBench(-55,28,90);

    /* ═══ FOOD COURT (SW quadrant) ═══
       3 stalls side by side, vendor behind each, big tables in front */
    /* Food court paved floor */
    glDisable(GL_LIGHTING);
    glColor3f(0.76f,0.72f,0.62f);
    glBegin(GL_QUADS);
    glVertex3f(-62,0.01f,-62);glVertex3f(-20,0.01f,-62);
    glVertex3f(-20,0.01f,-10);glVertex3f(-62,0.01f,-10);
    glEnd();
    /* tile grid */
    glColor3f(0.62f,0.58f,0.48f);
    for(float tx=-62;tx<-20;tx+=4){
        glBegin(GL_LINES);glVertex3f(tx,0.015f,-62);glVertex3f(tx,0.015f,-10);glEnd();}
    for(float tz=-62;tz<-10;tz+=4){
        glBegin(GL_LINES);glVertex3f(-62,0.015f,tz);glVertex3f(-20,0.015f,tz);glEnd();}
    glEnable(GL_LIGHTING);

    /* 3 food stalls side by side */
    drawPizzaStall(-52,-42, 90);    /* pizza — leftmost, facing east (+X) */
    drawBurgerStall(-42,-42, 90);   /* burger — middle */
    drawIceCreamStall(-32,-42, 90); /* ice cream — rightmost */

    /* big dining tables in front of stalls */
    float dtc[][3]={{0.92f,0.22f,0.22f},{0.22f,0.52f,0.92f},{0.22f,0.78f,0.28f},
                    {0.92f,0.72f,0.10f},{0.78f,0.18f,0.82f},{0.92f,0.42f,0.18f},
                    {0.18f,0.72f,0.82f},{0.92f,0.28f,0.48f},{0.58f,0.82f,0.22f},
                    {0.88f,0.58f,0.10f},{0.28f,0.48f,0.88f},{0.88f,0.88f,0.18f}};
    for(int r=0;r<3;r++) for(int c=0;c<4;c++)
        drawBigDiningTable(-54+c*7.0f,-28+r*7.0f,dtc[r*4+c][0],dtc[r*4+c][1],dtc[r*4+c][2]);

    drawLampPost(-54,-52);drawLampPost(-38,-52);drawLampPost(-24,-52);
    drawLampPost(-54,-18);drawLampPost(-38,-18);drawLampPost(-24,-18);
    drawTrashBin(-58,-30,0.88f,0.48f,0.10f);drawTrashBin(-22,-30,0.88f,0.48f,0.10f);
    drawTrashBin(-58,-48,0.88f,0.48f,0.10f);drawTrashBin(-22,-48,0.88f,0.48f,0.10f);

    /* ── GAME STALLS (NW) ── */
    drawBalloonStall(-38,18,180);drawBalloonStall(-52,18,90);
    drawBalloonStall(-38,30,180);drawBalloonStall(-52,30,90);
    drawLampPost(-32,22);drawLampPost(-58,22);drawLampPost(-32,34);drawLampPost(-58,34);
    drawBench(-30,24,0);drawBench(-58,26,90);
    drawTrashBin(-30,36,0.18f,0.68f,0.28f);drawTrashBin(-58,36,0.18f,0.68f,0.28f);

    /* ── FLOATING BALLOONS ── */
    float bC[][3]={{1,.10f,.10f},{.10f,.40f,1},{.92f,.86f,.05f},{.10f,.76f,.20f},{.86f,.20f,.86f},{1,.5f,.1f}};
    float bP[][2]={{22,-22},{-26,26},{12,42},{-36,-12},{36,12},{-10,-35}};
    for(int b=0;b<6;b++){float sw=sinf(windTime+b*1.3f)*.32f;
        drawBalloon(bP[b][0],10,bP[b][1],bC[b][0],bC[b][1],bC[b][2],sw);}

    /* ── PARKING LOT ── */
    drawParkingLot();

    glutSwapBuffers();
}

/* ─── TIMER ─── */
void timerFunc(int){
    if(wheelSpinning){wheelAngle+=.45f;if(wheelAngle>=360)wheelAngle-=360;}
    if(coasterRunning){coasterProg+=1.5f;if(coasterProg>=360)coasterProg-=360;}
    if(merryRunning){merryAngle+=1.2f;if(merryAngle>=360)merryAngle-=360;}
    windTime+=.038f;fountainTime+=.06f;ledTime+=0.055f;
    spotAngle+=0.5f;if(spotAngle>=360)spotAngle-=360;
    glutPostRedisplay();
    glutTimerFunc(16,timerFunc,0);
}

/* ─── INPUT ─── */
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

/* ─── INIT + MAIN ─── */
void init(){
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);
    glClearColor(.38f,.65f,.95f,1);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glEnable(GL_LINE_SMOOTH);glEnable(GL_NORMALIZE);
    setupLighting();
    int tw,th;
    unsigned char*td=loadBMP("grass.bmp",&tw,&th);
    if(td){
        glGenTextures(1,&groundTex);glBindTexture(GL_TEXTURE_2D,groundTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,tw,th,GL_RGB,GL_UNSIGNED_BYTE,td);
        delete[]td;printf("grass.bmp loaded %dx%d\n",tw,th);
    } else {
        printf("No grass.bmp — using solid green grass\n");
    }
}

int main(int argc,char**argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(1280,800);
    glutCreateWindow("FUNLAND 3D — Final");
    init();
    glutDisplayFunc(display);glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseBtn);glutMotionFunc(mouseMove);
    glutTimerFunc(16,timerFunc,0);
    glutMainLoop();return 0;
}