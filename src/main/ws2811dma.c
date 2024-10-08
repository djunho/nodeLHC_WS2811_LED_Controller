#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp8266/pin_mux_register.h>
#include "esp_log.h"

// Project
#include "ws2811dma.h"
#include "slc_register.h"
#include "i2s_reg.h"

const uint8_t colorOrder[][3] = {
    [COLOR_RGB] = {0, 1, 2},
    [COLOR_GRB] = {1, 0, 2},
};

// The following definitions is taken from ESP8266_MP3_DECODER demo
// https://github.com/espressif/ESP8266_MP3_DECODER/blob/master/mp3/driver/i2s_freertos.c
// It is requred to set clock to I2S subsystem
void rom_i2c_writeReg_Mask(uint32_t block, uint32_t host_id,
        uint32_t reg_add, uint32_t Msb, uint32_t Lsb, uint32_t indata);

/*
 * Raw WS2811 output data storage:
 * "tape" stores the i2s / ws2811 binary data that is played on loop,
 * including the 50us pause at the end. Each entry in tape stores the
 * data for one one color of an LED (so 32-bit per color which makes
 * 96bit = 12byte per LED)
 */
uint32_t tape[CONFIG_MAXPIXELS * 3 + WS_BREAK_BYTES];
colorOrder_t color_type = COLOR_RGB;

void ws2811dma_init(colorOrder_t color) {
    // I²S module seems to take some time to power up
    //vTaskDelay(100);

    // Reset DMA
    SET_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);
    CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);

    // Clear DMA int flags
    SET_PERI_REG_MASK(SLC_INT_CLR, 0xffffffff);
    CLEAR_PERI_REG_MASK(SLC_INT_CLR, 0xffffffff);

    /*** Prefill tape with zeros ***/
    memset(tape, WS_BIT0 | (WS_BIT0<<4), sizeof(tape) - WS_BREAK_BYTES);
    memset((void *)(((uint32_t)tape) + WS_BREAK_BYTES), 0, WS_BREAK_BYTES);
    color_type = color;
    ESP_LOGI("WS2811", "Color type is %d", color_type);
    ESP_LOGI("WS2811", "%d", colorOrder[color_type][0]);
    ESP_LOGI("WS2811", "%d %d %d", colorOrder[color_type][0], colorOrder[color_type][1], colorOrder[color_type][2]);

    /*
     * DMA (Direct Memory Access) using SLC controller to fill I²S FIFO:
     * Setup and start SLC transmission
     * datalen / blocksize are in bytes
     */
    static struct slcRXDescriptor slc;
    slc.owner = 1;
    slc.buf_ptr = (uint32_t)tape;
    slc.eof = 1;
    slc.sub_sof = 0;
    slc.size = slc.length = sizeof(tape);
    slc.next_link_ptr = (uint32_t)&slc;

    CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE << SLC_MODE_S));
    SET_PERI_REG_MASK(SLC_CONF0, (1 << SLC_MODE_S));
    SET_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_INFOR_NO_REPLACE | SLC_TOKEN_NO_REPLACE);
    CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN | SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);

    SET_PERI_REG_MASK(SLC_RX_LINK, (((uint32_t) &slc) & SLC_RXLINK_DESCADDR_MASK));
    SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);

    /*
     * Setup I²S output:
     * - Choose Function 2 (count from 0) of U0RXD which is I2SO_DATA (I²S Data Output)
     * - Give I²S module clock signal
     * - Setup clock prescaler and frequency divider + MSB + start tape playback
     */
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, 1);
    rom_i2c_writeReg_Mask(0x67, 4, 4, 7, 7, 1);
    // Reset I2S subsystem
    CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
    SET_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
    CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);

    *((uint32_t *)I2SCONF) = I2S_RIGHT_FIRST | I2S_MSB_RIGHT | I2S_I2S_TX_START
                            | (((I2S_CLKM_DIV - 1) & I2S_CLKM_DIV_NUM) << I2S_CLKM_DIV_NUM_S)
                            | (((I2S_BCK_DIV - 1) & I2S_BCK_DIV_NUM) << I2S_BCK_DIV_NUM_S);
}

/*
 * ws2811dma_put: add data to playback tape
 * buffer: raw color data, 3 bytes per pixel
 * pixels: number of pixels in dataset
 * offset: nth LED in String
 */
void ws2811dma_put(uint8_t buffer[], uint16_t pixels, uint16_t offset) {
    if(pixels > CONFIG_MAXPIXELS) return;

    // Fill tape with color data, 32bit of color data per color
    uint16_t px, c;
    uint8_t bit;
    for(px = offset; px < offset + pixels; ++px) {
        for (c = 0; c < 3; ++c) {
            uint8_t color = colorOrder[color_type][c];
            uint32_t pxval = 0x00000000;
            uint8_t colorbyte = buffer[((px - offset)*3) + color];
            for (bit = 0; bit < 8; ++bit)
                pxval |= (((1<<bit) & colorbyte) ? WS_BIT1 : WS_BIT0) << (bit * 4);
            tape[px * 3 + c] = pxval;
        }
    }
}
