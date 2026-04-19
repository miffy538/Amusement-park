#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <cmath>
#include <cstdio>
#include<algorithm>

float angleY   = 20.0f;
float angleX   = 25.0f;
float zoomDist = 70.0f;

float coasterProgress = 0.0f;
bool coasterRunning = false;
float coasterSpeed = 1.5f;

float carouselAngle = 0.0f;
bool carouselRunning = false;
float carouselSpeed = 1.5f;

int  lastMouseX = -1, lastMouseY = -1;
bool mouseDown  = false;

bool  wheelSpinning = false;
float wheelAngle    = 0.0f;

GLuint groundTexture;

unsigned char* loadBMP(const char* filename, int* width, int* height)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: File not found\n");
        exit(1);
    }

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    *width  = *(int*)&header[18];
    *height = *(int*)&header[22];

    int row_padded = (*width * 3 + 3) & (~3);  // 🔥 important
    unsigned char* data = new unsigned char[row_padded * (*height)];
    fread(data, sizeof(unsigned char), row_padded * (*height), file);

    fclose(file);

    unsigned char* cleanData = new unsigned char[(*width) * (*height) * 3];

    for (int i = 0; i < *height; i++) {
        for (int j = 0; j < *width; j++) {
            for (int k = 0; k < 3; k++) {
                cleanData[(i * (*width) + j) * 3 + k] =
                    data[i * row_padded + j * 3 + (2 - k)]; // BGR → RGB
            }
        }
    }

    delete[] data;
    return cleanData;
}
/* =============================
   PSEUDO-RANDOM FUNCTION
============================= */
float pseudoRandom(float x, float y)
{
    float n = sinf(x * 12.9898f + y * 78.233f) * 43758.5453f;
    return n - floorf(n);
}

/* =============================
   DRAW CLOUD
============================= */
void drawCloud(float x, float y, float z, float scale)
{
    glColor4f(0.95f, 0.95f, 1.0f, 0.85f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glPushMatrix();
    glTranslatef(x, y, z);
    
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glTranslatef(0, 0, 0);
    gluSphere(q, 1.5f * scale, 16, 16);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-1.2f * scale, 0.3f * scale, 0);
    gluSphere(q, 1.2f * scale, 16, 16);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(1.2f * scale, 0.2f * scale, 0);
    gluSphere(q, 1.3f * scale, 16, 16);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0, 1.0f * scale, 0);
    gluSphere(q, 1.0f * scale, 16, 16);
    glPopMatrix();
    
    gluDeleteQuadric(q);
    glPopMatrix();
    
    glDisable(GL_BLEND);
}

/* =============================
   DRAW SKY WITH CLOUDS
============================= */
void drawSkyAndClouds()
{
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-50, 50, -20, 20, -1, 1);//compact
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glBegin(GL_QUADS);
    glColor3f(0.3f, 0.6f, 1.0f);
    glVertex3f(-50, 20, -0.99f);
    glVertex3f(50, 20, -0.99f);
    
    glColor3f(0.5f, 0.8f, 1.0f);
    glVertex3f(50, -20, -0.99f);
    glVertex3f(-50, -20, -0.99f);
    glEnd();
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    glEnable(GL_DEPTH_TEST);
}



/* =============================
   CHECK IF POINT IS ON PATH/ROAD
============================= */
bool isOnRoad(float x, float z)
{
    // Park internal roads
    if ((fabsf(z - (-19)) < 3 && fabsf(x) < 60) ||
        (fabsf(z - 3) < 3 && fabsf(x) < 60) ||
        (fabsf(z - 25) < 3 && fabsf(x) < 60) ||
        (fabsf(x - (-19)) < 3 && fabsf(z) < 60) ||
        (fabsf(x - 3) < 3 && fabsf(z) < 60) ||
        (fabsf(x - 25) < 3 && fabsf(z) < 60)) {
        return true;
    }
    
    // Entrance path - from park (0, -5) to entrance gate (3, 50)
    float pathStartX = 0.0f;
    float pathStartZ = -5.0f;
    float pathEndX = 3.0f;
    float pathEndZ = 50.0f;
    
    float dx = pathEndX - pathStartX;
    float dz = pathEndZ - pathStartZ;
    float pathLen = sqrtf(dx*dx + dz*dz);
    
    dx /= pathLen;
    dz /= pathLen;
    
    float perpX = -dz;
    float perpZ = dx;
    
    float px = x - pathStartX;
    float pz = z - pathStartZ;
    
    float projection = px * dx + pz * dz;
    float distToLine = fabsf(px * perpX + pz * perpZ);
    
    if (projection >= -10.0f && projection <= pathLen + 50.0f && distToLine < 3.0f) {
        return true;
    }
    
    return false;
}

