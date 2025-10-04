#include <tamtypes.h>
#include <kernel.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <packet.h>
#include <math.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 448

int main(int argc, char *argv[])
{
    framebuffer_t fb;
    zbuffer_t zb;
    packet_t *packet;
    qword_t *q;
    
    // Initialize DMA
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
    
    // Set up frame buffer
    fb.width = SCREEN_WIDTH;
    fb.height = SCREEN_HEIGHT;
    fb.psm = GS_PSM_32;
    fb.mask = 0;
    fb.address = graph_vram_allocate(fb.width, fb.height, fb.psm, GRAPH_ALIGN_PAGE);
    
    // Set up Z buffer
    zb.enable = 0;
    zb.zsm = GS_ZBUF_32;
    zb.address = graph_vram_allocate(fb.width, fb.height, zb.zsm, GRAPH_ALIGN_PAGE);
    zb.mask = 0;
    
    // Initialize the screen
    graph_initialize(fb.address, fb.width, fb.height, fb.psm, 0, 0);
    
    // Create a packet
    packet = packet_init(100, PACKET_NORMAL);
    
    float angle = 0.0f;
    
    while(1) {
        // Wait for vsync
        graph_wait_vsync();
        
        q = packet->data;
        
        // Clear to white
        q = draw_clear(q, 0, 0.0f, 0.0f, (float)fb.width, (float)fb.height, 255, 255, 255);
        
        // Set up drawing
        q = draw_setup_environment(q, 0, &fb, &zb);
        q = draw_primitive_xyrgba(q, 0, 0, 0, 0);
        q = draw_finish(q);
        
        // Draw some colored rectangles
        q = packet->data;
        q = draw_clear(q, 0, 0.0f, 0.0f, (float)fb.width, (float)fb.height, 255, 255, 255);
        
        // Animate position
        angle += 0.05f;
        float cx = (fb.width / 2.0f) + cosf(angle) * 100.0f;
        float cy = (fb.height / 2.0f) + sinf(angle) * 100.0f;
        
        // Draw a red rectangle
        rect_t rect;
        rect.v0.x = cx - 50.0f;
        rect.v0.y = cy - 50.0f;
        rect.v0.z = 0;
        rect.v1.x = cx + 50.0f;
        rect.v1.y = cy + 50.0f;
        rect.v1.z = 0;
        rect.color.r = 255;
        rect.color.g = 0;
        rect.color.b = 0;
        rect.color.a = 128;
        rect.color.q = 1.0f;
        
        q = draw_rect_filled(q, 0, &rect);
        
        // Draw a blue rectangle
        rect.v0.x = (fb.width / 2.0f) - 30.0f;
        rect.v0.y = (fb.height / 2.0f) - 30.0f;
        rect.v1.x = (fb.width / 2.0f) + 30.0f;
        rect.v1.y = (fb.height / 2.0f) + 30.0f;
        rect.color.r = 0;
        rect.color.g = 0;
        rect.color.b = 255;
        rect.color.a = 128;
        
        q = draw_rect_filled(q, 0, &rect);
        
        q = draw_finish(q);
        
        // Send to GS
        dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
        dma_wait_fast();
    }
    
    return 0;
}