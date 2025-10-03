#include <kernel.h>
#include <malloc.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>
#include <math.h>
#include <string.h>
#include <libpad.h>

typedef struct {
    float x, y, z;
} Vector3;

typedef struct {
    u64 color;
} Color;

typedef struct {
    Vector3 curPos;
    Vector3 originalPos;
    Vector3 targetPos;
    Vector3 velocity;
    float radius;
    float size;
    float friction;
    float springStrength;
    Color color;
} Point;

typedef struct {
    Vector3 mousePos;
    Point* points;
    int pointCount;
} PointCollection;

typedef struct {
    int x, y;
    int size;
    u32 colorHex;
} PointData;

// Pad globals
static char padBuf[256] __attribute__((aligned(64)));

u64 hexToColor(u32 hex) {
    u8 r = (hex >> 16) & 0xFF;
    u8 g = (hex >> 8) & 0xFF;
    u8 b = hex & 0xFF;
    u8 a = 0x80;
    return GS_SETREG_RGBAQ(r, g, b, a, 0x00);
}

void computeBounds(const PointData* data, int count, float* w, float* h) {
    int minX = 99999, maxX = -99999;
    int minY = 99999, maxY = -99999;
    
    for (int i = 0; i < count; i++) {
        if (data[i].x < minX) minX = data[i].x;
        if (data[i].x > maxX) maxX = data[i].x;
        if (data[i].y < minY) minY = data[i].y;
        if (data[i].y > maxY) maxY = data[i].y;
    }
    
    *w = maxX - minX;
    *h = maxY - minY;
}

void Point_init(Point* p, float x, float y, float z, float size, u32 colorHex) {
    p->curPos.x = x;
    p->curPos.y = y;
    p->curPos.z = z;
    
    p->originalPos.x = x;
    p->originalPos.y = y;
    p->originalPos.z = z;
    
    p->targetPos.x = x;
    p->targetPos.y = y;
    p->targetPos.z = z;
    
    p->velocity.x = 0;
    p->velocity.y = 0;
    p->velocity.z = 0;
    
    p->size = size;
    p->radius = size;
    p->friction = 0.8f;
    p->springStrength = 0.1f;
    p->color.color = hexToColor(colorHex);
}

void Point_update(Point* p) {
    float dx = p->targetPos.x - p->curPos.x;
    float ax = dx * p->springStrength;
    p->velocity.x += ax;
    p->velocity.x *= p->friction;
    
    if (fabsf(dx) < 0.1f && fabsf(p->velocity.x) < 0.01f) {
        p->curPos.x = p->targetPos.x;
        p->velocity.x = 0;
    } else {
        p->curPos.x += p->velocity.x;
    }
    
    float dy = p->targetPos.y - p->curPos.y;
    float ay = dy * p->springStrength;
    p->velocity.y += ay;
    p->velocity.y *= p->friction;
    
    if (fabsf(dy) < 0.1f && fabsf(p->velocity.y) < 0.01f) {
        p->curPos.y = p->targetPos.y;
        p->velocity.y = 0;
    } else {
        p->curPos.y += p->velocity.y;
    }
    
    float dox = p->originalPos.x - p->curPos.x;
    float doy = p->originalPos.y - p->curPos.y;
    float dd = (dox * dox) + (doy * doy);
    float d = sqrtf(dd);
    
    p->targetPos.z = d / 100.0f + 1.0f;
    float dz = p->targetPos.z - p->curPos.z;
    float az = dz * p->springStrength;
    p->velocity.z += az;
    p->velocity.z *= p->friction;
    
    if (fabsf(dz) < 0.01f && fabsf(p->velocity.z) < 0.001f) {
        p->curPos.z = p->targetPos.z;
        p->velocity.z = 0;
    } else {
        p->curPos.z += p->velocity.z;
    }
    
    p->radius = p->size * p->curPos.z;
    if (p->radius < 1) p->radius = 1;
}

void Point_draw(Point* p, GSGLOBAL* gsGlobal) {
    // Draw as sprite (square for now - circles would need custom rendering)
    gsKit_prim_sprite(gsGlobal, 
                     p->curPos.x - p->radius, 
                     p->curPos.y - p->radius,
                     p->curPos.x + p->radius, 
                     p->curPos.y + p->radius,
                     0, p->color.color);
}

void PointCollection_init(PointCollection* pc, int maxPoints) {
    pc->mousePos.x = 320;
    pc->mousePos.y = 240;
    pc->mousePos.z = 0;
    pc->points = (Point*)malloc(sizeof(Point) * maxPoints);
    pc->pointCount = 0;
}