/* =============================
   ULTRA-DENSE FOREST - WITH PATH CLEARING
============================= */
/* =============================
   ULTRA-DENSE FOREST - WITH PATH CLEARING
============================= */
void drawForestBackground()
{
    // Ground plane - BROWN
    glColor3f(0.40f, 0.28f, 0.16f);
    glBegin(GL_QUADS);
    glVertex3f(-500, 0.0f, -500);
    glVertex3f(500, 0.0f, -500);
    glVertex3f(500, 0.0f, 500);
    glVertex3f(-500, 0.0f, 500);
    glEnd();
    
    // Add brown ground texture variation
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(0.3f);
    glColor4f(0.25f, 0.15f, 0.08f, 0.3f);
    
    for (int x = -500; x < 500; x += 20) {
        for (int z = -500; z < 500; z += 20) {
            float randVal = pseudoRandom(x * 0.01f, z * 0.01f);
            if (randVal > 0.5f) {
                float shade = 0.2f + pseudoRandom(x * 0.02f, z * 0.02f) * 0.15f;
                glColor4f(shade * 0.8f, shade * 0.5f, shade * 0.3f, 0.4f);
                
                glBegin(GL_LINE_LOOP);
                glVertex3f(x, 0.001f, z);
                glVertex3f(x + 15, 0.001f, z);
                glVertex3f(x + 15, 0.001f, z + 15);
                glVertex3f(x, 0.001f, z + 15);
                glEnd();
            }
        }
    }
    
    glDisable(GL_BLEND);
    glLineWidth(1.0f);
    
    // ULTRA-DENSE FOREST
    for (int x_pos = -150; x_pos <= 150; x_pos += 5) {
        for (int z_pos = -150; z_pos <= 150; z_pos += 5) {
            float dist = sqrtf(x_pos * x_pos + z_pos * z_pos);
            
            if (isOnRoad(x_pos, z_pos)) {
                continue;
            }
            
            if (dist > 63.0f && dist < 220.0f) {
                float rand1 = pseudoRandom(x_pos * 0.11f, z_pos * 0.13f);
                
                if (rand1 > 0.20f) {
                    float rand2 = pseudoRandom(x_pos * 0.23f, z_pos * 0.29f);
                    float rand3 = pseudoRandom(x_pos * 0.37f, z_pos * 0.41f);
                    
                    float offset_x = (rand2 - 0.5f) * 3.0f;
                    float offset_z = (rand3 - 0.5f) * 3.0f;
                    
                    float tree_x = x_pos + offset_x;
                    float tree_z = z_pos + offset_z;
                    
                    if (isOnRoad(tree_x, tree_z)) {
                        continue;
                    }
                    
                    float treeHeightRand = pseudoRandom(tree_x * 0.07f, tree_z * 0.11f);
                    float treeHeight = 4.0f + treeHeightRand * 6.0f;
                    
                    float trunkRadRand = pseudoRandom(tree_x * 0.13f, tree_z * 0.17f);
                    float trunkRadius = 0.25f + trunkRadRand * 0.35f;
                    
                    float colorRand1 = pseudoRandom(tree_x * 0.19f, tree_z * 0.23f);
                    float colorRand2 = pseudoRandom(tree_x * 0.31f, tree_z * 0.37f);
                    
                    float r = 0.06f + colorRand1 * 0.14f;
                    float treeG = 0.32f + colorRand2 * 0.28f;
                    float treeB = 0.06f + colorRand1 * 0.14f;
                    
                    glPushMatrix();
                    glTranslatef(tree_x, 0.0f, tree_z);
                    
                    glColor3f(0.35f, 0.19f, 0.08f);
                    glRotatef(-90, 1, 0, 0);
                    GLUquadric* q = gluNewQuadric();
                    gluCylinder(q, trunkRadius, trunkRadius * 0.5f, treeHeight * 0.5f, 8, 1);
                    gluDeleteQuadric(q);
                    glPopMatrix();
                    
                    glColor3f(r, treeG, treeB);
                    glPushMatrix();
                    glTranslatef(tree_x, treeHeight * 0.35f, tree_z);
                    GLUquadric* qf = gluNewQuadric();
                    float foliageScale = 3.5f + colorRand1 * 2.5f;
                    gluSphere(qf, trunkRadius * foliageScale, 14, 14);
                    gluDeleteQuadric(qf);
                    glPopMatrix();
                    
                    glColor3f(r * 0.9f, treeG * 0.9f, treeB * 0.9f);
                    glPushMatrix();
                    glTranslatef(tree_x, treeHeight * 0.55f, tree_z);
                    GLUquadric* qf2 = gluNewQuadric();
                    gluSphere(qf2, trunkRadius * (foliageScale * 0.7f), 12, 12);
                    gluDeleteQuadric(qf2);
                    glPopMatrix();
                }
            }
        }
    }
    
    // EXTRA DENSE OUTER RING
    for (int x_pos = -200; x_pos <= 200; x_pos += 3) {
        for (int z_pos = -200; z_pos <= 200; z_pos += 3) {
            float dist = sqrtf(x_pos * x_pos + z_pos * z_pos);
            
            if (isOnRoad(x_pos, z_pos)) {
                continue;
            }
            
            if (dist > 63.0f && dist < 280.0f) {
                float rand1 = pseudoRandom(x_pos * 0.17f, z_pos * 0.19f);
                
                if (rand1 > 0.10f) {
                    float rand2 = pseudoRandom(x_pos * 0.27f, z_pos * 0.31f);
                    float rand3 = pseudoRandom(x_pos * 0.41f, z_pos * 0.43f);
                    
                    float offset_x = (rand2 - 0.5f) * 2.0f;
                    float offset_z = (rand3 - 0.5f) * 2.0f;
                    
                    float tree_x = x_pos + offset_x;
                    float tree_z = z_pos + offset_z;
                    
                    if (isOnRoad(tree_x, tree_z)) {
                        continue;
                    }
                    
                    float treeHeightRand = pseudoRandom(tree_x * 0.09f, tree_z * 0.13f);
                    float treeHeight = 3.5f + treeHeightRand * 5.5f;
                    
                    float trunkRadRand = pseudoRandom(tree_x * 0.15f, tree_z * 0.21f);
                    float trunkRadius = 0.20f + trunkRadRand * 0.30f;
                    
                    float colorRand1 = pseudoRandom(tree_x * 0.23f, tree_z * 0.29f);
                    float colorRand2 = pseudoRandom(tree_x * 0.33f, tree_z * 0.39f);
                    
                    float r = 0.05f + colorRand1 * 0.15f;
                    float treeG = 0.30f + colorRand2 * 0.30f;
                    float treeB = 0.05f + colorRand1 * 0.15f;
                    
                    glPushMatrix();
                    glTranslatef(tree_x, 0.0f, tree_z);
                    
                    glColor3f(0.33f, 0.18f, 0.07f);
                    glRotatef(-90, 1, 0, 0);
                    GLUquadric* q = gluNewQuadric();
                    gluCylinder(q, trunkRadius, trunkRadius * 0.5f, treeHeight * 0.5f, 7, 1);
                    gluDeleteQuadric(q);
                    glPopMatrix();
                    
                    glColor3f(r, treeG, treeB);
                    glPushMatrix();
                    glTranslatef(tree_x, treeHeight * 0.35f, tree_z);
                    GLUquadric* qf = gluNewQuadric();
                    float foliageScale = 3.0f + colorRand1 * 2.0f;
                    gluSphere(qf, trunkRadius * foliageScale, 12, 12);
                    gluDeleteQuadric(qf);
                    glPopMatrix();
                }
            }
        }
    }
    
    // Draw clouds
    glPushMatrix();
    drawCloud(-80, 40, -50, 3.0f);
    drawCloud(-30, 50, -80, 2.5f);
    drawCloud(20, 45, -100, 3.5f);
    drawCloud(70, 55, -40, 2.8f);
    drawCloud(90, 42, -70, 3.0f);
    drawCloud(-60, 65, -120, 2.2f);
    glPopMatrix();
}

