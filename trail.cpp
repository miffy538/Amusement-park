#define GL_SILENCE_DEPRECATION
#include <GL/glut.h>
#include <cmath>
#include <ctime>

float angleY   = 20.0f;
float angleX   = 25.0f;
float zoomDist = 100.0f;

int  lastMouseX = -1, lastMouseY = -1;
bool mouseDown  = false;

float merryAngle = 0.0f;
float windTime   = 0.0f;
float walkTime   = 0.0f;

/* =============================
   GROUND
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
   ROAD
============================= */
void drawRoad(float x1, float z1, float x2, float z2)
{
    glColor3f(0.22f, 0.22f, 0.22f);
    glBegin(GL_QUADS);
    glVertex3f(x1, 0.03f, z1); glVertex3f(x2, 0.03f, z1);
    glVertex3f(x2, 0.03f, z2); glVertex3f(x1, 0.03f, z2);
    glEnd();
    bool  horiz = fabsf(x2 - x1) > fabsf(z2 - z1);
    float cx = (x1+x2)*0.5f, cz = (z1+z2)*0.5f;
    float len = horiz ? fabsf(x2-x1) : fabsf(z2-z1);
    int   dashes = (int)(len / 5);
    glColor3f(0.9f, 0.8f, 0.0f);
    for (int d = 0; d < dashes; d += 2) {
        glBegin(GL_QUADS);
        if (horiz) {
            float xa=x1+d*5.0f, xb=xa+4.0f;
            glVertex3f(xa,0.04f,cz-0.15f); glVertex3f(xb,0.04f,cz-0.15f);
            glVertex3f(xb,0.04f,cz+0.15f); glVertex3f(xa,0.04f,cz+0.15f);
        } else {
            float za=z1+d*5.0f, zb=za+4.0f;
            glVertex3f(cx-0.15f,0.04f,za); glVertex3f(cx+0.15f,0.04f,za);
            glVertex3f(cx+0.15f,0.04f,zb); glVertex3f(cx-0.15f,0.04f,zb);
        }
        glEnd();
    }
}

/* =============================
   TREE
============================= */
void drawTree(float x, float z)
{
    glPushMatrix(); glTranslatef(x,0,z);
    glColor3f(0.42f,0.27f,0.12f);
    glPushMatrix(); glTranslatef(0,1.5f,0); glRotatef(-90,1,0,0);
    GLUquadric*q=gluNewQuadric(); gluCylinder(q,0.25,0.18,3.0,8,1); gluDeleteQuadric(q);
    glPopMatrix();
    for(int layer=0;layer<3;layer++){
        glColor3f(0.12f,0.55f-layer*0.04f,0.18f);
        glPushMatrix(); glTranslatef(0,3.0f+layer*1.6f,0); glRotatef(-90,1,0,0);
        GLUquadric*qc=gluNewQuadric(); gluCylinder(qc,1.5f-layer*0.35f,0.0f,2.0f,8,1); gluDeleteQuadric(qc);
        glPopMatrix();
    }
    glPopMatrix();
}

/* =============================
   FENCE
============================= */
void drawFenceSegment(float x1,float z1,float x2,float z2)
{
    float dx=x2-x1,dz=z2-z1,len=sqrtf(dx*dx+dz*dz);
    float ang=atan2f(dx,dz)*180.0f/(float)M_PI;
    int posts=(int)(len/3.0f)+1;
    glPushMatrix(); glTranslatef(x1,0,z1); glRotatef(ang,0,1,0);
    for(int p=0;p<posts;p++){
        float px=p*(len/(posts-1));
        glColor3f(0.65f,0.45f,0.20f);
        glPushMatrix(); glTranslatef(0,1.0f,px); glRotatef(-90,1,0,0);
        GLUquadric*q=gluNewQuadric(); gluCylinder(q,0.08,0.08,2.0,6,1); gluDeleteQuadric(q);
        glPopMatrix();
        glColor3f(0.55f,0.35f,0.12f);
        glPushMatrix(); glTranslatef(0,3.1f,px); glutSolidSphere(0.12,6,6); glPopMatrix();
    }
    glColor3f(0.60f,0.42f,0.18f);
    for(int r=0;r<2;r++){
        float ry=(r==0)?1.6f:2.6f;
        glBegin(GL_QUADS);
        glVertex3f(-0.06f,ry,0); glVertex3f(0.06f,ry,0);
        glVertex3f(0.06f,ry,len); glVertex3f(-0.06f,ry,len);
        glEnd();
    }
    glPopMatrix();
}

/* =============================
   BALLOON
============================= */
void drawBalloon(float x,float y,float z,float r,float g,float b,float sway)
{
    glPushMatrix(); glTranslatef(x+sway,y,z);
    glColor3f(0.8f,0.8f,0.8f);
    glBegin(GL_LINES); glVertex3f(0,0,0); glVertex3f(0,-2.2f,0); glEnd();
    glColor3f(r,g,b); glutSolidSphere(0.55,12,12);
    glColor3f(fminf(r+0.4f,1.f),fminf(g+0.4f,1.f),fminf(b+0.4f,1.f));
    glPushMatrix(); glTranslatef(-0.15f,0.18f,0.35f); glutSolidSphere(0.10,6,6); glPopMatrix();
    glPopMatrix();
}

