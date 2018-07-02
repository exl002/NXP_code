/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 8/22/2009
 Linux Version - 6/2/2009

 Copyright 2009, All Rights Reserved.

 At the discretion of the user of this library,
 this software may be licensed under the terms of the
 GNU General Public License v3, a BSD-Style license, or the
 original HIDAPI license as outlined in the LICENSE.txt,
 LICENSE-gpl3.txt, LICENSE-bsd.txt, and LICENSE-orig.txt
 files located at the root of the source distribution.
 These files may also be found in the public source
 code repository located at:
        http://github.com/signal11/hidapi .
********************************************************/

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

/* Unix */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <dirent.h>
#include <poll.h>
#include <stdbool.h>

/* Linux */
#include <linux/hidraw.h>
#include <linux/version.h>
#include <linux/input.h>

#include "hidapi.h"

#define HID_MAX_PATH 255

/* Definitions from linux/hidraw.h. Since these are new, some distros
   may not have header files which contain them. */
#ifndef HIDIOCSFEATURE
#define HIDIOCSFEATURE(len) _IOC(_IOC_WRITE | _IOC_READ, 'H', 0x06, len)
#endif
#ifndef HIDIOCGFEATURE
#define HIDIOCGFEATURE(len) _IOC(_IOC_WRITE | _IOC_READ, 'H', 0x07, len)
#endif

/* USB HID device property names */
const char *device_string_names[] = {
    "manufacturer", "product", "serial",
};

/* Symbolic names for the properties above */
enum device_string_id
{
    DEVICE_STRING_MANUFACTURER,
    DEVICE_STRING_PRODUCT,
    DEVICE_STRING_SERIAL,

    DEVICE_STRING_COUNT,
};

struct hid_device_
{
    int device_handle;
    int blocking;
    int uses_numbered_reports;
};

static __u32 kernel_version = 0;

static __u32 detect_kernel_version(void)
{
    struct utsname name;
    int major, minor, release;
    int ret;

    uname(&name);
    ret = sscanf(name.release, "%d.%d.%d", &major, &minor, &release);
    if (ret == 3)
    {
        return KERNEL_VERSION(major, minor, release);
    }

    ret = sscanf(name.release, "%d.%d", &major, &minor);
    if (ret == 2)
    {
        return KERNEL_VERSION(major, minor, 0);
    }

    printf("Couldn't determine kernel version from version string \"%s\"\n", name.release);
    return 0;
}

static hid_device *new_hid_device(void)
{
    hid_device *dev = calloc(1, sizeof(hid_device));
    dev->device_handle = -1;
    dev->blocking = 1;
    dev->uses_numbered_reports = 0;

    return dev;
}

/* The caller must free the returned string with free(). */
static wchar_t *utf8_to_wchar_t(const char *utf8)
{
    wchar_t *ret = NULL;

    if (utf8)
    {
        size_t wlen = mbstowcs(NULL, utf8, 0);
        if ((size_t)-1 == wlen)
        {
            return wcsdup(L"");
        }
        ret = calloc(wlen + 1, sizeof(wchar_t));
        mbstowcs(ret, utf8, wlen + 1);
        ret[wlen] = 0x0000;
    }

    return ret;
}


/* uses_numbered_reports() returns 1 if report_descriptor describes a device
   which contains numbered reports. */
static int uses_numbered_reports(__u8 *report_descriptor, __u32 size)
{
    unsigned int i = 0;
    int size_code;
    int data_len, key_size;

    while (i < size)
    {
        int key = report_descriptor[i];

        /* Check for the Report ID key */
        if (key == 0x85 /*Report ID*/)
        {
            /* This device has a Report ID, which means it uses
               numbered reports. */
            return 1;
        }

        // printf("key: %02hhx\n", key);

        if ((key & 0xf0) == 0xf0)
        {
            /* This is a Long Item. The next byte contains the
               length of the data section (value) for this key.
               See the HID specification, version 1.11, section
               6.2.2.3, titled "Long Items." */
            if (i + 1 < size)
                data_len = report_descriptor[i + 1];
            else
                data_len = 0; /* malformed report */
            key_size = 3;
        }
        else
        {
            /* This is a Short Item. The bottom two bits of the
               key contain the size code for the data section
               (value) for this key.  Refer to the HID
               specification, version 1.11, section 6.2.2.2,
               titled "Short Items." */
            size_code = key & 0x3;
            switch (size_code)
            {
                case 0:
                case 1:
                case 2:
                    data_len = size_code;
                    break;
                case 3:
                    data_len = 4;
                    break;
                default:
                    /* Can't ever happen since size_code is & 0x3 */
                    data_len = 0;
                    break;
            };
            key_size = 1;
        }

        /* Skip over this key and it's associated data */
        i += data_len + key_size;
    }

    /* Didn't find a Report ID key. Device doesn't use numbered reports. */
    return 0;
}

