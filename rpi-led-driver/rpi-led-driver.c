#include <linux/module.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME     "rpi_led"         /* Название файла в /dev */

static int rpi_open(struct inode *inode, struct file *file);
static int rpi_release(struct inode *inode, struct file *file);
static ssize_t rpi_write(struct file *file, const char __user *buffer,
    size_t count, loff_t *offp);


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

    return count;
}

/* Функция вызывается при инициализации модуля */
static int __init rpi_led_init(void)
{
    int ret;

    ret = misc_register(&rpi_miscdevice);
    if (ret != 0) {
        pr_err("Failed to register misc device\n");
        goto err;
    }

    pr_info("Driver loaded\n");
    return 0;
err:
    return -1;
}

/* Фнкция вызывается при деинициализации модуля модуля */
static void __exit rpi_led_exit(void)
{
    misc_deregister(&rpi_miscdevice);
    pr_info("Driver unloaded\n");
}

module_init(rpi_led_init);
module_exit(rpi_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Markov Alexey <markovalex95@gmail.com>");

MODULE_DESCRIPTION("Simple test driver for Raspberry Pi");
