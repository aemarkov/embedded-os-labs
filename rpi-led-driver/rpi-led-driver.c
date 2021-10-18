#include "linux/export.h"
#include <linux/module.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

#define DEVICE_NAME     "rpi_led"         /* Название файла в /dev */

static int rpi_open(struct inode *inode, struct file *file);
static int rpi_release(struct inode *inode, struct file *file);
static ssize_t rpi_write(struct file *file, const char __user *buffer,
    size_t count, loff_t *offp);

static int __init rpi_led_probe(struct platform_device *pdev);
static int __exit rpi_led_remove(struct platform_device *pdev);

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
     {.compatible = "rpi-tarsov-expansion"},
     {}
};
MODULE_DEVICE_TABLE(of, supported_devices);

/* Структура, описывающая platform device driver */
static struct platform_driver rpi_led_platform_driver = {
    .probe = rpi_led_probe,                     /* Функция, которая вызывается при загрузке */
    .remove = rpi_led_remove,                   /* Функция, которая вызывается при удалении */
    .driver = {
        .name = DEVICE_NAME,                    /* Фиг его знает... */
        .of_match_table = supported_devices,    /* Список поддерживаемых устройств */
        .owner = THIS_MODULE
    }
};

/* Дескрипторы GPIO-пинов, которые мы получили из Device Tree и которые используется
   для управления пинами
*/
struct gpio_descs *gpios;

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

/* Выключает все пины */
void rpi_gpio_disable(void)
{
    unsigned long values;

    if (gpios == NULL) {
        pr_err("gpios isn't initialized\n");
        return;
    }

    /* Записываем битмаску (все нули) в пины */
    values = 0;
    gpiod_set_array_value(gpios->ndescs, gpios->desc, gpios->info, &values);
}

/* Включает пин с заданным индексом, выключает остальные */
void rpi_gpio_set(unsigned index)
{
    unsigned long values;

    if (gpios == NULL) {
        pr_err("gpios isn't initialized\n");
        return;
    }

    if (index >= gpios->ndescs) {
        pr_err("GPIO index %u is out of range\n", index);
        return;
    }

    /* Записываем битмаску (все нули) в пины, чтобы выключить уже включенные*/
    values = 0;
    gpiod_set_array_value(gpios->ndescs, gpios->desc, gpios->info, &values);

    /* Записываем битмаску, в которой установлен только бит с заданным индексом*/
    values = (1 << index);
    gpiod_set_array_value(gpios->ndescs, gpios->desc, gpios->info, &values);

}
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
    int ret;
    u8 led;

    pr_info("write count=%zu, offp=%lld\n", count, *offp);

    /* Для упрощения, не поддерживаем смещение, т.е. несколько последовательных
       записей и всякие lseek */
    if (*offp != 0) {
        pr_err("This driver doesn't support offset\n");
        return -EINVAL;
    }

    /* Парсим строку в число */
    ret = kstrtou8_from_user(buffer, count, 10, &led);
    if (ret != 0) {
        pr_err("Invalid data\n");
        return -EINVAL;
    }

    pr_info("Led: %u\n", led);
    rpi_gpio_set(led);

    return count;
}

/* Инициализация драйвера */
static int rpi_driver_init(struct platform_device *pdev)
{
    int ret;

    gpios= gpiod_get_array(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (gpios == NULL || IS_ERR(gpios)) {
        pr_err("Failed to get GPIO");
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
    gpiod_put_array(gpios);
err:
    pr_err("Failed to initialize driver\n");
    return -1;
}

/* Деинициализация драйвера */
static void rpi_driver_deinit(void)
{
    rpi_gpio_disable();
    gpiod_put_array(gpios);
    misc_deregister(&rpi_miscdevice);
    pr_info("Driver deinitialized\n");
}

/* Функция вызывается, когда подходящее устройство обнаружено */
static int __init rpi_led_probe(struct platform_device *pdev)
{
    pr_info("Driver probed\n");
    return rpi_driver_init(pdev);
}

/* Функция вызывается, когда устройство удалено, либо драйвер выгружен */
static int __exit rpi_led_remove(struct platform_device *pdev)
{
    pr_info("Driver is removing\n");
    rpi_driver_deinit();
    return 0;
}

/* Регистрация platform device driver */
module_platform_driver(rpi_led_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Markov Alexey <markovalex95@gmail.com>");

MODULE_DESCRIPTION("Simple test driver for Raspberry Pi");