/* =============================
   GROUND
============================= */
void drawGround()
{
    float F = 62.0f;

    // =========================
    // OUTSIDE (plain brown)
    // =========================
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.45f, 0.30f, 0.15f);

    float y = -0.05f; // 🔥 push ground slightly down (VERY IMPORTANT)

    glBegin(GL_QUADS);

    // Left
    glVertex3f(-500, y, -500);
    glVertex3f(-F,   y, -500);
    glVertex3f(-F,   y,  500);
    glVertex3f(-500, y,  500);

    // Right
    glVertex3f(F,    y, -500);
    glVertex3f(500,  y, -500);
    glVertex3f(500,  y,  500);
    glVertex3f(F,    y,  500);

    // Bottom
    glVertex3f(-F, y, -500);
    glVertex3f(F,  y, -500);
    glVertex3f(F,  y, -F);
    glVertex3f(-F, y, -F);

    // Top
    glVertex3f(-F, y, F);
    glVertex3f(F,  y, F);
    glVertex3f(F,  y, 500);
    glVertex3f(-F, y, 500);

    glEnd();


    // =========================
// INSIDE (TEXTURED - FIXED)
// =========================
glEnable(GL_TEXTURE_2D);
glBindTexture(GL_TEXTURE_2D, groundTexture);

glColor3f(1.0f, 1.0f, 1.0f);

float tileSize = 4.0f;


float repeatFactor = 0.08f; // 🔥 controls grass scale

for (float x = -F; x < F; x += tileSize) {
    for (float z = -F; z < F; z += tileSize) {

        float tx = (x + F) * repeatFactor;
        float tz = (z + F) * repeatFactor;

        glBegin(GL_QUADS);

        glTexCoord2f(tx, tz);
        glVertex3f(x, y, z);

        glTexCoord2f(tx + tileSize * repeatFactor, tz);
        glVertex3f(x + tileSize, y, z);

        glTexCoord2f(tx + tileSize * repeatFactor, tz + tileSize * repeatFactor);
        glVertex3f(x + tileSize, y, z + tileSize);

        glTexCoord2f(tx, tz + tileSize * repeatFactor);
        glVertex3f(x, y, z + tileSize);

        glEnd();
    }
}

glDisable(GL_TEXTURE_2D);
}

void drawHills()
{
    float startZ = -220.0f;   // distance from park
    float height = 35.0f;     // hill height
    float width  = 500.0f;    // full width

    glColor3f(0.18f, 0.38f, 0.18f); // dark green

    for (int layer = 0; layer < 3; layer++)
    {
        float z = startZ - layer * 40.0f;
        float h = height - layer * 8.0f;

        glBegin(GL_TRIANGLE_STRIP);

        for (float x = -width; x <= width; x += 5.0f)
        {
            float y = h + 8.0f * sinf(x * 0.02f + layer);

            glVertex3f(x, 0.0f, z);   // base
            glVertex3f(x, y, z);      // top
        }

        glEnd();
    }
}
/* =============================
   ENTRANCE PATH ROAD - With proper road styling
============================= */
void drawEntrancePath()
{
    // Path from park center (0, -5) to entrance gate (3, 50)
    float pathX1 = 0.0f, pathZ1 = 0.0f;
    float pathX2 = 6.0f, pathZ2 = 100.0f;
    
    float pathWidth = 3.0f;
    
    // Direction vector
    float dx = pathX2 - pathX1;
    float dz = pathZ2 - pathZ1;
    float pathLen = sqrtf(dx*dx + dz*dz);
    dx /= pathLen;
    dz /= pathLen;
    
    // Perpendicular vector
    float perpX = -dz;
    float perpZ = dx;
    
    // Draw the main road - dark asphalt
    glColor3f(0.22f, 0.22f, 0.22f);
    glBegin(GL_QUADS);
    glVertex3f(pathX1 + perpX * pathWidth, 0.03f, pathZ1 + perpZ * pathWidth);
    glVertex3f(pathX1 - perpX * pathWidth, 0.03f, pathZ1 - perpZ * pathWidth);
    glVertex3f(pathX2 - perpX * pathWidth, 0.03f, pathZ2 - perpZ * pathWidth);
    glVertex3f(pathX2 + perpX * pathWidth, 0.03f, pathZ2 + perpZ * pathWidth);
    glEnd();
    
    // Yellow dashed line down the center
    glColor3f(0.9f, 0.8f, 0.0f);
    int numSegments = (int)(pathLen / 1.0f);
    
    for (int seg = 0; seg < numSegments; seg += 10) {
        float t0 = (float)seg / numSegments;
        float t1 = (float)(seg + 8) / numSegments;
        
        if (t1 > 1.0f) t1 = 1.0f;
        
        float x0 = pathX1 + dx * pathLen * t0;
        float z0 = pathZ1 + dz * pathLen * t0;
        float x1 = pathX1 + dx * pathLen * t1;
        float z1 = pathZ1 + dz * pathLen * t1;
        
        glBegin(GL_QUADS);
        glVertex3f(x0 - perpX * 0.15f, 0.04f, z0 - perpZ * 0.15f);
        glVertex3f(x0 + perpX * 0.15f, 0.04f, z0 + perpZ * 0.15f);
        glVertex3f(x1 + perpX * 0.15f, 0.04f, z1 + perpZ * 0.15f);
        glVertex3f(x1 - perpX * 0.15f, 0.04f, z1 - perpZ * 0.15f);
        glEnd();
    }
    
    // Add road edge lines for depth (white edge markings)
    glColor3f(0.95f, 0.95f, 0.95f);
    glLineWidth(1.5f);
    
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= numSegments; i += 5) {
        float t = (float)i / numSegments;
        if (t > 1.0f) t = 1.0f;
        float x = pathX1 + dx * pathLen * t;
        float z = pathZ1 + dz * pathLen * t;
        glVertex3f(x + perpX * pathWidth, 0.035f, z + perpZ * pathWidth);
    }
    glEnd();
    
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= numSegments; i += 5) {
        float t = (float)i / numSegments;
        if (t > 1.0f) t = 1.0f;
        float x = pathX1 + dx * pathLen * t;
        float z = pathZ1 + dz * pathLen * t;
        glVertex3f(x - perpX * pathWidth, 0.035f, z - perpZ * pathWidth);
    }
    glEnd();
    
    glLineWidth(1.0f);
}