/* =============================
   BALLOON STALL
============================= */
void drawBalloonStall(float cx,float cz,float angle,float animTime)
{
    glPushMatrix(); glTranslatef(cx,0,cz); glRotatef(angle,0,1,0);
    glColor3f(0.55f,0.33f,0.11f);
    glPushMatrix(); glScalef(4.0f,1.0f,2.0f); glTranslatef(0,0.5f,0); glutSolidCube(1.0); glPopMatrix();
    glColor3f(0.85f,0.75f,0.55f);
    glPushMatrix(); glTranslatef(0,1.1f,0); glScalef(4.2f,0.12f,2.2f); glutSolidCube(1.0); glPopMatrix();
    float px[]={-1.8f,1.8f,-1.8f,1.8f},pzz[]={-0.9f,-0.9f,0.9f,0.9f};
    for(int i=0;i<4;i++){
        glColor3f(0.80f,0.10f,0.10f);
        glPushMatrix(); glTranslatef(px[i],1.1f,pzz[i]); glRotatef(-90,1,0,0);
        GLUquadric*q=gluNewQuadric(); gluCylinder(q,0.09,0.09,3.5,6,1); gluDeleteQuadric(q);
        glPopMatrix();
    }
    int stripes=8; float cw=4.0f/stripes,roofY=4.7f;
    for(int s=0;s<stripes;s++){
        if(s%2==0)glColor3f(0.90f,0.12f,0.12f); else glColor3f(0.98f,0.98f,0.98f);
        float xs=-2.0f+s*cw;
        glBegin(GL_QUADS);
        glVertex3f(xs,roofY,-1.1f); glVertex3f(xs+cw,roofY,-1.1f);
        glVertex3f(xs+cw,roofY-0.5f,-1.6f); glVertex3f(xs,roofY-0.5f,-1.6f); glEnd();
        glBegin(GL_QUADS);
        glVertex3f(xs,roofY,1.1f); glVertex3f(xs+cw,roofY,1.1f);
        glVertex3f(xs+cw,roofY-0.5f,1.6f); glVertex3f(xs,roofY-0.5f,1.6f); glEnd();
        glBegin(GL_QUADS);
        glVertex3f(xs,roofY,-1.1f); glVertex3f(xs+cw,roofY,-1.1f);
        glVertex3f(xs+cw,roofY,1.1f); glVertex3f(xs,roofY,1.1f); glEnd();
    }
    float bcolors[6][3]={{1.0f,0.2f,0.2f},{0.2f,0.5f,1.0f},{1.0f,0.9f,0.1f},
                         {0.2f,0.85f,0.3f},{1.0f,0.45f,0.05f},{0.75f,0.2f,0.9f}};
    for(int b=0;b<5;b++){
        float bx=-1.5f+(b%3)*0.55f,bz2=-0.3f+(b/3)*0.6f,by=1.1f+1.8f+(b%2)*0.4f;
        float sw=sinf(animTime*1.2f+b)*0.12f;
        drawBalloon(bx,by,bz2,bcolors[b%6][0],bcolors[b%6][1],bcolors[b%6][2],sw);
    }
    for(int b=0;b<5;b++){
        float bx=0.8f+(b%3)*0.55f,bz2=-0.3f+(b/3)*0.6f,by=1.1f+1.8f+(b%2)*0.4f;
        float sw=sinf(animTime*1.0f+b+2.0f)*0.12f;
        drawBalloon(bx,by,bz2,bcolors[(b+2)%6][0],bcolors[(b+2)%6][1],bcolors[(b+2)%6][2],sw);
    }
    glPopMatrix();
}

