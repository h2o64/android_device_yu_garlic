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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include "log.h"
#include "property_service.h"
#include "util.h"
#include "vendor_init.h"

#define STRCONV_(x)      #x
#define STRCONV(x) "%" STRCONV_(x) "s"
#define BUF_SIZE 64
#define DRV_INFO "/sys/devices/platform/fp_drv/fp_drv_info"

static int read_file2(const char *fname, char *data, int max_size)
{
    int fd, rc;

    if (max_size < 1)
        return 0;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        ERROR("failed to open '%s'\n", fname);
        return 0;
    }

    rc = read(fd, data, max_size -1);
    if ((rc > 0) && (rc < max_size ))
        data[rc] = '\0';
    else
        data[0] = '\0';
    close(fd);

    return 1;
}

static void fp_prop()
{
    int rv;
    char buf[BUF_SIZE];
    char vend[BUF_SIZE];

    if (read_file2(DRV_INFO, buf, sizeof(buf)))
        sscanf(buf, STRCONV(BUF_SIZE), vend);
    else
        ERROR("%s: Failed to open %c\n", __func__, DRV_INFO);

    if (!strcmp(vend, "elan_fp")) {
        property_set("ro.hardware.fingerprint", "elan");
        property_set("persist.sys.fp.goodix", "0");
    } else if (!strcmp(vend, "goodix_fp")) {
        property_set("ro.hardware.fingerprint", "goodix");
        property_set("persist.sys.fp.goodix", "1");
    } else if (!strcmp(vend, "silead_fp_dev")) {
        ERROR("%s: Silead fpsvcd fingerprint sensor is unsupported", __func__);
    } else {
        ERROR("%s: Fingerprint sensor is unsupported", __func__);
    }
}

void vendor_load_properties()
{
    fp_prop();
}