/* =============================
   ROAD  (dark asphalt + dashed centre line)
============================= */
void drawRoad(float x1, float z1, float x2, float z2)
{
    glColor3f(0.20f, 0.20f, 0.20f);
    glBegin(GL_QUADS);
    glVertex3f(x1, 0.03f, z1);
    glVertex3f(x2, 0.03f, z1);
    glVertex3f(x2, 0.03f, z2);
    glVertex3f(x1, 0.03f, z2);
    glEnd();

    bool  horiz = fabsf(x2 - x1) > fabsf(x2 - z1);
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
   TREE
============================= */
void drawTree(float x, float z)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    glColor3f(0.42f, 0.27f, 0.12f);
    glPushMatrix();
    glTranslatef(0, 1.5f, 0);
    glRotatef(-90, 1, 0, 0);
    GLUquadric* q = gluNewQuadric();
    gluCylinder(q, 0.25, 0.18, 3.0, 8, 1);
    gluDeleteQuadric(q);
    glPopMatrix();

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
   DRAW BOX
============================= */
void drawBox(float x, float y, float z, float sx, float sy, float sz, float r, float g, float b)
{
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glutSolidCube(1.0f);
    glPopMatrix();
}

/* =============================
   FENCE
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

        glColor3f(0.65f, 0.45f, 0.20f);
        glPushMatrix();
        glTranslatef(0, 1.2f, px);
        glRotatef(-90, 1, 0, 0);
        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, 0.08f, 0.08f, 5.0f, 6, 1);
        gluDeleteQuadric(q);
        glPopMatrix();

        glColor3f(0.55f, 0.35f, 0.12f);
        glPushMatrix();
        glTranslatef(0, 6.3f, px);
        glutSolidSphere(0.12f, 6, 6);
        glPopMatrix();
    }

    glColor3f(0.60f, 0.42f, 0.18f);
    for (int r = 0; r < 3; r++) {
        float ry = (r == 0) ? 1.5f : (r == 1) ? 3.5f : 5.5f;
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
   ENTRANCE GATE
============================= */
void drawEntranceGate(float cx, float cz)
{
    glPushMatrix();
    glTranslatef(cx, 0, cz);

    GLUquadric* q = gluNewQuadric();

    for (int p = -1; p <= 1; p += 2) {
        glColor3f(0.85f, 0.78f, 0.60f);
        glPushMatrix();
        glTranslatef(p * 6.0f, 0, 0);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 1.2f, 1.2f, 12.0f, 14, 1);
        glPopMatrix();
        glColor3f(0.70f, 0.60f, 0.40f);
        glPushMatrix();
        glTranslatef(p * 6.0f, 12.3f, 0);
        glutSolidSphere(1.5f, 12, 12);
        glPopMatrix();
    }

    drawBox(0, 11.0f, 0, 13.0f, 1.5f, 1.5f, 0.80f, 0.70f, 0.45f);

    glColor3f(0.75f, 0.65f, 0.40f);
    int archSegs = 24;
    float ar = 4.5f;
    glBegin(GL_TRIANGLE_STRIP);
    for (int s = 0; s <= archSegs; s++) {
        float a = s * (float)M_PI / archSegs;
        float ay = 11.0f + ar * sinf(a);
        float ax = ar * cosf(a);
        glVertex3f(ax, ay + 0.75f, 0.8f);
        glVertex3f(ax, ay - 0.75f, 0.8f);
    }
    glEnd();

    drawBox(0, 13.2f, 0, 8.0f, 1.6f, 0.3f, 0.95f, 0.85f, 0.10f);

    for (int f = -1; f <= 1; f += 2) {
        glColor3f(0.5f, 0.5f, 0.5f);
        glPushMatrix();
        glTranslatef(f * 6.0f, 11.5f, 0);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.12f, 0.12f, 4.5f, 8, 1);
        glPopMatrix();
        glColor3f((f == -1) ? 0.9f : 0.1f,
                  (f == -1) ? 0.1f : 0.8f,
                  0.15f);
        glPushMatrix();
        glTranslatef(f * 6.0f + (f > 0 ? 0.2f : -2.0f), 14.0f, 0);
        glScalef(2.0f, 1.2f, 0.15f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    gluDeleteQuadric(q);
    glPopMatrix();
}

/* =============================
   CAROUSEL
============================= */
void drawCarousel(float cx, float cz)
{
    glPushMatrix();
    glTranslatef(cx, 0, cz);

    GLUquadric* q = gluNewQuadric();

    glColor3f(0.85f, 0.75f, 0.90f);
    glPushMatrix();
    glTranslatef(0, 0.5f, 0);
    gluCylinder(q, 5.0f, 5.0f, 0.5f, 24, 1);
    glPopMatrix();

    glColor3f(0.7f, 0.1f, 0.1f);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.4f, 0.4f, 7.0f, 10, 1);
    glPopMatrix();

    glColor3f(0.90f, 0.20f, 0.40f);
    glPushMatrix();
    glTranslatef(0, 7.0f, 0);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 5.5f, 0.0f, 2.5f, 16, 1);
    glPopMatrix();

    glColor3f(1.0f, 0.85f, 0.0f);
    glPushMatrix();
    glTranslatef(0, 9.6f, 0);
    glutSolidSphere(0.6f, 12, 12);
    glPopMatrix();

    glPushMatrix();
    glRotatef(carouselAngle, 0, 1, 0);

    float hColors[8][3] = {
        {0.95f, 0.95f, 0.95f},
        {0.75f, 0.50f, 0.25f},
        {0.95f, 0.75f, 0.80f},
        {0.65f, 0.65f, 0.70f},
        {0.95f, 0.3f, 0.3f},
        {0.3f, 0.6f, 0.95f},
        {0.3f, 0.95f, 0.3f},
        {0.95f, 0.95f, 0.1f}
    };

    for (int h = 0; h < 8; h++) {
        float ang = h * 45.0f * (float)M_PI / 180.0f;
        float hx  = 3.5f * sinf(ang);
        float hz  = 3.5f * cosf(ang);
        float bobbing = sinf(carouselAngle * (float)M_PI / 180.0f + h) * 0.3f;

        glColor3f(0.8f, 0.8f, 0.1f);
        glPushMatrix();
        glTranslatef(hx, 0, hz);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.1f, 0.1f, 6.5f, 8, 1);
        glPopMatrix();

        glColor3f(hColors[h][0], hColors[h][1], hColors[h][2]);
        glPushMatrix();
        glTranslatef(hx, 2.0f + bobbing, hz);
        glScalef(1.3f, 0.9f, 0.6f);
        glutSolidCube(1.0f);
        glPopMatrix();

        glColor3f(hColors[h][0]*0.95f, hColors[h][1]*0.95f, hColors[h][2]*0.95f);
        glPushMatrix();
        glTranslatef(hx + 0.7f, 2.5f + bobbing, hz);
        glScalef(0.7f, 0.6f, 0.5f);
        glutSolidCube(1.0f);
        glPopMatrix();

        glColor3f(0.7f, 0.2f, 0.6f);
        glBegin(GL_LINES);
        glVertex3f(0, 6.8f, 0);
        glVertex3f(hx, 6.8f, hz);
        glEnd();

        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_LINES);
        glVertex3f(hx, 2.0f + bobbing, hz);
        glVertex3f(0, 0.0f, 0);
        glEnd();
    }

    glPopMatrix();
    gluDeleteQuadric(q);
    glPopMatrix();
}