/* =============================
   MERRY-GO-ROUND
============================= */
void drawMerryGoRound(float cx,float cz,float spinAngle)
{
    glPushMatrix(); glTranslatef(cx,0,cz);
    glColor3f(0.65f,0.20f,0.60f);
    glPushMatrix(); glRotatef(-90,1,0,0);
    GLUquadric*disk=gluNewQuadric(); gluDisk(disk,0.0,5.5,32,1); gluDeleteQuadric(disk); glPopMatrix();
    glColor3f(0.85f,0.85f,0.10f);
    glPushMatrix(); glTranslatef(0,0.05f,0); glRotatef(-90,1,0,0);
    GLUquadric*rim=gluNewQuadric(); gluCylinder(rim,5.4,5.4,0.30,32,1); gluDeleteQuadric(rim); glPopMatrix();
    float ringColors[3][3]={{0.9f,0.2f,0.5f},{0.2f,0.6f,0.9f},{0.9f,0.7f,0.1f}};
    for(int r=0;r<3;r++){
        glColor3f(ringColors[r][0],ringColors[r][1],ringColors[r][2]);
        glPushMatrix(); glTranslatef(0,0.35f+r*0.01f,0); glRotatef(-90,1,0,0);
        GLUquadric*rq=gluNewQuadric(); gluCylinder(rq,5.3f-r*1.6f,5.3f-r*1.6f,0.02f,32,1); gluDeleteQuadric(rq); glPopMatrix();
    }
    glColor3f(0.80f,0.80f,0.80f);
    glPushMatrix(); glRotatef(-90,1,0,0);
    GLUquadric*pole=gluNewQuadric(); gluCylinder(pole,0.25,0.25,8.0,12,1); gluDeleteQuadric(pole); glPopMatrix();
    glColor3f(1.0f,0.85f,0.0f);
    glPushMatrix(); glTranslatef(0,8.2f,0); glutSolidSphere(0.5,12,12); glPopMatrix();

    glPushMatrix(); glRotatef(spinAngle,0,1,0);
    int cSegments=12; float innerR=0.3f,outerR=5.2f,canopyY=7.5f;
    for(int s=0;s<cSegments;s++){
        float a0=s*(360.0f/cSegments)*(float)M_PI/180.0f;
        float a1=(s+1)*(360.0f/cSegments)*(float)M_PI/180.0f;
        if(s%2==0)glColor3f(0.95f,0.15f,0.45f); else glColor3f(0.98f,0.98f,0.98f);
        float dip=0.8f;
        glBegin(GL_TRIANGLES);
        glVertex3f(innerR*cosf(a0),canopyY,innerR*sinf(a0));
        glVertex3f(outerR*cosf(a0),canopyY-dip,outerR*sinf(a0));
        glVertex3f(outerR*cosf(a1),canopyY-dip,outerR*sinf(a1));
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex3f(innerR*cosf(a0),canopyY,innerR*sinf(a0));
        glVertex3f(outerR*cosf(a1),canopyY-dip,outerR*sinf(a1));
        glVertex3f(innerR*cosf(a1),canopyY,innerR*sinf(a1));
        glEnd();
    }
    for(int s=0;s<cSegments*2;s++){
        float a0=s*(180.0f/cSegments)*(float)M_PI/180.0f;
        float a1=(s+0.5f)*(180.0f/cSegments)*(float)M_PI/180.0f;
        float a2=(s+1)*(180.0f/cSegments)*(float)M_PI/180.0f;
        glColor3f(1.0f,0.85f,0.0f);
        glBegin(GL_TRIANGLES);
        glVertex3f(outerR*cosf(a0),canopyY-0.8f,outerR*sinf(a0));
        glVertex3f((outerR+0.6f)*cosf(a1),canopyY-1.4f,(outerR+0.6f)*sinf(a1));
        glVertex3f(outerR*cosf(a2),canopyY-0.8f,outerR*sinf(a2));
        glEnd();
    }
    for(int sp=0;sp<6;sp++){
        float sa=sp*(60.0f)*(float)M_PI/180.0f;
        glColor3f(0.80f,0.80f,0.80f);
        glBegin(GL_LINES);
        glVertex3f(0,7.8f,0);
        glVertex3f(outerR*cosf(sa),canopyY-0.7f,outerR*sinf(sa));
        glEnd();
    }
    float horseColors[6][3]={{0.95f,0.90f,0.85f},{0.60f,0.35f,0.15f},{0.20f,0.20f,0.20f},
                              {0.95f,0.75f,0.50f},{0.55f,0.25f,0.10f},{0.85f,0.85f,0.80f}};
    for(int h=0;h<6;h++){
        float ha=h*(60.0f)*(float)M_PI/180.0f;
        float hx=3.8f*cosf(ha),hz=3.8f*sinf(ha);
        float hy=1.0f+0.7f*sinf(spinAngle*(float)M_PI/180.0f*3.0f+h*1.05f);
        glPushMatrix(); glTranslatef(hx,hy,hz);
        glRotatef(-ha*180.0f/(float)M_PI+90.0f,0,1,0);
        float*hc=horseColors[h];
        glColor3f(0.85f,0.75f,0.20f);
        glPushMatrix(); glRotatef(-90,1,0,0);
        GLUquadric*hp=gluNewQuadric(); gluCylinder(hp,0.07,0.07,6.5f-hy,6,1); gluDeleteQuadric(hp); glPopMatrix();
        glColor3f(hc[0],hc[1],hc[2]);
        glPushMatrix(); glScalef(0.6f,0.45f,1.1f); glutSolidSphere(1.0,10,10); glPopMatrix();
        glPushMatrix(); glTranslatef(0,0.4f,0.7f); glRotatef(40,1,0,0); glScalef(0.32f,0.42f,0.65f); glutSolidSphere(1.0,8,8); glPopMatrix();
        glPushMatrix(); glTranslatef(0,0.85f,1.0f); glScalef(0.25f,0.28f,0.42f); glutSolidSphere(1.0,8,8); glPopMatrix();
        glColor3f(0.85f,0.60f,0.20f);
        for(int m=0;m<4;m++){glPushMatrix();glTranslatef(0,0.75f+m*0.12f,0.55f+m*0.08f);glutSolidSphere(0.10,5,5);glPopMatrix();}
        glColor3f(0.70f,0.10f,0.10f);
        glPushMatrix(); glTranslatef(0,0.5f,0); glScalef(0.5f,0.15f,0.7f); glutSolidSphere(1.0,8,8); glPopMatrix();
        float legX[]={-0.3f,0.3f,-0.3f,0.3f},legZZ[]={-0.5f,-0.5f,0.5f,0.5f};
        for(int l=0;l<4;l++){
            glColor3f(hc[0]*0.85f,hc[1]*0.85f,hc[2]*0.85f);
            glPushMatrix(); glTranslatef(legX[l],-0.45f,legZZ[l]); glRotatef(-90,1,0,0);
            GLUquadric*lq=gluNewQuadric(); gluCylinder(lq,0.09,0.07,0.7f,6,1); gluDeleteQuadric(lq); glPopMatrix();
        }
        glColor3f(0.85f,0.60f,0.20f);
        for(int t=0;t<3;t++){glPushMatrix();glTranslatef(0,0.1f-t*0.12f,-1.05f-t*0.08f);glutSolidSphere(0.11,5,5);glPopMatrix();}
        glPopMatrix();
    }
    glPopMatrix();
    glPopMatrix();
}

/* =============================
   TICKET BOOTH
============================= */
void drawTicketBooth(float cx,float cz,float rotY)
{
    glPushMatrix(); glTranslatef(cx,0,cz); glRotatef(rotY,0,1,0);
    glColor3f(0.25f,0.45f,0.85f);
    glPushMatrix(); glScalef(2.5f,3.5f,2.0f); glTranslatef(0,0.5f,0); glutSolidCube(1.0); glPopMatrix();
    glColor3f(0.90f,0.15f,0.15f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-1.4f,3.5f,1.1f); glVertex3f(1.4f,3.5f,1.1f); glVertex3f(0,5.0f,0);
    glVertex3f(-1.4f,3.5f,-1.1f);glVertex3f(1.4f,3.5f,-1.1f);glVertex3f(0,5.0f,0);
    glVertex3f(-1.4f,3.5f,-1.1f);glVertex3f(-1.4f,3.5f,1.1f);glVertex3f(0,5.0f,0);
    glVertex3f(1.4f,3.5f,-1.1f); glVertex3f(1.4f,3.5f,1.1f); glVertex3f(0,5.0f,0);
    glEnd();
    glColor3f(0.85f,0.95f,1.00f);
    glPushMatrix(); glTranslatef(0,1.8f,1.01f); glScalef(1.2f,0.8f,0.05f); glutSolidCube(1.0); glPopMatrix();
    glColor3f(1.0f,0.85f,0.05f);
    glPushMatrix(); glTranslatef(0,2.8f,1.02f); glScalef(2.0f,0.5f,0.05f); glutSolidCube(1.0); glPopMatrix();
    glColor3f(0.55f,0.33f,0.10f);
    glPushMatrix(); glTranslatef(0,1.4f,1.25f); glScalef(1.6f,0.12f,0.5f); glutSolidCube(1.0); glPopMatrix();
    float bt[3][3]={{1.0f,0.2f,0.2f},{0.2f,0.5f,1.0f},{1.0f,0.85f,0.0f}};
    for(int b=0;b<3;b++){
        float sw=sinf(windTime*1.5f+b)*0.15f;
        drawBalloon(-0.7f+b*0.7f,5.5f,0.0f,bt[b][0],bt[b][1],bt[b][2],sw);
    }
    glPopMatrix();
}

