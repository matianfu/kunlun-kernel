# Copyright (c) 2010 Wind River Systems, Inc.
#
# The right to copy, distribute, modify, or otherwise make use
# of this software may be licensed only pursuant to the terms
# of an applicable Wind River license agreement.

LOCAL_PATH := $(call my-dir)

KERNEL_CONFIG := $(TARGET_KERNEL_CONFIG)
kernel_MAKE_CMD := make -j 4 -C $(LOCAL_PATH) ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(ANDROID_BUILD_TOP)/$(TARGET_KERNEL_CROSSCOMPILE)

.PHONY: kernel.clean
kernel.clean:
	$(hide) echo "Cleaning kernel.."
	$(hide) $(kernel_MAKE_CMD) clean

.PHONY: kernel.distclean
kernel.distclean:
	$(hide) echo "Distcleaning kernel.."
	$(hide) $(kernel_MAKE_CMD) distclean

$(LOCAL_PATH)/.config: $(LOCAL_PATH)/arch/$(TARGET_ARCH)/configs/$(KERNEL_CONFIG)
	$(hide) echo "Configuring kernel with $(KERNEL_CONFIG).."
	$(hide) $(kernel_MAKE_CMD) $(KERNEL_CONFIG)

.PHONY: kernel.config
kernel.config: $(LOCAL_PATH)/.config

$(LOCAL_PATH)/arch/arm/boot/zImage: $(LOCAL_PATH)/.config
	$(hide) echo "Building kernel.."
	$(hide) $(kernel_MAKE_CMD) zImage

.PHONY: kernel.image
kernel.image: $(LOCAL_PATH)/arch/arm/boot/zImage

$(LOCAL_PATH)/modules.order: $(LOCAL_PATH)/.config kernel.image
	$(hide) echo "Building kernel modules.."
	$(hide) $(kernel_MAKE_CMD) modules

.PHONY: kernel.modules
kernel.modules: $(LOCAL_PATH)/modules.order

.PHONY: kernel
kernel: kernel.image kernel.modules

.PHONY: kernel.rebuild
kernel.rebuild: $(LOCAL_PATH)/.config
	$(hide) echo "Rebuilding kernel.."
	$(hide) $(kernel_MAKE_CMD) zImage
	$(hide) $(kernel_MAKE_CMD) modules

.PHONY: kernel.rebuild_all
kernel.rebuild_all: kernel.clean kernel

# WORKAROUND:
# touch the ko files if they are missing
$(LOCAL_PATH)/%.ko: kernel.modules
	$(hide) [ -f "$@" ] || touch $@
