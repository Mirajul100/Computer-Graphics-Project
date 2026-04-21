#include <GL/glut.h>
#include <cmath>
#include <cstring>

// ── ALGORITHM 1: DDA LINE ── float increment, best for diagonal/angled lines
void drawLineDDA(float x1, float y1, float x2, float y2) {
    float dx = x2-x1, dy = y2-y1;
    float steps = (fabsf(dx) >= fabsf(dy)) ? fabsf(dx) : fabsf(dy);
    if (steps == 0.0f) { glBegin(GL_POINTS); glVertex2f(x1,y1); glEnd(); return; }
    float xInc = dx/steps, yInc = dy/steps, x = x1, y = y1;
    glBegin(GL_POINTS);
    for (int i = 0; i <= (int)steps; i++) {
        glVertex2f(roundf(x), roundf(y));
        x += xInc; y += yInc;
    }
    glEnd();
}

// ── ALGORITHM 2: BRESENHAM LINE ── integer only, best for straight/axis lines
void drawLineBresenham(int x1, int y1, int x2, int y2) {
    int dx=abs(x2-x1), dy=abs(y2-y1);
    int sx=(x1<x2)?1:-1, sy=(y1<y2)?1:-1, err=dx-dy;
    glBegin(GL_POINTS);
    while (true) {
        glVertex2f(x1, y1);
        if (x1==x2 && y1==y2) break;
        int e2 = 2*err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
    glEnd();
}

// ── ALGORITHM 3: MIDPOINT CIRCLE ── 8-way symmetry, no sin/cos needed
void drawFilledCircle(int cx, int cy, int r) {
    int x=0, y=r, d=1-r;
    while (x <= y) {
        drawLineBresenham(cx-y, cy+x, cx+y, cy+x);  // [BRE] scanline
        drawLineBresenham(cx-y, cy-x, cx+y, cy-x);
        drawLineBresenham(cx-x, cy+y, cx+x, cy+y);
        drawLineBresenham(cx-x, cy-y, cx+x, cy-y);
        if (d < 0) d += 2*x+3;
        else { d += 2*(x-y)+5; y--; }
        x++;
    }
}

void drawCircleOutline(int cx, int cy, int r) {   // rim outline only [MPC]
    int x=0, y=r, d=1-r;
    glBegin(GL_POINTS);
    while (x <= y) {
        glVertex2f(cx+x,cy+y); glVertex2f(cx-x,cy+y);
        glVertex2f(cx+x,cy-y); glVertex2f(cx-x,cy-y);
        glVertex2f(cx+y,cy+x); glVertex2f(cx-y,cy+x);
        glVertex2f(cx+y,cy-x); glVertex2f(cx-y,cy-x);
        if (d < 0) d += 2*x+3;
        else { d += 2*(x-y)+5; y--; }
        x++;
    }
    glEnd();
}

void drawFilledEllipse(int cx, int cy, int rx, int ry) {  // [MPC] two-region
    long long rx2=(long long)rx*rx, ry2=(long long)ry*ry;
    int x=0, y=ry;
    long long d1 = ry2 - rx2*ry + rx2/4;
    while (2*ry2*x < 2*rx2*y) {
        drawLineBresenham(cx-x,cy+y,cx+x,cy+y);
        drawLineBresenham(cx-x,cy-y,cx+x,cy-y);
        if (d1 < 0) d1 += 2*ry2*x+3*ry2;
        else { d1 += 2*ry2*x-2*rx2*y+3*ry2+2*rx2; y--; }
        x++;
    }
    long long d2 = ry2*x*x + rx2*(long long)(y-1)*(y-1) - rx2*ry2;
    while (y >= 0) {
        drawLineBresenham(cx-x,cy+y,cx+x,cy+y);
        drawLineBresenham(cx-x,cy-y,cx+x,cy-y);
        if (d2 > 0) d2 -= 2*rx2*y-3*rx2;
        else { d2 += 2*ry2*x-2*rx2*y-3*rx2+2*ry2; x++; }
        y--;
    }
}

void fillRect(int x1, int y1, int x2, int y2) {  // [BRE] horizontal scanlines
    if (y1>y2){int t=y1;y1=y2;y2=t;} if (x1>x2){int t=x1;x1=x2;x2=t;}
    for (int row=y1; row<=y2; row++) drawLineBresenham(x1,row,x2,row);
}

// ── GLOBALS ──
float lightIntensity = 1.0f;
bool  decreasing     = true;
float car1X=-600.0f, car2X=600.0f;
const float carY = -150.0f;

struct Bird { float x,y,vx,wingPhase; };
Bird birds[3] = { {-420,320,1.6f,0}, {-560,360,1.2f,1}, {-500,400,1.8f,2} };

float cloud1X=-300.0f, cloud2X=100.0f;

struct Pt { float x,y; };
Pt walkway[4] = { {35,0},{65,0},{90,-120},{10,-120} };  // wide at road, narrow at door

const int fieldLeftX=-260, fieldRightX=360, fieldTopY=-20, fieldBottomY=-118;
const int fenceLeft=-240,  fenceRight=340,  fenceTop=-30,  fenceBottom=-110;

struct Kid { float x,y,vx,armSwing; bool facingRight; float sR,sG,sB; };
Kid leftKids[5] = {
    {-220,-60, 0.8f,0,true, 0.10f,0.40f,0.90f},
    {-180,-80,-0.7f,0,false,0.90f,0.30f,0.20f},
    {-140,-50, 0.6f,0,true, 0.20f,0.80f,0.30f},
    {-100,-95,-0.5f,0,false,0.85f,0.65f,0.10f},
    { -60,-70, 0.9f,0,true, 0.30f,0.60f,0.95f}
};
Kid rightKids[5] = {
    { 120,-60,-0.6f,0,false,0.10f,0.40f,0.90f},
    { 160,-90, 0.7f,0,true, 0.90f,0.30f,0.20f},
    { 200,-50,-0.5f,0,false,0.20f,0.80f,0.30f},
    { 240,-95, 0.8f,0,true, 0.85f,0.65f,0.10f},
    { 280,-70,-0.9f,0,false,0.30f,0.60f,0.95f}
};
struct BackpackKid { float t; } bpKid = {0.0f};

void init() {
    glClearColor(0.53f,0.81f,0.98f,1.0f);  // sky blue
    gluOrtho2D(-500,500,-300,500);
    glPointSize(1.5f);
}

// ── SUN: disc [MPC], rays [DDA] ──
void drawSun() {
    const int cx=350, cy=400, r=35;
    float rayLen = 20.0f*lightIntensity+5.0f;
    glColor3f(1.0f,0.87f,0.13f);
    drawFilledCircle(cx,cy,r);                        // [MPC] disc
    glColor3f(1.0f,0.92f,0.30f);
    for (int i=0; i<12; i++) {
        float a = 2.0f*3.14159265f*i/12;
        drawLineDDA(cx+cosf(a)*(r+6), cy+sinf(a)*(r+6),  // [DDA] ray
                    cx+cosf(a)*(r+6+rayLen), cy+sinf(a)*(r+6+rayLen));
    }
}

// ── GROUND & ROAD: [BRE] fills ──
void drawGround() {
    glColor3f(0.20f,0.75f,0.20f);
    fillRect(-500,-120,500,0);                        // [BRE]
}
void drawRoad() {
    glColor3f(0.12f,0.12f,0.12f);
    fillRect(-500,-220,500,-121);                     // [BRE] surface
    glColor3f(1,1,1);
    for (int x=-450; x<=390; x+=150)
        fillRect(x,-175,x+60,-165);                   // [BRE] dashes
}

// ── MOUNTAINS: fill [BRE], outline [DDA] ──
void drawMountain(int xL, int xR, int peak) {
    int xM=(xL+xR)/2;
    glColor3f(0.55f,0.38f,0.18f);
    for (int y=0; y<=peak; y++) {                     // [BRE] fill scanlines
        float f=(float)y/peak;
        drawLineBresenham((int)(xL+f*(xM-xL)), y, (int)(xR-f*(xR-xM)), y);
    }
    glColor3f(0.38f,0.25f,0.10f);
    drawLineDDA(xL,0,xM,peak);                        // [DDA] left slope
    drawLineDDA(xR,0,xM,peak);                        // [DDA] right slope
}
void drawMountains() {
    drawMountain(-450,-150,200);
    drawMountain(-200, 200,250);
    drawMountain( 150, 500,200);
}

// ── TREE: trunk [BRE], foliage fill [BRE], edges [DDA] ──
void drawTree(int tx, int ty) {
    glColor3f(0.45f,0.28f,0.10f);
    fillRect(tx-10,ty,tx+10,ty+40);                  // [BRE] trunk
    glColor3f(0.05f,0.50f,0.05f);
    for (int fy=ty+40; fy<=ty+120; fy++) {           // [BRE] foliage fill
        float f=(float)(fy-(ty+40))/80.0f;
        drawLineBresenham(tx-(int)(40*f),fy,tx+(int)(40*f),fy);
    }
    glColor3f(0.00f,0.35f,0.00f);
    drawLineDDA(tx,ty+40,tx-40,ty+120);              // [DDA] left edge
    drawLineDDA(tx,ty+40,tx+40,ty+120);              // [DDA] right edge
    drawLineBresenham(tx-40,ty+120,tx+40,ty+120);    // [BRE] base
}

// ── HOUSE: walls/door/window [BRE], roof fill [BRE], roof edges [DDA] ──
void drawHouse() {
    glColor3f(0.40f,0.40f,0.80f);
    fillRect(-400,0,-250,150);                        // [BRE] wall
    glColor3f(0.80f,0.12f,0.12f);
    for (int y=150; y<=230; y++) {                    // [BRE] roof fill
        float f=(float)(y-150)/80.0f;
        drawLineBresenham((int)(-420+f*95),y,(int)(-230-f*95),y);
    }
    glColor3f(0.55f,0.05f,0.05f);
    drawLineDDA(-420,150,-325,230);                   // [DDA] roof left
    drawLineDDA(-230,150,-325,230);                   // [DDA] roof right
    glColor3f(0.45f,0.18f,0.10f);
    fillRect(-350,0,-300,70);                         // [BRE] door
    glColor3f(0.85f,0.92f,1.00f);
    fillRect(-390,80,-270,130);                       // [BRE] window
    glColor3f(0.50f,0.70f,0.90f);
    drawLineBresenham(-390,80,-270,80);   drawLineBresenham(-390,130,-270,130);  // [BRE]
    drawLineBresenham(-390,80,-390,130); drawLineBresenham(-270,80,-270,130);
    drawLineBresenham(-330,80,-330,130); drawLineBresenham(-390,105,-270,105);
}

// ── SCHOOL: all [BRE] ──
void drawSchool() {
    const int bL=-180,bR=280,bBot=0,bTop=280;
    glColor3f(0.05f,0.10f,0.45f); fillRect(bL,bBot,bR,bTop);         // [BRE] body
    glColor3f(0.95f,0.95f,0.20f); fillRect(bL-20,bTop,bR+20,bTop+50);// [BRE] sign band
    glColor3f(0.88f,0.88f,0.88f); fillRect(30,bBot,70,bBot+90);       // [BRE] door
    glColor3f(0.55f,0.55f,0.55f);
    drawLineBresenham(30,bBot,70,bBot);   drawLineBresenham(30,bBot+90,70,bBot+90); // [BRE]
    drawLineBresenham(30,bBot,30,bBot+90);drawLineBresenham(70,bBot,70,bBot+90);

    const int cols=4,floorH=80,wW=40,wH=30;
    const int xStart=bL+20, xGap=((bR-bL)-40-cols*wW)/(cols-1);
    for (int f=0; f<3; f++) {
        int wy=bBot+30+f*(floorH+10);
        for (int c=0; c<cols; c++) {
            int wx=xStart+c*(wW+xGap);
            glColor3f(0.85f,0.92f,1.00f); fillRect(wx,wy,wx+wW,wy+wH); // [BRE] window
            glColor3f(0.45f,0.65f,0.90f);
            drawLineBresenham(wx,wy,wx+wW,wy);     drawLineBresenham(wx,wy+wH,wx+wW,wy+wH); // [BRE]
            drawLineBresenham(wx,wy,wx,wy+wH);     drawLineBresenham(wx+wW,wy,wx+wW,wy+wH);
            drawLineBresenham(wx+wW/2,wy,wx+wW/2,wy+wH);
            drawLineBresenham(wx,wy+wH/2,wx+wW,wy+wH/2);
        }
    }
    glColor3f(0,0,0);
    const char* text="USD PRIMARY SCHOOL";
    float tw=0; for(int i=0;text[i];i++) tw+=glutBitmapWidth(GLUT_BITMAP_HELVETICA_18,text[i]);
    glRasterPos2f((bL-20)+((bR+20-(bL-20))-tw)/2.0f, bTop+18);
    for(int i=0;text[i];i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,text[i]);
}

// ── FIELDS + FENCE: all [BRE] ──
void drawFieldsAndFence() {
    glColor3f(0.15f,0.58f,0.18f);
    fillRect(fieldLeftX,fieldBottomY,fieldRightX,fieldTopY);          // [BRE] grass
    glColor3f(0.82f,0.82f,0.82f);
    drawLineBresenham(fenceLeft,fenceTop,   fenceRight,fenceTop);     // [BRE] fence top
    drawLineBresenham(fenceLeft,fenceBottom,fenceRight,fenceBottom);  // [BRE] fence bottom
    drawLineBresenham(fenceLeft,fenceTop,   fenceLeft, fenceBottom);  // [BRE] fence left
    drawLineBresenham(fenceRight,fenceTop,  fenceRight,fenceBottom);  // [BRE] fence right
    for (int px=fenceLeft; px<=fenceRight; px+=28)
        drawLineBresenham(px,fenceTop,px,fenceBottom);                // [BRE] pickets
    drawLineBresenham(-180,0,-180,-25);  drawLineBresenham(-180,-25,fenceLeft,-25);  // [BRE]
    drawLineBresenham(280,0,280,-25);    drawLineBresenham(280,-25,fenceRight,-25);
}

// ── WALKWAY: fill [BRE], edges [DDA] ──
void drawWalkway() {
    glColor3f(0.72f,0.72f,0.72f);
    for (int row=-120; row<=0; row++) {                               // [BRE] fill
        float t=(float)(row+120)/120.0f;
        int lx=(int)(walkway[3].x+t*(walkway[0].x-walkway[3].x));
        int rx=(int)(walkway[2].x+t*(walkway[1].x-walkway[2].x));
        drawLineBresenham(lx,row,rx,row);
    }
    glColor3f(0.50f,0.50f,0.50f);
    drawLineDDA(walkway[0].x,0,walkway[3].x,-120);                   // [DDA] left edge
    drawLineDDA(walkway[1].x,0,walkway[2].x,-120);                   // [DDA] right edge
}

// ── STREET LIGHT: pole & lamp [BRE] ──
void drawStreetLight(int lx) {
    glColor3f(0.30f,0.30f,0.30f);
    fillRect(lx-3,-120,lx+3,-55);                                    // [BRE] pole
    float li=lightIntensity;
    glColor3f(1.0f*li,1.0f*li,0.28f*li);
    fillRect(lx-10,-55,lx+10,-35);                                   // [BRE] lamp
    glColor3f(0.35f,0.35f,0.35f);
    drawLineBresenham(lx-10,-55,lx+10,-55); drawLineBresenham(lx-10,-35,lx+10,-35); // [BRE]
    drawLineBresenham(lx-10,-55,lx-10,-35); drawLineBresenham(lx+10,-55,lx+10,-35);
}

// ── CAR: body [BRE], wheels [MPC] ──
void drawCar(float fx, float fy, float cr, float cg, float cb) {
    int bx=(int)fx, by=(int)fy;
    glColor3f(cr,cg,cb);
    fillRect(bx,by,bx+80,by+15);                                     // [BRE] base
    for (int row=by+15; row<=by+30; row++) {                         // [BRE] cabin
        float f=(float)(row-(by+15))/15.0f;
        drawLineBresenham(bx+10+(int)(10*f),row, bx+70-(int)(10*f),row);
    }
    glColor3f(0.55f,0.82f,0.98f); fillRect(bx+23,by+18,bx+57,by+28);// [BRE] windshield
    glColor3f(0.08f,0.08f,0.08f); fillRect(bx+20,by-2,bx+60,by+2);  // [BRE] underbody
    glColor3f(0.06f,0.06f,0.06f);
    drawFilledCircle(bx+16,by-11,11); drawFilledCircle(bx+64,by-11,11); // [MPC] tyres
    glColor3f(0.60f,0.60f,0.60f);
    drawFilledCircle(bx+16,by-11,5);  drawFilledCircle(bx+64,by-11,5);  // [MPC] hubcaps
    glColor3f(0.25f,0.25f,0.25f);
    drawCircleOutline(bx+16,by-11,11); drawCircleOutline(bx+64,by-11,11); // [MPC] rim
}

// ── BIRD: body [MPC], wings/beak [DDA] ──
void drawBird(float bx, float by, float wingPhase) {
    glColor3f(0.12f,0.12f,0.12f);
    drawFilledEllipse((int)bx,(int)by,11,6);                         // [MPC] body
    float flap=sinf(wingPhase)*12.0f+7.0f;
    glColor3f(0.20f,0.20f,0.20f);
    drawLineDDA(bx-4,by+2,bx-flap,by+flap*0.55f);                   // [DDA] left wing
    drawLineDDA(bx+4,by+2,bx+flap,by+flap*0.55f);                   // [DDA] right wing
    glColor3f(0.88f,0.68f,0.10f);
    drawLineDDA(bx+11,by+2,bx+19,by+3);                             // [DDA] beak upper
    drawLineDDA(bx+11,by+2,bx+19,by+1);                             // [DDA] beak lower
}

// ── CLOUD: 3 ellipses [MPC] ──
void drawCloud(float cx, float cy) {
    glColor3f(0.93f,0.93f,0.93f);
    drawFilledEllipse((int)cx,   (int)cy,   48,22); // [MPC]
    drawFilledEllipse((int)cx+55,(int)cy+8, 42,20); // [MPC]
    drawFilledEllipse((int)cx-55,(int)cy+12,42,20); // [MPC]
}

// ── STUDENT: head [MPC], body [BRE], arms [DDA], legs [BRE] ──
void drawStudent(float fx, float fy, bool facingRight, float armSwing,
                 float sR=0.10f, float sG=0.40f, float sB=0.90f) {
    int sx=(int)fx, sy=(int)fy;
    glColor3f(0.95f,0.80f,0.60f);
    drawFilledCircle(sx,sy+15,7);                                    // [MPC] head
    glColor3f(sR,sG,sB);   fillRect(sx-7,sy-6,sx+7,sy+8);           // [BRE] shirt
    glColor3f(0.10f,0.10f,0.12f); fillRect(sx-7,sy-17,sx+7,sy-6);   // [BRE] pants
    float sw=sinf(armSwing)*7.0f;
    glColor3f(0.95f,0.80f,0.60f);
    drawLineDDA(sx-7,sy+2,sx-16,sy+2+sw);                           // [DDA] left arm
    drawLineDDA(sx+7,sy+2,sx+16,sy+2-sw);                           // [DDA] right arm
    drawLineBresenham(sx-3,sy-17,sx-4,(int)(sy-29-sw*0.4f));        // [BRE] left leg
    drawLineBresenham(sx+3,sy-17,sx+4,(int)(sy-29+sw*0.4f));        // [BRE] right leg
    glColor3f(0,0,0);
    float eo=facingRight?2.5f:-2.5f;
    glBegin(GL_POINTS);
    glVertex2f(sx+eo,sy+16); glVertex2f(sx+eo+(facingRight?3.5f:-3.5f),sy+16);
    glEnd();
}

// ── BACKPACK KID: body [student], pack [BRE], strap [DDA] ──
void drawBackpackKid(float t) {
    float x=(walkway[2].x+walkway[3].x)*0.5f
           +t*((walkway[0].x+walkway[1].x)*0.5f-(walkway[2].x+walkway[3].x)*0.5f);
    float y=-120.0f+t*120.0f;
    drawStudent(x,y,true,t*12.566f,0.15f,0.45f,0.85f);
    glColor3f(0.28f,0.22f,0.12f);
    fillRect((int)(x-10),(int)(y-9),(int)(x+1),(int)(y+5));         // [BRE] backpack
    glColor3f(0.18f,0.14f,0.07f);
    drawLineDDA(x+1,y+5,x+8,y-7);                                   // [DDA] strap
}

// ── DISPLAY ──
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawSun(); drawGround(); drawMountains();
    drawCloud(cloud1X,360); drawCloud(cloud2X,420);
    drawHouse(); drawSchool();
    drawTree(-480,0); drawTree(-210,0); drawTree(370,0);
    drawFieldsAndFence(); drawWalkway(); drawRoad();
    drawStreetLight(-400); drawStreetLight(-200); drawStreetLight(0);
    drawStreetLight(200);  drawStreetLight(400);
    drawCar(car1X,carY,0.95f,0.10f,0.10f);
    drawCar(car2X,carY,0.10f,0.75f,0.20f);
    for (int i=0;i<3;i++) drawBird(birds[i].x,birds[i].y,birds[i].wingPhase);
    for (int i=0;i<5;i++) drawStudent(leftKids[i].x, leftKids[i].y,
        leftKids[i].facingRight,leftKids[i].armSwing,
        leftKids[i].sR,leftKids[i].sG,leftKids[i].sB);
    for (int i=0;i<5;i++) drawStudent(rightKids[i].x,rightKids[i].y,
        rightKids[i].facingRight,rightKids[i].armSwing,
        rightKids[i].sR,rightKids[i].sG,rightKids[i].sB);
    drawBackpackKid(bpKid.t);
    glutSwapBuffers();
}