/* =============================
   ENTRANCE ARCH
============================= */
void drawEntranceArch(float cx,float cz)
{
    glPushMatrix(); glTranslatef(cx,0,cz);
    float pw=0.6f,ph=8.0f,gap=7.0f;
    glColor3f(0.90f,0.90f,0.90f);
    glPushMatrix(); glTranslatef(-gap*0.5f,ph*0.5f,0); glScalef(pw,ph,pw); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef( gap*0.5f,ph*0.5f,0); glScalef(pw,ph,pw); glutSolidCube(1.0); glPopMatrix();
    int archSegs=16; float archR=gap*0.5f+pw*0.5f,archT=0.55f;
    float archCols[7][3]={{1,0,0},{1,0.5f,0},{1,1,0},{0,0.8f,0},{0,0.5f,1},{0.3f,0,1},{0.8f,0,0.8f}};
    for(int s=0;s<archSegs;s++){
        float a0=s*(float)M_PI/archSegs, a1=(s+1)*(float)M_PI/archSegs;
        glColor3f(archCols[s%7][0],archCols[s%7][1],archCols[s%7][2]);
        glBegin(GL_QUADS);
        glVertex3f((archR-archT)*cosf(a0),ph+(archR-archT)*sinf(a0),-0.25f);
        glVertex3f(archR*cosf(a0),ph+archR*sinf(a0),-0.25f);
        glVertex3f(archR*cosf(a1),ph+archR*sinf(a1),-0.25f);
        glVertex3f((archR-archT)*cosf(a1),ph+(archR-archT)*sinf(a1),-0.25f);
        glVertex3f((archR-archT)*cosf(a0),ph+(archR-archT)*sinf(a0),0.25f);
        glVertex3f(archR*cosf(a0),ph+archR*sinf(a0),0.25f);
        glVertex3f(archR*cosf(a1),ph+archR*sinf(a1),0.25f);
        glVertex3f((archR-archT)*cosf(a1),ph+(archR-archT)*sinf(a1),0.25f);
        glEnd();
    }
    glColor3f(1.0f,0.85f,0.0f);
    glPushMatrix(); glTranslatef(-gap*0.5f,ph+0.4f,0); glutSolidSphere(0.5,10,10); glPopMatrix();
    glPushMatrix(); glTranslatef( gap*0.5f,ph+0.4f,0); glutSolidSphere(0.5,10,10); glPopMatrix();
    glPopMatrix();
}

/* =============================
   BENCH
============================= */
void drawBench(float x,float z,float rotY)
{
    glPushMatrix(); glTranslatef(x,0,z); glRotatef(rotY,0,1,0);
    glColor3f(0.35f,0.22f,0.08f);
    float lx[]={-0.8f,0.8f,-0.8f,0.8f},lz[]={-0.3f,-0.3f,0.3f,0.3f};
    for(int i=0;i<4;i++){glPushMatrix();glTranslatef(lx[i],0.5f,lz[i]);glScalef(0.12f,1.0f,0.12f);glutSolidCube(1.0);glPopMatrix();}
    glColor3f(0.60f,0.40f,0.15f);
    glPushMatrix(); glTranslatef(0,1.1f,0); glScalef(1.8f,0.1f,0.7f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(0,1.7f,-0.28f); glScalef(1.8f,0.55f,0.1f); glutSolidCube(1.0); glPopMatrix();
    glPopMatrix();
}

/* =============================
   LAMP POST
============================= */
void drawLampPost(float x,float z)
{
    glPushMatrix(); glTranslatef(x,0,z);
    glColor3f(0.25f,0.25f,0.35f);
    glRotatef(-90,1,0,0);
    GLUquadric*lp=gluNewQuadric(); gluCylinder(lp,0.12,0.08,7.0,8,1); gluDeleteQuadric(lp);
    glTranslatef(0,0,7.0f); glTranslatef(0.5f,0,0);
    glColor3f(0.20f,0.20f,0.28f); glutSolidSphere(0.35,8,8);
    glColor3f(1.0f,0.98f,0.75f); glTranslatef(0,0,0.2f); glutSolidSphere(0.18,6,6);
    glPopMatrix();
}

/* ============================================================
   CAR
============================================================ */
void drawCar(float x, float z, float facing, float r, float g, float b)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(facing, 0, 1, 0);

    /* body */
    glColor3f(r, g, b);
    glPushMatrix(); glTranslatef(0,0.55f,0); glScalef(2.0f,0.7f,3.8f); glutSolidCube(1.0); glPopMatrix();
    /* cabin */
    glColor3f(r*0.75f,g*0.75f,b*0.75f);
    glPushMatrix(); glTranslatef(0,1.15f,0.1f); glScalef(1.6f,0.55f,2.2f); glutSolidCube(1.0); glPopMatrix();
    /* windshields */
    glColor3f(0.7f,0.88f,0.95f);
    glPushMatrix(); glTranslatef(0,1.05f, 1.22f); glScalef(1.4f,0.45f,0.05f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(0,1.05f,-1.15f); glScalef(1.4f,0.45f,0.05f); glutSolidCube(1.0); glPopMatrix();
    /* headlights */
    glColor3f(1.0f,0.97f,0.80f);
    glPushMatrix(); glTranslatef(-0.55f,0.55f, 1.92f); glutSolidSphere(0.18,6,6); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.55f,0.55f, 1.92f); glutSolidSphere(0.18,6,6); glPopMatrix();
    /* tail lights */
    glColor3f(0.9f,0.1f,0.1f);
    glPushMatrix(); glTranslatef(-0.55f,0.55f,-1.92f); glutSolidSphere(0.15,6,6); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.55f,0.55f,-1.92f); glutSolidSphere(0.15,6,6); glPopMatrix();
    /* wheels */
    float wx[]={-1.02f,1.02f,-1.02f,1.02f};
    float wz2[]={1.20f,1.20f,-1.20f,-1.20f};
    for(int w=0;w<4;w++){
        glColor3f(0.15f,0.15f,0.15f);
        glPushMatrix(); glTranslatef(wx[w],0.32f,wz2[w]); glRotatef(90,0,1,0);
        GLUquadric*wq=gluNewQuadric(); gluCylinder(wq,0.32,0.32,0.22,12,1); gluDeleteQuadric(wq);
        glColor3f(0.60f,0.60f,0.65f);
        GLUquadric*wq2=gluNewQuadric(); gluDisk(wq2,0,0.28,10,1); gluDeleteQuadric(wq2);
        glPopMatrix();
    }
    glPopMatrix();
}