/* =============================
   Roller Coaster Helper
============================= */
void getTrackPoint(float t, float& out_x, float& out_y, float& out_z)
{
    t = fmodf(t, 1.0f);
    if (t < 0.0f) t += 1.0f;
    
    float angle = t * 2.0f * (float)M_PI;
    
    float denominator = 1.0f + sinf(angle) * sinf(angle);
    float loop_x = 18.0f * cosf(angle) / denominator;
    float loop_z = 18.0f * sinf(angle) * cosf(angle) / denominator;
    
    float height_base = 8.0f + 8.0f * (sinf(angle) + 1.0f) / 2.0f;
    float loop_effect = 6.0f * sinf(angle * 2.0f);
    float loop_y = height_base + loop_effect;
    
    out_x = loop_x;
    out_y = fmaxf(3.0f, loop_y);
    out_z = loop_z;
}

/* =============================
   Roller Coaster
============================= */
void drawRollerCoaster(float cx, float cz)
{
    glPushMatrix();
    glTranslatef(cx, 0.0f, cz);

    const int SEGMENTS = 360;
    const float RAIL_GAP = 2.2f;
    
    GLUquadric* q = gluNewQuadric();

    glColor3f(0.35f, 0.35f, 0.38f);
    for (float track_t = 0.0f; track_t < 1.0f; track_t += 0.08f) {
        float tx, ty, tz;
        getTrackPoint(track_t, tx, ty, tz);
        
        glPushMatrix();
        glTranslatef(tx, 0.0f, tz);
        glRotatef(-90.0f, 1, 0, 0);
        gluCylinder(q, 0.4, 0.5, ty, 12, 1);
        glPopMatrix();
        
        float tx_next, ty_next, tz_next;
        getTrackPoint(fmodf(track_t + 0.03f, 1.0f), tx_next, ty_next, tz_next);
        
        float dx = tx_next - tx, dz = tz_next - tz;
        float dist = sqrtf(dx*dx + dz*dz);
        
        if (dist > 0.1f) {
            glColor3f(0.40f, 0.40f, 0.45f);
            glPushMatrix();
            glTranslatef(tx + dx*0.3f, ty*0.4f, tz + dz*0.3f);
            float angle_xz = atan2f(dz, dx) * 180.0f / (float)M_PI;
            glRotatef(angle_xz, 0, 1, 0);
            glRotatef(-90.0f, 1, 0, 0);
            gluCylinder(q, 0.2, 0.25, dist * 0.4f, 8, 1);
            glPopMatrix();
        }
    }

    for (int seg = 0; seg < SEGMENTS; seg++) {
        float t0 = (float)seg / SEGMENTS;
        float t1 = (float)(seg + 1) / SEGMENTS;
        
        float x0, y0, z0, x1, y1, z1;
        getTrackPoint(t0, x0, y0, z0);
        getTrackPoint(t1, x1, y1, z1);
        
        float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
        float track_len = sqrtf(dx*dx + dy*dy + dz*dz);
        
        if (track_len > 0.001f) {
            dx /= track_len; dy /= track_len; dz /= track_len;
            
            float perp_x = -dz, perp_z = dx;
            float perp_len = sqrtf(perp_x*perp_x + perp_z*perp_z);
            if (perp_len > 0.01f) {
                perp_x /= perp_len;
                perp_z /= perp_len;
            }
            
            float left_x0 = x0 - perp_x * RAIL_GAP/2.0f;
            float left_z0 = z0 - perp_z * RAIL_GAP/2.0f;
            
            glColor3f(0.85f, 0.15f, 0.15f);
            glPushMatrix();
            glTranslatef(left_x0, y0, left_z0);
            glRotatef(atan2f(sqrtf(dx*dx + dz*dz), dy) * 180.0f / (float)M_PI, 
                     -dz * perp_len, 0, dx * perp_len);
            glRotatef(atan2f(dz, dx) * 180.0f / (float)M_PI, 0, 1, 0);
            glRotatef(180.0f, 1, 0, 0);
            gluCylinder(q, 0.28, 0.28, track_len, 8, 1);
            glPopMatrix();
            
            float right_x0 = x0 + perp_x * RAIL_GAP/2.0f;
            float right_z0 = z0 + perp_z * RAIL_GAP/2.0f;
            
            glPushMatrix();
            glTranslatef(right_x0, y0, right_z0);
            glRotatef(atan2f(sqrtf(dx*dx + dz*dz), dy) * 180.0f / (float)M_PI, 
                     -dz * perp_len, 0, dx * perp_len);
            glRotatef(atan2f(dz, dx) * 180.0f / (float)M_PI, 0, 1, 0);
            glRotatef(180.0f, 1, 0, 0);
            gluCylinder(q, 0.28, 0.28, track_len, 8, 1);
            glPopMatrix();
            
            if (seg % 8 == 0) {
                float left_x1 = x1 - perp_x * RAIL_GAP/2.0f;
                float left_z1 = z1 - perp_z * RAIL_GAP/2.0f;
                float right_x1 = x1 + perp_x * RAIL_GAP/2.0f;
                float right_z1 = z1 + perp_z * RAIL_GAP/2.0f;
                
                glColor3f(0.75f, 0.12f, 0.12f);
                glBegin(GL_QUADS);
                glVertex3f(left_x0, y0 + 0.15f, left_z0);
                glVertex3f(right_x0, y0 + 0.15f, right_z0);
                glVertex3f(right_x1, y1 + 0.15f, right_z1);
                glVertex3f(left_x1, y1 + 0.15f, left_z1);
                glEnd();
            }
        }
    }

    for (int seg = 0; seg < SEGMENTS; seg += 6) {
        float t = (float)seg / SEGMENTS;
        float x, y, z;
        getTrackPoint(t, x, y, z);
        
        glColor3f(0.95f, 0.85f, 0.10f);
        
        glPushMatrix();
        glTranslatef(x - RAIL_GAP/2.0f - 0.3f, y + 1.2f, z);
        glRotatef(-90.0f, 1, 0, 0);
        gluCylinder(q, 0.1, 0.1, 0.6f, 6, 1);
        glPopMatrix();
        
        glPushMatrix();
        glTranslatef(x + RAIL_GAP/2.0f + 0.3f, y + 1.2f, z);
        glRotatef(-90.0f, 1, 0, 0);
        gluCylinder(q, 0.1, 0.1, 0.6f, 6, 1);
        glPopMatrix();
    }

    const int NUM_CARS = 4;
    const float CAR_LENGTH = 1.8f;
    const float CAR_SPACING = 0.8f;
    
    for (int car_idx = 0; car_idx < NUM_CARS; car_idx++) {
        float car_offset = car_idx * (CAR_LENGTH + CAR_SPACING);
        float car_t = fmodf((coasterProgress / 360.0f + car_offset / 360.0f), 1.0f);
        
        float car_x, car_y, car_z;
        getTrackPoint(car_t, car_x, car_y, car_z);
        
        float next_t = fmodf(car_t + 0.003f, 1.0f);
        float next_x, next_y, next_z;
        getTrackPoint(next_t, next_x, next_y, next_z);
        
        float dir_x = next_x - car_x, dir_y = next_y - car_y, dir_z = next_z - car_z;
        float dir_len = sqrtf(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z);
        
        glPushMatrix();
        glTranslatef(car_x, car_y, car_z);
        
        if (dir_len > 0.001f) {
            float yaw = atan2f(dir_z, dir_x) * 180.0f / (float)M_PI;
            float pitch = atan2f(dir_y, sqrtf(dir_x*dir_x + dir_z*dir_z)) * 180.0f / (float)M_PI;
            glRotatef(yaw, 0, 1, 0);
            glRotatef(pitch, 0, 0, 1);
        }
        
        float car_colors[4][3] = {
            {0.2f, 0.6f, 0.95f},
            {0.95f, 0.3f, 0.2f},
            {0.2f, 0.85f, 0.3f},
            {0.95f, 0.8f, 0.1f}
        };
        
        glColor3f(car_colors[car_idx][0], car_colors[car_idx][1], car_colors[car_idx][2]);
        glPushMatrix();
        glScalef(CAR_LENGTH, 0.9f, 1.0f);
        glutSolidCube(1.0f);
        glPopMatrix();
        
        glColor3f(0.7f, 0.9f, 1.0f);
        glPushMatrix();
        glTranslatef(-0.7f, 0.2f, 0.55f);
        glScalef(0.6f, 0.5f, 0.08f);
        glutSolidCube(1.0f);
        glPopMatrix();
        
        glColor3f(0.1f, 0.1f, 0.1f);
        glPushMatrix();
        glTranslatef(-0.9f, 0.0f, 0.0f);
        glScalef(0.2f, 0.7f, 1.0f);
        glutSolidCube(1.0f);
        glPopMatrix();
        
        glColor3f(car_colors[car_idx][0]*0.6f, car_colors[car_idx][1]*0.6f, car_colors[car_idx][2]*0.6f);
        glPushMatrix();
        glTranslatef(0.0f, -0.3f, 0.0f);
        glScalef(1.4f, 0.3f, 0.8f);
        glutSolidCube(1.0f);
        glPopMatrix();
        
        glColor3f(0.2f, 0.2f, 0.2f);
        glPushMatrix();
        glTranslatef(0.0f, 0.3f, 0.5f);
        glRotatef(90.0f, 0, 0, 1);
        gluCylinder(q, 0.1, 0.1, 1.6f, 8, 1);
        glPopMatrix();
        
        glColor3f(0.15f, 0.15f, 0.15f);
        glPushMatrix();
        glTranslatef(-0.4f, -0.7f, -0.6f);
        glutSolidSphere(0.35, 10, 10);
        glPopMatrix();
        
        glPushMatrix();
        glTranslatef(-0.4f, -0.7f, 0.6f);
        glutSolidSphere(0.35, 10, 10);
        glPopMatrix();
        
        glPopMatrix();
    }

    gluDeleteQuadric(q);
    glPopMatrix();
}

