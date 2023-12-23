#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/hid.h>
#include <linux/hiddev.h>
#include <linux/input.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artemev Ilya Olegovich");
MODULE_DESCRIPTION("Module for changing the scrolling direction of a USB mouse");

#define VENDOR_A4TECH       0x09da
#define PRODUCT_A4TECH      0xfae3

#define SETTINGS_SIZE       20
#define SETTINGS_PATH       "/media/psf/iCloud/Downloads/os-cg/src/settings.txt"

struct mouse_sc {
    struct hid_device* hdev;
    int is_natural_scrolling;
};

static int get_scrolling_value(const char* filepath)
{
    char settings[SETTINGS_SIZE];
    struct file* file;
    loff_t pos = 0;
    int bytes_read;


    file = filp_open(filepath, O_RDONLY, 0);
    if (IS_ERR(file)) {
        printk(KERN_ERR "Не удалось открыть файл\n");
        return -ENOENT;
    }

    vfs_llseek(file, pos, SEEK_SET);
    bytes_read = kernel_read(file, settings, sizeof(settings), &pos);
    filp_close(file, NULL);

    int is_natural_scrolling = -1;

    if (bytes_read > 0) {
        char* found = strstr(settings, "Natural scrolling: ");
        if (found != NULL) {
            found += strlen("Natural scrolling: ");
            is_natural_scrolling = *found - '0';
        }
    }

    if (is_natural_scrolling == 0 || is_natural_scrolling == 1) {
        printk(KERN_INFO "is_natural_scrolling значение: %d\n", is_natural_scrolling);
    }
    else {
        printk(KERN_INFO "Неверное значение: %d\n", is_natural_scrolling);
    }

    return is_natural_scrolling;
}

static int mouse_hid_input(struct hid_device* hdev, struct hid_report* report, u8* data, int size)
{
    struct mouse_sc* sc = hid_get_drvdata(hdev);

    printk(KERN_INFO "Флаг натурального скроллинга установлен в: %d\n", sc->is_natural_scrolling);

    if (sc->is_natural_scrolling == 1 && size > 0) {
        if (data[size - 1] == 0xFF) {
            printk(KERN_INFO "Скроллинг вниз изменен на скроллинг вверх\n");
            data[size - 1] = 0x01;
        }
        else if (data[size - 1] == 0x01) {
            printk(KERN_INFO "Скроллинг вверх изменен на скроллинг вниз\n");
            data[size - 1] = 0xFF;
        }
    }

    return 0;
}

static int mouse_probe(struct hid_device* hdev, const struct hid_device_id* id)
{
    int ret;
    struct mouse_sc* sc;

    sc = devm_kzalloc(&hdev->dev, sizeof(*sc), GFP_KERNEL);
    if (sc == NULL) {
        hid_err(hdev, "can’t alloc memory\n");
        return -ENOMEM;
    }

    sc->hdev = hdev;
    sc->is_natural_scrolling = get_scrolling_value(SETTINGS_PATH);

    hid_set_drvdata(hdev, sc);

    ret = hid_parse(hdev);
    if (ret)
    {
        hid_err(hdev, "parse faild\n");
        return ret;
    }

    ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
    if (ret) {
        hid_err(hdev, "hw start failed\n");
        return ret;
    }

    printk(KERN_WARNING "Connect driver\n");

    return 0;
}

static void mouse_disconnect(struct hid_device* hdev)
{
    printk(KERN_INFO "Device was disconnected\n");
    hid_hw_stop(hdev);
}

static struct hid_device_id mouse_table[] = {
    { HID_USB_DEVICE(VENDOR_A4TECH, PRODUCT_A4TECH) },
    {}
};
MODULE_DEVICE_TABLE(hid, mouse_table);

static struct hid_driver mouse_driver = {
    .name = "custom_mouse_driver",
    .id_table = mouse_table,
    .probe = mouse_probe,
    .remove = mouse_disconnect,
    .raw_event = mouse_hid_input,
};

static int __init custom_mouse_init(void)
{
    printk(KERN_INFO "Mouse module loaded\n");

    int ret = hid_register_driver(&mouse_driver);
    if (ret)
        printk(KERN_ERR "Failed to register mouse driver: %d\n", ret);

    return ret;
}

static void __exit custom_mouse_exit(void)
{
    printk(KERN_INFO "Mouse module unloaded\n");

    hid_unregister_driver(&mouse_driver);
}

module_init(custom_mouse_init);
module_exit(custom_mouse_exit);