/* ============================================================
   PARKING LOT  (SE corner: x 30..68, z 30..68)
============================================================ */
void drawParkingLot()
{
    /* asphalt base */
    glColor3f(0.20f,0.20f,0.20f);
    glBegin(GL_QUADS);
    glVertex3f(30.0f,0.02f,30.0f); glVertex3f(68.0f,0.02f,30.0f);
    glVertex3f(68.0f,0.02f,68.0f); glVertex3f(30.0f,0.02f,68.0f);
    glEnd();

    /* bay divider lines — row 1 */
    glColor3f(0.92f,0.92f,0.92f);
    for(int b=0;b<=6;b++){
        float bx=33.0f+b*5.5f;
        glBegin(GL_QUADS);
        glVertex3f(bx,     0.05f,34.0f); glVertex3f(bx+0.12f,0.05f,34.0f);
        glVertex3f(bx+0.12f,0.05f,42.0f);glVertex3f(bx,      0.05f,42.0f);
        glEnd();
    }
    /* bay end lines */
    glBegin(GL_QUADS);
    glVertex3f(33.0f,0.05f,34.0f);glVertex3f(66.0f,0.05f,34.0f);
    glVertex3f(66.0f,0.05f,34.12f);glVertex3f(33.0f,0.05f,34.12f);
    glEnd();
    glBegin(GL_QUADS);
    glVertex3f(33.0f,0.05f,42.0f);glVertex3f(66.0f,0.05f,42.0f);
    glVertex3f(66.0f,0.05f,42.12f);glVertex3f(33.0f,0.05f,42.12f);
    glEnd();

    /* bay divider lines — row 2 */
    for(int b=0;b<=6;b++){
        float bx=33.0f+b*5.5f;
        glBegin(GL_QUADS);
        glVertex3f(bx,      0.05f,51.0f); glVertex3f(bx+0.12f,0.05f,51.0f);
        glVertex3f(bx+0.12f,0.05f,59.0f); glVertex3f(bx,      0.05f,59.0f);
        glEnd();
    }
    glBegin(GL_QUADS);
    glVertex3f(33.0f,0.05f,51.0f);glVertex3f(66.0f,0.05f,51.0f);
    glVertex3f(66.0f,0.05f,51.12f);glVertex3f(33.0f,0.05f,51.12f);
    glEnd();
    glBegin(GL_QUADS);
    glVertex3f(33.0f,0.05f,59.0f);glVertex3f(66.0f,0.05f,59.0f);
    glVertex3f(66.0f,0.05f,59.12f);glVertex3f(33.0f,0.05f,59.12f);
    glEnd();

    /* centre aisle yellow stripe */
    glColor3f(0.85f,0.78f,0.0f);
    glBegin(GL_QUADS);
    glVertex3f(47.5f,0.05f,43.0f);glVertex3f(48.0f,0.05f,43.0f);
    glVertex3f(48.0f,0.05f,50.5f);glVertex3f(47.5f,0.05f,50.5f);
    glEnd();

    /* cars row 1 */
    float cr1[6][3]={{0.85f,0.12f,0.12f},{0.15f,0.35f,0.80f},{0.15f,0.60f,0.20f},
                     {0.85f,0.75f,0.10f},{0.60f,0.18f,0.65f},{0.80f,0.80f,0.80f}};
    for(int c=0;c<6;c++) drawCar(35.5f+c*5.5f, 37.5f, 0.0f, cr1[c][0],cr1[c][1],cr1[c][2]);

    /* cars row 2 (4 of 6 bays occupied) */
    float cr2[4][3]={{0.90f,0.50f,0.10f},{0.30f,0.30f,0.30f},{0.10f,0.55f,0.75f},{0.75f,0.25f,0.25f}};
    for(int c=0;c<4;c++) drawCar(35.5f+c*5.5f, 54.5f, 180.0f, cr2[c][0],cr2[c][1],cr2[c][2]);

    /* parking lot lamp posts */
    drawLampPost(35.0f,46.5f);
    drawLampPost(50.0f,46.5f);
    drawLampPost(64.5f,46.5f);
}