/* =============================
   GIANT WHEEL
============================= */
void drawGiantWheel(float cx, float cz)
{
    glPushMatrix();
    glTranslatef(cx, 0.0f, cz);

    const float HUB_H      = 20.0f;
    const float WHEEL_R    = 12.0f;
    const int   SPOKES     = 16;
    const int   GONDOLAS   = 12;
    const float LEG_SPREAD = 7.0f;

    GLUquadric* q = gluNewQuadric();

    glColor3f(0.55f, 0.55f, 0.60f);
    glPushMatrix();
    glTranslatef(-LEG_SPREAD, 0.0f, 2.0f);
    glRotatef(12.0f, 0, 0, 1);
    glRotatef(-90.0f, 1, 0, 0);
    gluCylinder(q, 0.4, 0.3, HUB_H + 2.0f, 10, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(LEG_SPREAD, 0.0f, 2.0f);
    glRotatef(-12.0f, 0, 0, 1);
    glRotatef(-90.0f, 1, 0, 0);
    gluCylinder(q, 0.4, 0.3, HUB_H + 2.0f, 10, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-LEG_SPREAD, 0.0f, -2.0f);
    glRotatef(12.0f, 0, 0, 1);
    glRotatef(-90.0f, 1, 0, 0);
    gluCylinder(q, 0.4, 0.3, HUB_H + 2.0f, 10, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(LEG_SPREAD, 0.0f, -2.0f);
    glRotatef(-12.0f, 0, 0, 1);
    glRotatef(-90.0f, 1, 0, 0);
    gluCylinder(q, 0.4, 0.3, HUB_H + 2.0f, 10, 1);
    glPopMatrix();

    glColor3f(0.45f, 0.45f, 0.50f);
    glPushMatrix();
    glTranslatef(-LEG_SPREAD, HUB_H * 0.45f, 0.0f);
    glRotatef(90.0f, 0, 1, 0);
    gluCylinder(q, 0.2, 0.2, LEG_SPREAD * 2.0f, 8, 1);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-LEG_SPREAD, HUB_H * 0.7f, 0.0f);
    glRotatef(90.0f, 0, 1, 0);
    gluCylinder(q, 0.15, 0.15, LEG_SPREAD * 2.0f, 8, 1);
    glPopMatrix();

    glColor3f(0.3f, 0.3f, 0.35f);
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    glScalef(2.0f, 1.5f, 2.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glColor3f(0.7f, 0.7f, 0.75f);
    glPushMatrix();
    glTranslatef(0.0f, HUB_H, 0.0f);
    glRotatef(90.0f, 1, 0, 0);
    gluCylinder(q, 0.35, 0.35, 4.0f, 10, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, HUB_H, 0.0f);
    glRotatef(wheelAngle, 0, 0, 1);

    const int RIM_SEG = 40;
    float rimZ[2] = { -1.5f, 1.5f };
    for (int ring = 0; ring < 2; ring++) {
        glColor3f(0.85f, 0.20f, 0.20f);
        glBegin(GL_LINE_LOOP);
        for (int s = 0; s < RIM_SEG; s++) {
            float a = s * 2.0f * (float)M_PI / RIM_SEG;
            glVertex3f(WHEEL_R * cosf(a), WHEEL_R * sinf(a), rimZ[ring]);
        }
        glEnd();

        glColor3f(0.80f, 0.18f, 0.18f);
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, rimZ[ring]);
        glRotatef(-90.0f, 1, 0, 0);
        GLUquadric* qr = gluNewQuadric();
        gluDisk(qr, WHEEL_R - 0.3f, WHEEL_R, 40, 1);
        gluDeleteQuadric(qr);
        glPopMatrix();
    }

    glColor3f(0.75f, 0.15f, 0.15f);
    for (int s = 0; s < RIM_SEG; s += 3) {
        float a = s * 2.0f * (float)M_PI / RIM_SEG;
        float x = WHEEL_R * cosf(a), y = WHEEL_R * sinf(a);
        glBegin(GL_LINES);
        glVertex3f(x, y, -1.5f);
        glVertex3f(x, y,  1.5f);
        glEnd();
    }

    for (int sp = 0; sp < SPOKES; sp++) {
        float a  = sp * 2.0f * (float)M_PI / SPOKES;
        float sx = WHEEL_R * cosf(a);
        float sy = WHEEL_R * sinf(a);

        if (sp % 2 == 0) glColor3f(0.90f, 0.85f, 0.15f);
        else             glColor3f(0.95f, 0.95f, 0.95f);

        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, -1.3f);
        glVertex3f(sx, sy, -1.3f);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 1.3f);
        glVertex3f(sx, sy, 1.3f);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(sx * 0.5f, sy * 0.5f, -1.3f);
        glVertex3f(sx * 0.5f, sy * 0.5f,  1.3f);
        glEnd();
        glLineWidth(1.0f);
    }

    glColor3f(0.90f, 0.85f, 0.15f);
    glBegin(GL_LINE_LOOP);
    for (int s = 0; s < RIM_SEG; s++) {
        float a = s * 2.0f * (float)M_PI / RIM_SEG;
        glVertex3f((WHEEL_R * 0.5f) * cosf(a), (WHEEL_R * 0.5f) * sinf(a), 0.0f);
    }
    glEnd();

    glColor3f(0.95f, 0.85f, 0.10f);
    glPushMatrix();
    glRotatef(-90.0f, 1, 0, 0);
    gluDisk(q, 0.0f, 1.5f, 14, 2);
    glPopMatrix();
    glPushMatrix();
    glRotatef(90.0f, 1, 0, 0);
    gluDisk(q, 0.0f, 1.5f, 14, 2);
    glPopMatrix();
    glutSolidSphere(1.0f, 14, 14);

    for (int g = 0; g < GONDOLAS; g++) {
        float ga = g * 2.0f * (float)M_PI / GONDOLAS;
        float gx = WHEEL_R * cosf(ga);
        float gy = WHEEL_R * sinf(ga);

        glPushMatrix();
        glTranslatef(gx, gy, 0.0f);
        glRotatef(-wheelAngle, 0, 0, 1);

        glColor3f(0.4f, 0.4f, 0.45f);
        GLUquadric* qs = gluNewQuadric();
        glPushMatrix();
        glRotatef(-90.0f, 1, 0, 0);
        gluCylinder(qs, 0.07, 0.07, 1.2f, 6, 1);
        glPopMatrix();
        glTranslatef(0.0f, -1.2f, 0.0f);

        float gc_colors[6][3] = {
            {0.9f,0.2f,0.2f}, {0.2f,0.5f,0.9f}, {0.2f,0.8f,0.2f},
            {0.9f,0.7f,0.1f}, {0.8f,0.2f,0.8f}, {0.1f,0.8f,0.8f}
        };
        int ci = g % 6;
        glColor3f(gc_colors[ci][0], gc_colors[ci][1], gc_colors[ci][2]);

        glPushMatrix();
        glScalef(1.6f, 1.2f, 1.0f);
        glutSolidCube(1.0f);
        glPopMatrix();

        glColor3f(gc_colors[ci][0]*0.7f, gc_colors[ci][1]*0.7f, gc_colors[ci][2]*0.7f);
        glPushMatrix();
        glTranslatef(0.0f, 0.7f, 0.0f);
        glScalef(1.8f, 0.2f, 1.2f);
        glutSolidCube(1.0f);
        glPopMatrix();

        glColor3f(0.75f, 0.92f, 1.0f);
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.55f);
        glScalef(1.2f, 0.5f, 0.05f);
        glutSolidCube(1.0f);
        glPopMatrix();

        gluDeleteQuadric(qs);
        glPopMatrix();
    }

    glPopMatrix();
    gluDeleteQuadric(q);
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

    drawSkyAndClouds();
    drawGround();
    drawHills();
    //drawForestBackground();

    drawRoad(-60, -22,  60, -16);
    drawRoad(-60,   0,  60,   6);
    drawRoad(-60,  22,  60,  28);

    drawRoad(-60,  -72,  6,  -78);//outside road

    drawRoad(-22, -60, -16,  60);
    drawRoad(  0, -75,   6,  60);//entrance road
    drawRoad( 22, -60,  28,  60);

    // Draw entrance path ROAD
    //drawEntrancePath();

    drawGiantWheel(40.0f, -35.0f);
    drawRollerCoaster(-40.0f, 40.0f);
    drawCarousel(0.0f, -35.0f);
    drawEntranceGate(3.0f, -60.0f);

    float F = 62.0f;
    drawFenceSegment(-F, -F,  -4, -F);
   drawFenceSegment(11, -F, F, -F); //entrance right side fence
    drawFenceSegment( F, -F,  F,  F);
    drawFenceSegment( F,  F, -F,  F);
    drawFenceSegment(-F,  F, -F, -F);

    float T = 57.0f;
    for (int i = -5; i <= 5; i++) {
        drawTree(-T,  i * 11.0f);
        drawTree( T,  i * 11.0f);
        drawTree( i * 11.0f, -T);
        drawTree( i * 11.0f,  T);
    }

  

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55, (float)w / h, 1, 500);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int, int)
{
    if (key == '+' || key == '=') zoomDist -= 5.0f;
    if (key == '-')               zoomDist += 5.0f;
    zoomDist = fmaxf(20.0f, fminf(250.0f, zoomDist));
  
    if (key == 'w' || key == 'W') wheelSpinning = !wheelSpinning;
    if (key == 'r' || key == 'R') coasterRunning = !coasterRunning;
    if (key == 'c' || key == 'C') carouselRunning = !carouselRunning;
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

void init()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.42f, 0.72f, 0.98f, 1.0f);

    // 🔥 IMPORTANT: Better texture quality
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // Enable texturing globally (safe)
    glEnable(GL_TEXTURE_2D);

    int w, h;
unsigned char* data = loadBMP("grass.bmp", &w, &h);

glGenTextures(1, &groundTexture);
glBindTexture(GL_TEXTURE_2D, groundTexture);

// 🔥 Use THIS instead of gluBuild2DMipmaps
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
             GL_RGB, GL_UNSIGNED_BYTE, data);

glGenerateMipmap(GL_TEXTURE_2D); // 🔥 modern mipmaps

// Filtering
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// Wrapping
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

delete[] data;
}
void timer(int)
{
    if (wheelSpinning) {
        wheelAngle += 0.5f;
        if (wheelAngle >= 360.0f) wheelAngle -= 360.0f;
        glutPostRedisplay();
    }
    if (coasterRunning) {
        coasterProgress += coasterSpeed;
        if (coasterProgress >= 360.0f) coasterProgress -= 360.0f;
        glutPostRedisplay();
    }
    if (carouselRunning) {
        carouselAngle += carouselSpeed;
        if (carouselAngle >= 360.0f) carouselAngle -= 360.0f;
        glutPostRedisplay();
    }
    glutTimerFunc(16, timer, 0);  
}
   
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 750);
    glutCreateWindow("3D Amusement Park — Complete");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}