#include <linux/module.h>

// Эта функция вызывается при инициализации модуля
static int __init rpi_led_init(void)
{
    pr_info("Driver loaded\n");
    return 0;
}

// Эта функция вызывается при деинициализации модуля модуля
static void __exit rpi_led_exit(void)
{
    pr_info("Driver unloaded\n");
}

module_init(rpi_led_init);
module_exit(rpi_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Markov Alexey <markovalex95@gmail.com>");

MODULE_DESCRIPTION("Simple test driver for Raspberry Pi");