// ── TIMER: animation every 50ms ──
void timer(int) {
    if (decreasing) lightIntensity-=0.05f; else lightIntensity+=0.05f;
    if (lightIntensity<=0.20f) decreasing=false;
    if (lightIntensity>=1.00f) decreasing=true;

    car1X+=5.0f; if(car1X> 650.0f) car1X=-650.0f;
    car2X-=5.0f; if(car2X<-650.0f) car2X= 650.0f;
    cloud1X+=0.8f; if(cloud1X>650.0f) cloud1X=-650.0f;
    cloud2X+=0.8f; if(cloud2X>650.0f) cloud2X=-650.0f;

    for (int i=0;i<3;i++) {
        birds[i].x+=birds[i].vx;
        birds[i].wingPhase+=0.22f+0.04f*i;
        if(birds[i].wingPhase>6.28318f) birds[i].wingPhase-=6.28318f;
        if(birds[i].x>520.0f) birds[i].x=-520.0f;
    }

    const float lbL=fieldLeftX+20.0f, rbL=(walkway[3].x+walkway[2].x)*0.5f-18.0f;
    for (int i=0;i<5;i++) {
        leftKids[i].x+=leftKids[i].vx; leftKids[i].armSwing+=0.18f;
        if(leftKids[i].armSwing>6.28318f) leftKids[i].armSwing-=6.28318f;
        if(leftKids[i].x<lbL){leftKids[i].x=lbL;leftKids[i].vx= fabsf(leftKids[i].vx);leftKids[i].facingRight=true;}
        if(leftKids[i].x>rbL){leftKids[i].x=rbL;leftKids[i].vx=-fabsf(leftKids[i].vx);leftKids[i].facingRight=false;}
    }

    const float lbR=(walkway[3].x+walkway[2].x)*0.5f+18.0f, rbR=fieldRightX-20.0f;
    for (int i=0;i<5;i++) {
        rightKids[i].x+=rightKids[i].vx; rightKids[i].armSwing+=0.18f;
        if(rightKids[i].armSwing>6.28318f) rightKids[i].armSwing-=6.28318f;
        if(rightKids[i].x<lbR){rightKids[i].x=lbR;rightKids[i].vx= fabsf(rightKids[i].vx);rightKids[i].facingRight=true;}
        if(rightKids[i].x>rbR){rightKids[i].x=rbR;rightKids[i].vx=-fabsf(rightKids[i].vx);rightKids[i].facingRight=false;}
    }

    bpKid.t+=0.008f; if(bpKid.t>=1.0f) bpKid.t=0.0f;
    glutPostRedisplay();
    glutTimerFunc(50,timer,0);
}

// ── MAIN ──
int main(int argc, char** argv) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(900,700);
    glutCreateWindow("USD Primary School - DDA + Bresenham + Midpoint Circle");
    init();
    glutDisplayFunc(display);
    glutTimerFunc(50,timer,0);
    glutMainLoop();
    return 0;
}
