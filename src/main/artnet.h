
#ifndef _ARTNET_H_
#define _ARTNET_H_

typedef void (*led_callback)(uint8_t buffer[], uint16_t pixels, uint16_t offset);

void artnet_init(led_callback led_cb, uint32_t num_leds);

#endif /* _ARTNET_H_ */