/*
 * The caller is responsible for free()ing the (newly-allocated) character
 * strings pointed to by serial_number_utf8 and product_name_utf8 after use.
 */
static int parse_uevent_info(const char *uevent,
                             int *bus_type,
                             unsigned short *vendor_id,
                             unsigned short *product_id,
                             char **serial_number_utf8,
                             char **product_name_utf8)
{
    char *tmp = strdup(uevent);
    char *saveptr = NULL;
    char *line;
    char *key;
    char *value;

    int found_id = 0;
    int found_serial = 0;
    int found_name = 0;

    line = strtok_r(tmp, "\n", &saveptr);
    while (line != NULL)
    {
        /* line: "KEY=value" */
        key = line;
        value = strchr(line, '=');
        if (!value)
        {
            goto next_line;
        }
        *value = '\0';
        value++;

        if (strcmp(key, "HID_ID") == 0)
        {
            /**
             *        type vendor   product
             * HID_ID=0003:000005AC:00008242
             **/
            int ret = sscanf(value, "%x:%hx:%hx", bus_type, vendor_id, product_id);
            if (ret == 3)
            {
                found_id = 1;
            }
        }
        else if (strcmp(key, "HID_NAME") == 0)
        {
            /* The caller has to free the product name */
            *product_name_utf8 = strdup(value);
            found_name = 1;
        }
        else if (strcmp(key, "HID_UNIQ") == 0)
        {
            /* The caller has to free the serial number */
            *serial_number_utf8 = strdup(value);
            found_serial = 1;
        }

    next_line:
        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(tmp);
    return (found_id && found_name && found_serial);
}

static int get_device_string(hid_device *dev __attribute__((unused)),
                             enum device_string_id key __attribute__((unused)),
                             wchar_t *string __attribute__((unused)),
                             size_t maxlen __attribute__((unused)))
{
    /* Todo: implement this function for android */
    return 1;
}

int HID_API_EXPORT hid_init(void)
{
    const char *locale;

    /* Set the locale if it's not set. */
    locale = setlocale(LC_CTYPE, NULL);
    if (!locale)
        setlocale(LC_CTYPE, "");

    kernel_version = detect_kernel_version();

    return 0;
}

int HID_API_EXPORT hid_exit(void)
{
    /* Nothing to do for this in the Linux/hidraw implementation. */
    return 0;
}


/*
 * Used by scandir to find raw hid device files.
 */
 static int check_hid(const struct dirent *dir)
 {
   if(!dir)
     return 0;

   if(dir->d_type == DT_CHR) {
   {
       if(strncmp(dir->d_name,"hidraw", 6) == 0)
           return 1;
       }
   }

   return 0;
 }

/*
* Search for attached raw had device files. Pass the path to hid_open_path.
*/
hid_device *hid_open(unsigned short vendor_id, unsigned short product_id,
                     const wchar_t *serial_number __attribute__((unused)))
{
    int i=0;
    int count=0;
    int res;
    struct hidraw_devinfo dev_info;
    int handle=0;
    struct dirent **namelist;
    char hid_path[HID_MAX_PATH];
    bool found_device=false;

    count = scandir("/dev/", &namelist, check_hid, alphasort);

    for(i=0; i < count; i++)
    {
        if(!found_device)
        {
            sprintf(hid_path, "/dev/%s", namelist[i]->d_name);
            handle = open(hid_path, O_RDWR);

            if (handle > 0)
            {
                memset(&dev_info, 0x0, sizeof(dev_info));
                res = ioctl(handle,HIDIOCGRAWINFO, &dev_info);
                if (res < 0)
                    perror("HIDIOCGRAWINFO");

                if(((unsigned short)dev_info.vendor == vendor_id) && ((unsigned short)dev_info.product == product_id))
                {
                    printf("HID Device (%s) vendor:0x%X product:0x%X\n", hid_path, dev_info.vendor, dev_info.product);
                    found_device=true;
                }
            }
            close (handle);
          }

          free (namelist[i]);
    }

    free(namelist);

    if(found_device)
        return hid_open_path(hid_path);
    else
       return(NULL);

}

hid_device *HID_API_EXPORT hid_open_path(const char *path)
{
    hid_device *dev = NULL;

    hid_init();

    dev = new_hid_device();

    /* OPEN HERE */
    dev->device_handle = open(path, O_RDWR);

    /* If we have a good handle, return it. */
    if (dev->device_handle > 0)
    {
        /* Get the report descriptor */
        int res, desc_size = 0;
        struct hidraw_report_descriptor rpt_desc;

        memset(&rpt_desc, 0x0, sizeof(rpt_desc));

        /* Get Report Descriptor Size */
        res = ioctl(dev->device_handle, HIDIOCGRDESCSIZE, &desc_size);
        if (res < 0)
            perror("HIDIOCGRDESCSIZE");

        /* Get Report Descriptor */
        rpt_desc.size = desc_size;
        res = ioctl(dev->device_handle, HIDIOCGRDESC, &rpt_desc);
        if (res < 0)
        {
            perror("HIDIOCGRDESC");
        }
        else
        {
            /* Determine if this device uses numbered reports. */
            dev->uses_numbered_reports = uses_numbered_reports(rpt_desc.value, rpt_desc.size);
        }

        return dev;
    }
    else
    {
        /* Unable to open any devices. */
        free(dev);
        return NULL;
    }
}

int HID_API_EXPORT hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
    int bytes_written;

    bytes_written = write(dev->device_handle, data, length);

    return bytes_written;
}

