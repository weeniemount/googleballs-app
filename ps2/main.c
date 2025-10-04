#include <tamtypes.h>
#include <kernel.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <packet.h>
#include <gs_psm.h>

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
    fb.width = 640;
    fb.height = 448;
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
    packet = packet_init(50, PACKET_NORMAL);
    
    while(1) {
        graph_wait_vsync();
        
        q = packet->data;
        
        // Just clear to red
        q = draw_clear(q, 0, 0.0f, 0.0f, 640.0f, 448.0f, 255, 0, 0);
        q = draw_finish(q);
        
        dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
        dma_wait_fast();
    }
    
    return 0;
}