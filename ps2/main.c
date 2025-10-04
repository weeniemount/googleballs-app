#include <tamtypes.h>
#include <kernel.h>
#include <stdio.h>
#include <gs_privileged.h>
#include <gs_gp.h>
#include <draw.h>
#include <graph.h>
#include <packet.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 448

int main(int argc, char *argv[])
{
    packet_t *packet;
    qword_t *q;
    
    // Initialize the graphics system
    graph_vram_clear();
    
    // Set up the screen
    graph_mode_t mode;
    mode.width = SCREEN_WIDTH;
    mode.height = SCREEN_HEIGHT;
    mode.psm = GS_PSM_32;
    mode.x = 0;
    mode.y = 0;
    
    graph_initialize(mode.fbp, mode.width, mode.height, mode.psm, 0, 0);
    
    // Create a packet for drawing
    packet = packet_init(100, PACKET_NORMAL);
    
    int frame = 0;
    
    while(1) {
        // Wait for vsync
        graph_wait_vsync();
        
        // Start a new packet
        q = packet->data;
        
        // Clear screen to white
        q = draw_clear(q, 0, 0, mode.width, mode.height, 255, 255, 255);
        
        // Draw a simple colored rectangle
        color_t color;
        color.r = 255;
        color.g = 0;
        color.b = 0;
        color.a = 128;
        color.q = 1.0f;
        
        // Animated position
        int x = (SCREEN_WIDTH / 2) - 50;
        int y = (SCREEN_HEIGHT / 2) - 50 + (frame % 100) - 50;
        
        q = draw_rect_filled(q, 0, &color, x, y, x + 100, y + 100);
        
        // Send the packet
        q = draw_finish(q);
        
        dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data);
        dma_wait_fast();
        
        frame++;
    }
    
    return 0;
}