void PointCollection_addPoint(PointCollection* pc, float x, float y, float z, float size, u32 colorHex) {
    Point_init(&pc->points[pc->pointCount], x, y, z, size, colorHex);
    pc->pointCount++;
}

void PointCollection_update(PointCollection* pc) {
    for (int i = 0; i < pc->pointCount; i++) {
        Point* p = &pc->points[i];
        
        float dx = pc->mousePos.x - p->curPos.x;
        float dy = pc->mousePos.y - p->curPos.y;
        float dd = (dx * dx) + (dy * dy);
        float d = sqrtf(dd);
        
        if (d < 150) {
            p->targetPos.x = p->curPos.x - dx;
            p->targetPos.y = p->curPos.y - dy;
        } else {
            p->targetPos.x = p->originalPos.x;
            p->targetPos.y = p->originalPos.y;
        }
        
        Point_update(p);
    }
}

void PointCollection_draw(PointCollection* pc, GSGLOBAL* gsGlobal) {
    for (int i = 0; i < pc->pointCount; i++) {
        Point_draw(&pc->points[i], gsGlobal);
    }
}

void PointCollection_cleanup(PointCollection* pc) {
    free(pc->points);
}

int initializePad(int port, int slot) {
    int ret;
    int state;
    int waitTime;
    
    padInit(0);
    
    ret = padPortOpen(port, slot, padBuf);
    if(ret == 0) {
        return 0;
    }
    
    // Wait for pad to be ready (with timeout)
    waitTime = 1000;
    while(waitTime > 0) {
        state = padGetState(port, slot);
        if((state == PAD_STATE_STABLE) || (state == PAD_STATE_FINDCTP1)) {
            break;
        }
        waitTime--;
    }
    
    if(waitTime <= 0) {
        return 0;
    }
    
    return 1;
}