/* ============================================================
   PARKING ENTRY GATE  (south edge of lot, x≈40, z≈30)
============================================================ */
void drawParkingGate()
{
    float gx=41.0f, gz=31.0f;
    glPushMatrix(); glTranslatef(gx,0,gz);

    /* short connecting lane */
    glColor3f(0.20f,0.20f,0.20f);
    glBegin(GL_QUADS);
    glVertex3f(-4.5f,0.02f, 0.5f); glVertex3f(4.5f,0.02f, 0.5f);
    glVertex3f( 4.5f,0.02f,-2.5f); glVertex3f(-4.5f,0.02f,-2.5f);
    glEnd();

    /* lane arrows (yellow triangles) */
    glColor3f(0.9f,0.8f,0.0f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.4f,0.05f,-1.8f); glVertex3f(0.4f,0.05f,-1.8f); glVertex3f(0,0.05f,-0.5f);
    glEnd();

    /* guard booth */
    glColor3f(0.35f,0.60f,0.35f);
    glPushMatrix(); glTranslatef(-3.0f,1.1f,0); glScalef(1.3f,2.2f,1.3f); glutSolidCube(1.0); glPopMatrix();
    /* booth roof */
    glColor3f(0.18f,0.40f,0.18f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-3.7f,2.2f,-0.75f);glVertex3f(-2.3f,2.2f,-0.75f);glVertex3f(-3.0f,3.1f, 0);
    glVertex3f(-3.7f,2.2f, 0.75f);glVertex3f(-2.3f,2.2f, 0.75f);glVertex3f(-3.0f,3.1f, 0);
    glVertex3f(-3.7f,2.2f,-0.75f);glVertex3f(-3.7f,2.2f, 0.75f);glVertex3f(-3.0f,3.1f, 0);
    glVertex3f(-2.3f,2.2f,-0.75f);glVertex3f(-2.3f,2.2f, 0.75f);glVertex3f(-3.0f,3.1f, 0);
    glEnd();
    /* booth window */
    glColor3f(0.75f,0.92f,1.0f);
    glPushMatrix(); glTranslatef(-2.33f,1.2f,0); glScalef(0.05f,0.55f,0.65f); glutSolidCube(1.0); glPopMatrix();

    /* barrier base post */
    glColor3f(0.30f,0.30f,0.30f);
    glPushMatrix(); glTranslatef(0,0,0); glRotatef(-90,1,0,0);
    GLUquadric*bp=gluNewQuadric(); gluCylinder(bp,0.14,0.14,2.8,8,1); gluDeleteQuadric(bp); glPopMatrix();
    glColor3f(0.22f,0.22f,0.22f);
    glPushMatrix(); glTranslatef(0,2.9f,0); glutSolidSphere(0.20,8,8); glPopMatrix();

    /* boom arm — slightly raised */
    glPushMatrix(); glTranslatef(0,2.8f,0); glRotatef(-20,1,0,0);
    for(int s=0;s<8;s++){
        if(s%2==0) glColor3f(0.95f,0.18f,0.18f);
        else        glColor3f(0.98f,0.98f,0.98f);
        glPushMatrix(); glTranslatef(0,0,s*0.65f);
        glScalef(0.13f,0.13f,0.63f); glutSolidCube(1.0); glPopMatrix();
    }
    glPopMatrix();

    /* PARKING sign */
    glColor3f(0.10f,0.20f,0.65f);
    glPushMatrix(); glTranslatef(0,3.8f,0); glScalef(4.0f,0.65f,0.14f); glutSolidCube(1.0); glPopMatrix();
    glColor3f(1.0f,1.0f,0.0f);
    glPushMatrix(); glTranslatef(0,3.8f,0.08f); glScalef(3.2f,0.38f,0.02f); glutSolidCube(1.0); glPopMatrix();
    /* "P" symbol (circle + stem) */
    glColor3f(1.0f,1.0f,0.0f);
    glPushMatrix(); glTranslatef(-1.3f,3.8f,0.10f);
    glutSolidSphere(0.22f,8,8); glPopMatrix();

    glPopMatrix(); /* end gate */
}

