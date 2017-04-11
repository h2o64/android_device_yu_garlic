#
# Copyright (C) 2017 YU Community OS Team
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

$(call inherit-product, device/yu/garlic/full_garlic.mk)

# Inherit some common LineageOS stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

PRODUCT_NAME := lineage_garlic
BOARD_VENDOR := yu

PRODUCT_GMS_CLIENTID_BASE := android-micromax

TARGET_VENDOR_PRODUCT_NAME := YUREKA_BLACK
TARGET_VENDOR_DEVICE_NAME := YUREKA_BLACK
PRODUCT_BUILD_PROP_OVERRIDES += TARGET_DEVICE=YUREKA_BLACK PRODUCT_NAME=YUREKA_BLACK

## Use the latest approved GMS identifiers unless running a signed build
ifneq ($(SIGN_BUILD),true)
PRODUCT_BUILD_PROP_OVERRIDES += \
    BUILD_FINGERPRINT=YU/YU5040/YU5040:6.0.1/MMB29M/1483083874:user/release-keys \
    PRIVATE_BUILD_DESC="p7201-user 6.0.1 MMB29M 1483083874 release-keys"
endif
