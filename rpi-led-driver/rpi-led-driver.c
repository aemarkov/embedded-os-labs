#include <linux/module.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define DEVICE_NAME     "rpi_led%d"         /* Название файла в /dev */
#define CLASS_NAME      "rpi_led_class"     /* Название класса драйвера */
#define MAX_MINOR_CNT   1                   /* Количество девайсов, создаваемых драйвером */

/* Структура для хранения данных драйвера */
struct rpi_device_data {
    struct cdev cdev;                       /* Структура, описывающая символьное устройство */
};

static dev_t rpi_devno;                     /* Номер устройства */
static struct class* rpi_class;             /* Класс драйвера. Нужен для создания устройств в /dev */
static struct rpi_device_data rpi_device;   /* Данные драйвера */

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

/* Структура с указателями функции работы с файлами, которые мы реализовали.
   По-сути, это описание того, как мы наследлвали интерфейс */
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = rpi_open,
    .release = rpi_release,
    .write = rpi_write,
};

/**
 * @brief Создает устройство
 * @param devno  номер устройства (должен быть уиникальный)
 * @param data   указатель на нашу структуру данных устройства
 * @retval       0 - успех, < 0 - ошибка
 */
static int rpi_create_dev(dev_t devno, struct rpi_device_data *data)
{
    int ret;
    struct device *device;

    pr_info("Creating device %d.%d\n", MAJOR(devno), MINOR(devno));

    /* Создаем character device, указываем для него наши file operations */
    cdev_init(&data->cdev, &fops);
    ret = cdev_add(&data->cdev, devno, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        goto err;
    }

    /* Создаем файл (inode) в /dev/ */
    device = device_create(rpi_class, NULL, devno, NULL, DEVICE_NAME, MINOR(devno));
    if (IS_ERR(device)) {
        pr_err("Failed to create device\n");
        goto del_cdev;
    }

    return 0;

del_cdev:
    cdev_del(&data->cdev);
err:
    return -1;
}

/**
 * @brief Удаляет устройство
 * @param devno  номер устройства (должен быть уиникальный)
 * @param data   указатель на нашу структуру данных устройства
 */
static void rpi_delete_dev(dev_t devno, struct rpi_device_data *data)
{
    device_destroy(rpi_class, devno);
    cdev_del(&data->cdev);
}


/* Функция вызывается при инициализации модуля */
static int __init rpi_led_init(void)
{
    int ret;

    /* Выделяем диапазон уникальных номеров для устройств */
    ret = alloc_chrdev_region(&rpi_devno, 0, MAX_MINOR_CNT, DEVICE_NAME);
    if (ret != 0) {
        pr_err("Unable to allocate chrdev region: %d\n", ret);
        goto err;
    }

    /* Регистрируем класс (нужен потом для создания устройств в /dev) */
    rpi_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(rpi_class)) {
        pr_err("Failed to create  device class\n");
        goto del_region;
    }

    /* Создаем устройство */
    ret = rpi_create_dev(rpi_devno, &rpi_device);
    if (ret < 0) {
        goto del_class;
    }

    pr_info("Driver loaded\n");
    return 0;

    /* Если что-то пошло не так, подчищаем за собой */
del_class:
   class_destroy(rpi_class);
del_region:
   unregister_chrdev_region(rpi_devno, MAX_MINOR_CNT);
err:
    return -1;
}

/* Фнкция вызывается при деинициализации модуля модуля */
static void __exit rpi_led_exit(void)
{
    rpi_delete_dev(rpi_devno, &rpi_device);
    class_destroy(rpi_class);
    unregister_chrdev_region(rpi_devno, MAX_MINOR_CNT);

    pr_info("Driver unloaded\n");
}

module_init(rpi_led_init);
module_exit(rpi_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Markov Alexey <markovalex95@gmail.com>");

MODULE_DESCRIPTION("Simple test driver for Raspberry Pi");
