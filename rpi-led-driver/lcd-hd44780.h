#ifndef __LCD_HD44780_H__
#define __LCD_HD44780_H__

#include <linux/gpio/consumer.h>

struct lcd_4470 {

    struct lcd_44780_pins {
        struct gpio_desc *rs;
        struct gpio_desc *rw;
        struct gpio_desc *en;
        struct gpio_descs *data;
    } pins;

};

#endif