int HID_API_EXPORT hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds)
{
    int bytes_read;

    if (milliseconds >= 0)
    {
        /* Milliseconds is either 0 (non-blocking) or > 0 (contains
           a valid timeout). In both cases we want to call poll()
           and wait for data to arrive.  Don't rely on non-blocking
           operation (O_NONBLOCK) since some kernels don't seem to
           properly report device disconnection through read() when
           in non-blocking mode.  */
        int ret;
        struct pollfd fds;

        fds.fd = dev->device_handle;
        fds.events = POLLIN;
        fds.revents = 0;
        ret = poll(&fds, 1, milliseconds);
        if (ret == -1 || ret == 0)
        {
            /* Error or timeout */
            return ret;
        }
        else
        {
            /* Check for errors on the file descriptor. This will
               indicate a device disconnection. */
            if (fds.revents & (POLLERR | POLLHUP | POLLNVAL))
                return -1;
        }
    }

    bytes_read = read(dev->device_handle, data, length);
    if (bytes_read < 0 && (errno == EAGAIN || errno == EINPROGRESS))
        bytes_read = 0;

    if (bytes_read >= 0 && kernel_version != 0 && kernel_version < KERNEL_VERSION(2, 6, 34) &&
        dev->uses_numbered_reports)
    {
        /* Work around a kernel bug. Chop off the first byte. */
        memmove(data, data + 1, bytes_read);
        bytes_read--;
    }

    return bytes_read;
}

int HID_API_EXPORT hid_read(hid_device *dev, unsigned char *data, size_t length)
{
    return hid_read_timeout(dev, data, length, (dev->blocking) ? -1 : 0);
}

int HID_API_EXPORT hid_set_nonblocking(hid_device *dev, int nonblock)
{
    /* Do all non-blocking in userspace using poll(), since it looks
       like there's a bug in the kernel in some versions where
       read() will not return -1 on disconnection of the USB device */

    dev->blocking = !nonblock;
    return 0; /* Success */
}

int HID_API_EXPORT hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
    int res;

    res = ioctl(dev->device_handle, HIDIOCSFEATURE(length), data);
    if (res < 0)
        perror("ioctl (SFEATURE)");

    return res;
}

int HID_API_EXPORT hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
    int res;

    res = ioctl(dev->device_handle, HIDIOCGFEATURE(length), data);
    if (res < 0)
        perror("ioctl (GFEATURE)");

    return res;
}

void HID_API_EXPORT hid_close(hid_device *dev)
{
    if (!dev)
        return;
    close(dev->device_handle);
    free(dev);
}

int HID_API_EXPORT_CALL hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
    return get_device_string(dev, DEVICE_STRING_MANUFACTURER, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
    return get_device_string(dev, DEVICE_STRING_PRODUCT, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
    return get_device_string(dev, DEVICE_STRING_SERIAL, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_indexed_string(hid_device *dev __attribute__((unused)),
                                               int string_index __attribute__((unused)),
                                               wchar_t *string __attribute__((unused)),
					       size_t maxlen __attribute__((unused)))
{
    return -1;
}

HID_API_EXPORT const wchar_t *HID_API_CALL hid_error(hid_device *dev __attribute__((unused)))
{
    return NULL;
}
