/*
 * Copyright (C) 2017 YU Community OS Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "FingerprintWrapper"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <hardware/fingerprint.h>
#include <utils/threads.h>

#define DRV_INFO "/sys/devices/platform/fp_drv/fp_drv_info"

typedef struct {
    fingerprint_device_t base;
    union {
        fingerprint_device_t *device;
        hw_device_t *hw_device;
    } vendor;
} device_t;

static android::Mutex vendor_mutex;

static union {
    const fingerprint_module_t *module;
    const hw_module_t *hw_module;
} vendor;

static bool ensure_vendor_module_is_loaded(void)
{
    int rv;
    FILE *fp;
    char *vend;

    fp = fopen(DRV_INFO, "r");
    fscanf(fp, "%c", vend);
    fclose(fp);

    android::Mutex::Autolock lock(vendor_mutex);

    if (!vendor.module) {
        if (strcmp(vend, "elan_fp")) {
            property_set("persist.sys.fp.goodix", "0");
            rv = hw_get_module_by_class("fingerprint", "elan", &vendor.hw_module);
        } else if (strcmp(vend, "goodix_fp")) {
            property_set("persist.sys.fp.goodix", "1");
            rv = hw_get_module_by_class("fingerprint", "goodix", &vendor.hw_module);
        } else if (strcmp(vend, "silead_fp_dev")) {
            ALOGE("Silead fpsvcd fingerprint sensor is unsupported");
            vendor.module = NULL;
        } else {
            if (rv) {
                ALOGE("failed to open vendor module, error %d", rv);
                vendor.module = NULL;
            } else {
                ALOGI("loaded vendor module: %s version %x", vendor.module->common.name,
                vendor.module->common.module_api_version);
            }
        }
    }

    return vendor.module != NULL;
}

static int set_notify(struct fingerprint_device *dev, fingerprint_notify_t notify)
{
    device_t *device = (device_t *) dev;

    device->base.notify = notify;
    return device->vendor.device->set_notify(device->vendor.device, notify);
}

static uint64_t pre_enroll(struct fingerprint_device *dev)
{
    device_t *device = (device_t *) dev;

    return device->vendor.device->pre_enroll(device->vendor.device);
}

static int enroll(struct fingerprint_device *dev, const hw_auth_token_t *hat, uint32_t gid,
                uint32_t timeout_sec)
{
    device_t *device = (device_t *) dev;

    return device->vendor.device->enroll(device->vendor.device, hat, gid, timeout_sec);
}

static int post_enroll(struct fingerprint_device *dev)
{
    device_t *device = (device_t *) dev;

    return device->vendor.device->post_enroll(device->vendor.device);
}

static uint64_t get_authenticator_id(struct fingerprint_device *dev)
{
    device_t *device = (device_t *) dev;

    return device->vendor.device->get_authenticator_id(device->vendor.device);
}

static int cancel(struct fingerprint_device *dev)
{
    device_t *device = (device_t *) dev;

    return device->vendor.device->cancel(device->vendor.device);
}

#define MAX_FINGERPRINTS 100

typedef int (*enumerate_2_0)(struct fingerprint_device *dev, fingerprint_finger_id_t *results,
        uint32_t *max_size);

static int enumerate_pre_2_1(struct fingerprint_device *dev)
{
    device_t *device = (device_t *) dev;
    fingerprint_finger_id_t results[MAX_FINGERPRINTS];
    uint32_t n = MAX_FINGERPRINTS;
    enumerate_2_0 enumerate = (enumerate_2_0) device->vendor.device->enumerate;
    int rv = enumerate(device->vendor.device, results, &n);

    if (rv == 0) {
        uint32_t i;
        fingerprint_msg_t msg;

        msg.type = FINGERPRINT_TEMPLATE_ENUMERATING;
        for (i = 0; i < n; i++) {
            msg.data.enumerated.finger = results[i];
            msg.data.enumerated.remaining_templates = n - i - 1;
            device->base.notify(&msg);
        }
    }

    return rv;
}

static int enumerate(struct fingerprint_device *dev)
{
    device_t *device = (device_t *) dev;

    return device->vendor.device->enumerate(device->vendor.device);
}

static int remove(struct fingerprint_device *dev, uint32_t gid, uint32_t fid)
{
    device_t *device = (device_t *) dev;

    return device->vendor.device->remove(device->vendor.device, gid, fid);
}

static int set_active_group(struct fingerprint_device *dev, uint32_t gid, const char *store_path)
{
    device_t *device = (device_t *) dev;

    return device->vendor.device->set_active_group(device->vendor.device, gid, store_path);
}

static int authenticate(struct fingerprint_device *dev, uint64_t operation_id, uint32_t gid)
{
    device_t *device = (device_t *) dev;

    return device->vendor.device->authenticate(device->vendor.device, operation_id, gid);
}

static int device_close(hw_device_t *hw_device)
{
    device_t *device = (device_t *) hw_device;
    int rv = device->base.common.close(device->vendor.hw_device);
    free(device);
    return rv;
}

static int device_open(const hw_module_t *module, const char *name, hw_device_t **device_out)
{
    int rv;
    device_t *device;

    if (!ensure_vendor_module_is_loaded()) {
        return -EINVAL;
    }

    device = (device_t *) calloc(sizeof(*device), 1);
    if (!device) {
        ALOGE("%s: Failed to allocate memory", __func__);
        return -ENOMEM;
    }

    rv = vendor.module->common.methods->open(vendor.hw_module, name, &device->vendor.hw_device);
    if (rv) {
        ALOGE("%s: failed to open, error %d\n", __func__, rv);
        free(device);
        return rv;
    }

    device->base.common.tag = HARDWARE_DEVICE_TAG;
    device->base.common.version = device->vendor.device->common.version;
    device->base.common.module = (hw_module_t *) module;
    device->base.common.close = device_close;

    device->base.set_notify = set_notify;
    device->base.pre_enroll = pre_enroll;
    device->base.enroll = enroll;
    device->base.post_enroll = post_enroll;
    device->base.get_authenticator_id = get_authenticator_id;
    device->base.cancel = cancel;
    if (vendor.module->common.module_api_version >= FINGERPRINT_MODULE_API_VERSION_2_1) {
        device->base.enumerate = enumerate;
    } else {
        device->base.enumerate = enumerate_pre_2_1;
    }
    device->base.remove = remove;
    device->base.set_active_group = set_active_group;
    device->base.authenticate = authenticate;

    *device_out = (hw_device_t *) device;
    return 0;
}

static struct hw_module_methods_t module_methods = {
    .open = device_open
};

fingerprint_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = FINGERPRINT_MODULE_API_VERSION_2_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = FINGERPRINT_HARDWARE_MODULE_ID,
        .name = "Lineage Fingerprint Wrapper",
        .author = "The LineageOS Project",
        .methods = &module_methods,
        .dso = NULL, /* remove compilation warnings */
        .reserved = {0}, /* remove compilation warnings */
    },
};
