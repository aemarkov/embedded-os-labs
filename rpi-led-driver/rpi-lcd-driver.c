#include "linux/err.h"
#include "linux/export.h"
#include <linux/module.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

#include "lcd-hd44780.h"

#define DEVICE_NAME     "rpi_lcd"         /* Название файла в /dev */

#define IS_ERR_OR_NULL(value) (IS_ERR((value)) || (value) == NULL)

static int rpi_open(struct inode *inode, struct file *file);
static int rpi_release(struct inode *inode, struct file *file);
static ssize_t rpi_write(struct file *file, const char __user *buffer,
    size_t count, loff_t *offp);

static int __init rpi_lcd_probe(struct platform_device *pdev);
static int __exit rpi_lcd_remove(struct platform_device *pdev);

static void rpi_driver_deinit(void);

/* Структура с указателями функции работы с файлами, которые мы реализовали.
   По-сути, это описание того, как мы наследлвали интерфейс */
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = rpi_open,
    .release = rpi_release,
    .write = rpi_write,
};

/* Структура, описывающая "miscellaneous device". Это упрощенная версия символьного
   устройства для простых драйверов, которые не относятся к какому-то конкретному
   классу устройств (т.е. "прочее"). misc device автоматически создает inode
   в /dev, без необходимости вручную регистрировать major и minor номера устройства
*/
static struct miscdevice rpi_miscdevice = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &fops
};

/* Список устройств в Device Tree, которые совместимы с нашим драйвером.
   Если соответствующее устройство будет обнаружено, наш драйвер будет загружен
*/
static const struct of_device_id supported_devices[] = {
     {.compatible = "tarasov-expansion-lcd"},
     {}
};
MODULE_DEVICE_TABLE(of, supported_devices);

/* Структура, описывающая platform device driver */
static struct platform_driver rpi_lcd_platform_driver = {
    .probe = rpi_lcd_probe,                     /* Функция, которая вызывается при загрузке */
    .remove = rpi_lcd_remove,                   /* Функция, которая вызывается при удалении */
    .driver = {
        .name = DEVICE_NAME,                    /* Фиг его знает... */
        .of_match_table = supported_devices,    /* Список поддерживаемых устройств */
        .owner = THIS_MODULE
    }
};

struct lcd_4470 lcd;

/* Драйверы реализуются аналогично реализации интерфейса, только на Си. Линукс
   имеет ряд стандартных типо драйверов, а мы должны подготовить соответствующие
   структуры, функции, всё это настроить, и тогда Линукс сможет работать с нашим
   драйвером. Мы создаем т.н. символьное устройство (character device). Это самый
   простой тип драйвера. Этот драйвер работает как обычный текстовый файл, который
   можно открывать/закрывать, писать/читать итп. Только в отличие от "настоящего"*
   файла, будут вызываться функции, которые мы определим.

   Примечание
   * В линуксе "все есть файл". С точки зрения юзерспейса наш драйвер - обычный
     файл. С точки зрения ядра работа со всеми файлами тоже аналогична. Происходит
     соответсвующий ситемный вызов (open, write итп), а ядро вызывает соответсвующую
     функцию из какого-то драйвера. Это может быть как наш драйвер, и его "файл"
     представлен в виртуальной файловой системе, так и драйвер настоящей файловой
     системы, которая уже обратится к драйверу диска, который запишет данные на
     диск
*/

/**
 * @brief Функция вызывается при открытии файла устройства
 * @param inode
 * @param file
 * @retval
 */
static int rpi_open(struct inode *inode, struct file *file)
{
    pr_info("open\n");
    return 0;
}

/**
 * @brief Функция вызывается при удалении всех ссылок на файл (когда все закрывают)
 * @param inode
 * @param file
 * @retval
 */
static int rpi_release(struct inode *inode, struct file *file)
{
    pr_info("close\n");
    return 0;
}

/**
 * @brief Функция вызывается при записи в файл
 * @param file
 * @param buffer данные для записи
 * @param count  количество байт для записи
 * @param offp   положение указателя ("курсора") в файле
 */
static ssize_t rpi_write(struct file *file, const char __user *buffer,
    size_t count, loff_t *offp)
{
    return -EINVAL;
}

static void rpi_deinit_pins()
{
    if (!IS_ERR_OR_NULL(lcd.pins.rs)) {
        gpiod_put(lcd.pins.rs);
    }
    if (!IS_ERR_OR_NULL(lcd.pins.rw)) {
        gpiod_put(lcd.pins.rw);
    }
    if (!IS_ERR_OR_NULL(lcd.pins.en)) {
        gpiod_put(lcd.pins.en);
    }
    if (!IS_ERR_OR_NULL(lcd.pins.data)) {
        gpiod_put_array(lcd.pins.data);
    }
}

static int rpi_get_pins(struct platform_device *pdev)
{
    struct device *device = &pdev->dev;
    lcd.pins.rs = gpiod_get(device, "rs", GPIOD_OUT_LOW);
    lcd.pins.rw = gpiod_get(device, "rw", GPIOD_OUT_LOW);
    lcd.pins.en = gpiod_get(device, "en", GPIOD_OUT_LOW);
    lcd.pins.data = gpiod_get_array(device, "data", GPIOD_OUT_LOW);

    if (IS_ERR_OR_NULL(lcd.pins.rs) ||
        IS_ERR_OR_NULL(lcd.pins.rw) ||
        IS_ERR_OR_NULL(lcd.pins.en) ||
        IS_ERR_OR_NULL(lcd.pins.data)) {
        pr_err("Failed to get LCD pins\n");
        goto err;
    }

    return 0;
err:
    rpi_deinit_pins();
    return -1;
}

/* Инициализация драйвера */
static int rpi_driver_init(struct platform_device *pdev)
{
    int ret;

    if (rpi_get_pins(pdev) != 0) {
        goto err;
    }

    ret = misc_register(&rpi_miscdevice);
    if (ret != 0) {
        pr_err("Failed to register misc device\n");
        goto deinit_gpio;
    }

    pr_info("Driver initialized\n");

    return 0;
deinit_gpio:
    rpi_deinit_pins();
err:
    pr_err("Failed to initialize driver\n");
    return -1;
}

/* Деинициализация драйвера */
static void rpi_driver_deinit(void)
{
    rpi_deinit_pins();
    misc_deregister(&rpi_miscdevice);
    pr_info("Driver deinitialized\n");
}

/* Функция вызывается, когда подходящее устройство обнаружено */
static int __init rpi_lcd_probe(struct platform_device *pdev)
{
    pr_info("Driver probed\n");
    return rpi_driver_init(pdev);
}

/* Функция вызывается, когда устройство удалено, либо драйвер выгружен */
static int __exit rpi_lcd_remove(struct platform_device *pdev)
{
    pr_info("Driver is removing\n");
    rpi_driver_deinit();
    return 0;
}

static int __init rpi_lcd_init()
{
    pr_info("init\n");
    return 0;
}

static void __exit rpi_lcd_exit()
{
    pr_info("exit\n");
}

/* Регистрация platform device driver */
// module_platform_driver(rpi_lcd_platform_driver);
module_init(rpi_lcd_init);
module_exit(rpi_lcd_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Markov Alexey <markovalex95@gmail.com>");

MODULE_DESCRIPTION("Simple LCD driver for Raspberry Pi");