int main(int argc, char *argv[]) {
    GSGLOBAL* gsGlobal;
    PointCollection pointCollection;
    struct padButtonStatus buttons;
    int port = 0;
    int slot = 0;
    
    // Initialize DMA
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    
    // Initialize gsKit
    gsGlobal = gsKit_init_global();
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    
    gsGlobal->PSM = GS_PSM_CT32;
    gsGlobal->PSMZ = GS_PSMZ_16S;
    gsGlobal->ZBuffering = GS_SETTING_OFF;
    
    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);
    
    gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gsGlobal, GS_ATEST_OFF);
    
    // Initialize controller (non-blocking, ignore failures)
    initializePad(port, slot);
    
    PointData pointData[] = {
        {202, 78, 9, 0xed9d33}, {348, 83, 9, 0xd44d61}, {256, 69, 9, 0x4f7af2},
        {214, 59, 9, 0xef9a1e}, {265, 36, 9, 0x4976f3}, {300, 78, 9, 0x269230},
        {294, 59, 9, 0x1f9e2c}, {45, 88, 9, 0x1c48dd}, {268, 52, 9, 0x2a56ea},
        {73, 83, 9, 0x3355d8}, {294, 6, 9, 0x36b641}, {235, 62, 9, 0x2e5def},
        {353, 42, 8, 0xd53747}, {336, 52, 8, 0xeb676f}, {208, 41, 8, 0xf9b125},
        {321, 70, 8, 0xde3646}, {8, 60, 8, 0x2a59f0}, {180, 81, 8, 0xeb9c31},
        {146, 65, 8, 0xc41731}, {145, 49, 8, 0xd82038}, {246, 34, 8, 0x5f8af8},
        {169, 69, 8, 0xefa11e}, {273, 99, 8, 0x2e55e2}, {248, 120, 8, 0x4167e4},
        {294, 41, 8, 0x0b991a}, {267, 114, 8, 0x4869e3}, {78, 67, 8, 0x3059e3},
        {294, 23, 8, 0x10a11d}, {117, 83, 8, 0xcf4055}, {137, 80, 8, 0xcd4359},
        {14, 71, 8, 0x2855ea}, {331, 80, 8, 0xca273c}, {25, 82, 8, 0x2650e1},
        {233, 46, 8, 0x4a7bf9}, {73, 13, 8, 0x3d65e7}, {327, 35, 6, 0xf47875},
        {319, 46, 6, 0xf36764}, {256, 81, 6, 0x1d4eeb}, {244, 88, 6, 0x698bf1},
        {194, 32, 6, 0xfac652}, {97, 56, 6, 0xee5257}, {105, 75, 6, 0xcf2a3f},
        {42, 4, 6, 0x5681f5}, {10, 27, 6, 0x4577f6}, {166, 55, 6, 0xf7b326},
        {266, 88, 6, 0x2b58e8}, {178, 34, 6, 0xfacb5e}, {100, 65, 6, 0xe02e3d},
        {343, 32, 6, 0xf16d6f}, {59, 5, 6, 0x507bf2}, {27, 9, 6, 0x5683f7},
        {233, 116, 6, 0x3158e2}, {123, 32, 6, 0xf0696c}, {6, 38, 6, 0x3769f6},
        {63, 62, 6, 0x6084ef}, {6, 49, 6, 0x2a5cf4}, {108, 36, 6, 0xf4716e},
        {169, 43, 6, 0xf8c247}, {137, 37, 6, 0xe74653}, {318, 58, 6, 0xec4147},
        {226, 100, 5, 0x4876f1}, {101, 46, 5, 0xef5c5c}, {226, 108, 5, 0x2552ea},
        {17, 17, 5, 0x4779f7}, {232, 93, 5, 0x4b78f1}
    };
    
    int pointCount = sizeof(pointData) / sizeof(PointData);
    
    PointCollection_init(&pointCollection, pointCount);
    
    float logoW, logoH;
    computeBounds(pointData, pointCount, &logoW, &logoH);
    
    float offsetX = (gsGlobal->Width / 2.0f) - (logoW / 2.0f);
    float offsetY = (gsGlobal->Height / 2.0f) - (logoH / 2.0f);
    
    for (int i = 0; i < pointCount; i++) {
        float x = offsetX + pointData[i].x;
        float y = offsetY + pointData[i].y;
        PointCollection_addPoint(&pointCollection, x, y, 0.0f, (float)pointData[i].size, pointData[i].colorHex);
    }
    
    // Set initial cursor position
    pointCollection.mousePos.x = gsGlobal->Width / 2.0f;
    pointCollection.mousePos.y = gsGlobal->Height / 2.0f;
    
    int frameCount = 0;
    float cursorSpeed = 5.0f;
    
    while(1) {
        frameCount++;
        
        // Read controller
        int ret = padRead(port, slot, &buttons);
        if (ret != 0) {
            int paddata = 0xffff ^ buttons.btns;
            
            // D-Pad movement
            if (paddata & PAD_LEFT) {
                pointCollection.mousePos.x -= cursorSpeed;
            }
            if (paddata & PAD_RIGHT) {
                pointCollection.mousePos.x += cursorSpeed;
            }
            if (paddata & PAD_UP) {
                pointCollection.mousePos.y -= cursorSpeed;
            }
            if (paddata & PAD_DOWN) {
                pointCollection.mousePos.y += cursorSpeed;
            }
            
            // Analog stick (more precise control)
            if (buttons.ljoy_h < 100) {
                pointCollection.mousePos.x -= cursorSpeed * 0.5f;
            }
            if (buttons.ljoy_h > 200) {
                pointCollection.mousePos.x += cursorSpeed * 0.5f;
            }
            if (buttons.ljoy_v < 100) {
                pointCollection.mousePos.y -= cursorSpeed * 0.5f;
            }
            if (buttons.ljoy_v > 200) {
                pointCollection.mousePos.y += cursorSpeed * 0.5f;
            }
        }
        
        // Clamp cursor to screen
        if (pointCollection.mousePos.x < 0) pointCollection.mousePos.x = 0;
        if (pointCollection.mousePos.x >= gsGlobal->Width) pointCollection.mousePos.x = gsGlobal->Width - 1;
        if (pointCollection.mousePos.y < 0) pointCollection.mousePos.y = 0;
        if (pointCollection.mousePos.y >= gsGlobal->Height) pointCollection.mousePos.y = gsGlobal->Height - 1;
        
        // Update physics
        PointCollection_update(&pointCollection);
        
        // Render
        gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x80, 0x00));
        
        PointCollection_draw(&pointCollection, gsGlobal);
        
        // Draw cursor
        gsKit_prim_sprite(gsGlobal,
                         pointCollection.mousePos.x - 3,
                         pointCollection.mousePos.y - 3,
                         pointCollection.mousePos.x + 3,
                         pointCollection.mousePos.y + 3,
                         0, GS_SETREG_RGBAQ(0xFF, 0x00, 0x00, 0x80, 0x00));
        
        gsKit_sync_flip(gsGlobal);
        gsKit_queue_exec(gsGlobal);
    }
    
    PointCollection_cleanup(&pointCollection);
    
    return 0;
}