/* ============================================================
   PERSON  (stick figure)
   action: 0=standing  1=sitting  2=walking  3=arms-up joy
============================================================ */
void drawPerson(float x, float z, float facing,
                int type, float sr, float sg, float sb,
                int action, float animPhase)
{
    float sc = (type==1) ? 0.65f : 1.0f;
    float legSwing=0, armSwing=0;
    if(action==2){ legSwing=sinf(animPhase)*25.0f;  armSwing=-sinf(animPhase)*20.0f; }
    if(action==3){ armSwing=-50.0f+sinf(animPhase)*15.0f; }

    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(facing,0,1,0);
    glScalef(sc,sc,sc);

    /* left leg */
    glColor3f(0.25f,0.25f,0.55f);
    glPushMatrix(); glTranslatef(-0.18f,0.9f,0);
    glRotatef(action==1?-70.0f:legSwing,1,0,0); glRotatef(-90,1,0,0);
    GLUquadric*lq=gluNewQuadric(); gluCylinder(lq,0.10,0.09,0.9f,6,1); gluDeleteQuadric(lq);
    glColor3f(0.18f,0.12f,0.08f); glTranslatef(0,0,0.9f); glutSolidSphere(0.13,6,6);
    glPopMatrix();
    /* right leg */
    glColor3f(0.25f,0.25f,0.55f);
    glPushMatrix(); glTranslatef(0.18f,0.9f,0);
    glRotatef(action==1?-70.0f:-legSwing,1,0,0); glRotatef(-90,1,0,0);
    GLUquadric*rq=gluNewQuadric(); gluCylinder(rq,0.10,0.09,0.9f,6,1); gluDeleteQuadric(rq);
    glColor3f(0.18f,0.12f,0.08f); glTranslatef(0,0,0.9f); glutSolidSphere(0.13,6,6);
    glPopMatrix();

    /* torso */
    glColor3f(sr,sg,sb);
    float torsoY=(action==1)?1.55f:1.30f;
    glPushMatrix(); glTranslatef(0,torsoY,0); glScalef(0.42f,0.70f,0.30f); glutSolidSphere(1.0,8,8); glPopMatrix();

    float shouldY=(action==1)?1.95f:1.75f;
    /* left arm */
    glColor3f(sr,sg,sb);
    glPushMatrix(); glTranslatef(-0.35f,shouldY,0);
    glRotatef(action==1?-10.0f:armSwing,1,0,0); glRotatef(-90,1,0,0);
    GLUquadric*aq=gluNewQuadric(); gluCylinder(aq,0.08,0.07,0.65f,6,1); gluDeleteQuadric(aq);
    glColor3f(0.88f,0.72f,0.55f); glTranslatef(0,0,0.65f); glutSolidSphere(0.10,6,6);
    glPopMatrix();
    /* right arm */
    glColor3f(sr,sg,sb);
    glPushMatrix(); glTranslatef(0.35f,shouldY,0);
    glRotatef(action==1?-10.0f:-armSwing,1,0,0); glRotatef(-90,1,0,0);
    GLUquadric*aq2=gluNewQuadric(); gluCylinder(aq2,0.08,0.07,0.65f,6,1); gluDeleteQuadric(aq2);
    glColor3f(0.88f,0.72f,0.55f); glTranslatef(0,0,0.65f); glutSolidSphere(0.10,6,6);
    glPopMatrix();

    /* neck */
    float neckY=(action==1)?2.25f:2.10f;
    glColor3f(0.88f,0.72f,0.55f);
    glPushMatrix(); glTranslatef(0,neckY,0); glRotatef(-90,1,0,0);
    GLUquadric*nq=gluNewQuadric(); gluCylinder(nq,0.09,0.09,0.22f,6,1); gluDeleteQuadric(nq); glPopMatrix();

    /* head */
    glPushMatrix(); glTranslatef(0,neckY+0.38f,0);
    glColor3f(0.88f,0.72f,0.55f); glutSolidSphere(0.26f,10,10);
    /* hair */
    glColor3f(0.22f+sr*0.08f,0.14f,0.05f);
    glPushMatrix(); glTranslatef(0,0.18f,0); glScalef(1.0f,0.5f,1.0f); glutSolidSphere(0.27f,8,8); glPopMatrix();
    /* eyes */
    glColor3f(0.08f,0.08f,0.08f);
    glPushMatrix(); glTranslatef(-0.10f,0.05f,0.23f); glutSolidSphere(0.04f,5,5); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.10f,0.05f,0.23f); glutSolidSphere(0.04f,5,5); glPopMatrix();
    glPopMatrix(); /* head */

    glPopMatrix(); /* person */
}

/* ============================================================
   PEOPLE SCENE  — families with kids + parents
============================================================ */
void drawPeopleScene()
{
    /* ---- Mum sitting on bench watching merry-go-round ---- */
    drawPerson(-17.5f,-5.5f,  85.0f, 0, 0.85f,0.30f,0.58f, 1, 0.0f);
    /* ---- Dad standing beside mum, hand on bench ---- */
    drawPerson(-15.8f,-5.8f, 100.0f, 0, 0.20f,0.35f,0.75f, 0, 0.0f);

    /* ---- Kid 1: orbiting the merry-go-round ---- */
    float k1ang = walkTime * 1.8f;
    drawPerson(-8.0f+8.8f*cosf(k1ang), -9.0f+8.8f*sinf(k1ang),
               90.0f-k1ang*180.0f/(float)M_PI,
               1, 1.0f,0.55f,0.10f, 2, walkTime*4.0f);

    /* ---- Kid 2 near balloon stall — jumping for joy ---- */
    drawPerson(23.5f,-14.5f, 200.0f, 1, 0.90f,0.28f,0.28f, 3, walkTime*3.0f);
    /* Parent watching kid 2 */
    drawPerson(25.5f,-13.8f, 215.0f, 0, 0.60f,0.20f,0.20f, 0, 0.0f);

    /* ---- Kids 3 & 4 running together toward MGR ---- */
    float k3x = -12.5f + sinf(walkTime*1.2f)*0.4f;
    drawPerson(k3x,     -2.5f, 185.0f, 1, 0.88f,0.88f,0.18f, 2, walkTime*3.5f);
    drawPerson(k3x+1.4f,-2.5f, 175.0f, 1, 0.28f,0.68f,0.90f, 2, walkTime*3.5f+1.0f);
    /* Their parent chasing / following slowly */
    drawPerson(k3x-1.0f, 0.0f, 185.0f, 0, 0.45f,0.22f,0.65f, 2, walkTime*2.5f);

    /* ---- Couple walking down the main path ---- */
    float coupleZ = fmodf(walkTime*3.5f, 60.0f) - 40.0f;
    drawPerson(-1.5f, coupleZ, 0.0f,  0, 0.38f,0.22f,0.62f, 2, walkTime*3.0f);
    drawPerson( 1.5f, coupleZ, 0.0f,  0, 0.85f,0.48f,0.22f, 2, walkTime*3.0f+0.5f);

    /* ---- Person at ticket window ---- */
    drawPerson(14.5f,-44.5f, 270.0f, 0, 0.70f,0.70f,0.18f, 0, 0.0f);
    /* Parent sitting on bench near entrance waiting ---- */
    drawPerson( 8.5f,-46.8f, 270.0f, 0, 0.30f,0.55f,0.30f, 1, 0.0f);

    /* ---- Family near east balloon stall ---- */
    drawPerson(30.5f,-8.5f, 185.0f, 0, 0.50f,0.28f,0.68f, 0, 0.0f); /* parent */
    drawPerson(29.2f,-9.0f, 195.0f, 1, 0.88f,0.28f,0.28f, 3, walkTime*2.5f); /* kid */

    /* ---- Kid skipping near park entrance ---- */
    float skipX = fmodf(walkTime*4.0f, 14.0f) - 7.0f;
    drawPerson(skipX,-51.0f, 90.0f, 1, 0.18f,0.85f,0.72f, 2, walkTime*5.0f);

    /* ---- People walking from parking lot toward gate ---- */
    float pWalk = fmodf(walkTime*2.5f, 20.0f);
    drawPerson(41.0f, 30.5f - pWalk, 0.0f,   0, 0.55f,0.55f,0.85f, 2, walkTime*3.2f);
    drawPerson(43.0f, 30.5f - pWalk+1.0f, 5.0f, 1, 1.0f,0.65f,0.15f, 2, walkTime*3.2f+0.7f);

    /* ---- Guard / staff standing at parking gate booth ---- */
    drawPerson(37.5f, 31.0f, 90.0f, 0, 0.20f,0.50f,0.20f, 0, 0.0f);
}

/* ============================
   DISPLAY
============================= */
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float radX=angleX*(float)M_PI/180.0f, radY=angleY*(float)M_PI/180.0f;
    float ex=zoomDist*cosf(radX)*sinf(radY);
    float ey=zoomDist*sinf(radX);
    float ez=zoomDist*cosf(radX)*cosf(radY);
    gluLookAt(ex,ey,ez, 0,0,0, 0,1,0);

    drawGround();

    drawRoad(-55,-22,55,-16); drawRoad(-55,0,55,6); drawRoad(-55,22,55,28);
    drawRoad(-22,-55,-16,55); drawRoad(0,-55,6,55); drawRoad(22,-55,28,55);

    float F=62.0f;
    drawFenceSegment(-F,-F,F,-F); drawFenceSegment(F,-F,F,F);
    drawFenceSegment(F,F,-F,F);   drawFenceSegment(-F,F,-F,-F);

    float T=57.0f;
    for(int i=-5;i<=5;i++){
        drawTree(-T,i*11.0f); drawTree(T,i*11.0f);
        drawTree(i*11.0f,-T); drawTree(i*11.0f,T);
    }
    for(int i=-4;i<=4;i++){
        drawTree(-23,i*11.0f); drawTree(-13,i*11.0f);
        drawTree(-3, i*11.0f); drawTree( 9, i*11.0f);
        drawTree(19, i*11.0f); drawTree(31, i*11.0f);
    }

    drawEntranceArch(3.0f,-55.0f);
    drawMerryGoRound(-8.0f,-9.0f,merryAngle);
    drawBalloonStall( 28.0f,-10.0f,  0.0f,windTime);
    drawBalloonStall(-32.0f, 14.0f,180.0f,windTime);
    drawBalloonStall(  3.0f, 35.0f, 90.0f,windTime);
    drawTicketBooth( 12.0f,-48.0f,180.0f);
    drawTicketBooth( -6.0f,-48.0f,180.0f);

    drawBench( 15.0f, -5.0f,  0.0f);
    drawBench(-18.0f, 10.0f, 90.0f);
    drawBench(  3.0f, 16.0f,  0.0f);
    drawBench( 30.0f, 32.0f,180.0f);
    drawBench(-30.0f,-30.0f, 45.0f);
    drawBench(-16.5f, -5.5f, 90.0f);

    for(int lp=-4;lp<=4;lp++){
        drawLampPost(-1.5f,lp*10.0f);
        drawLampPost( 7.5f,lp*10.0f);
    }

    float bCols[5][3]={{1.0f,0.1f,0.1f},{0.1f,0.4f,1.0f},{0.9f,0.85f,0.05f},{0.1f,0.75f,0.2f},{0.85f,0.2f,0.85f}};
    float bPos[5][2]={{20,-20},{-25,25},{10,40},{-35,-10},{35,10}};
    for(int b=0;b<5;b++){
        float sw=sinf(windTime+b*1.3f)*0.3f;
        drawBalloon(bPos[b][0],8.0f,bPos[b][1],bCols[b][0],bCols[b][1],bCols[b][2],sw);
    }

    /* NEW */
    drawParkingLot();
    drawParkingGate();
    drawPeopleScene();

    glutSwapBuffers();
}

/* =============================
   TIMER
============================= */
void timerFunc(int)
{
    merryAngle += 1.2f;
    if(merryAngle>=360.0f) merryAngle-=360.0f;
    windTime += 0.04f;
    walkTime += 0.02f;
    glutPostRedisplay();
    glutTimerFunc(16,timerFunc,0);
}

void reshape(int w,int h)
{
    if(h==0)h=1;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(55,(float)w/h,1,500);
    glMatrixMode(GL_MODELVIEW);
}
void keyboard(unsigned char key,int,int)
{
    if(key=='+'||key=='=')zoomDist-=5.0f;
    if(key=='-')zoomDist+=5.0f;
    zoomDist=fmaxf(20.0f,fminf(250.0f,zoomDist));
    glutPostRedisplay();
}
void specialKeys(int key,int,int)
{
    if(key==GLUT_KEY_LEFT) angleY-=3.0f;
    if(key==GLUT_KEY_RIGHT)angleY+=3.0f;
    if(key==GLUT_KEY_UP)   angleX=fminf(angleX+2.0f,85.0f);
    if(key==GLUT_KEY_DOWN) angleX=fmaxf(angleX-2.0f,5.0f);
    glutPostRedisplay();
}
void mouseButton(int btn,int state,int x,int y)
{
    if(btn==GLUT_LEFT_BUTTON){mouseDown=(state==GLUT_DOWN);lastMouseX=x;lastMouseY=y;}
    if(btn==3){zoomDist-=3.0f;glutPostRedisplay();}
    if(btn==4){zoomDist+=3.0f;glutPostRedisplay();}
}
void mouseMotion(int x,int y)
{
    if(!mouseDown)return;
    int dx=x-lastMouseX,dy=y-lastMouseY;
    angleY+=dx*0.4f;
    angleX=fmaxf(5.0f,fminf(85.0f,angleX-dy*0.35f));
    lastMouseX=x;lastMouseY=y;
    glutPostRedisplay();
}

void init()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.42f,0.72f,0.98f,1.0f);
}

int main(int argc,char**argv)
{
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(1200,750);
    glutCreateWindow("3D Amusement Park — Full Scene");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16,timerFunc,0);
    glutMainLoop();
    return 0;
}