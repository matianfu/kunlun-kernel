#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#

obj-$(CONFIG_DS1682)		+= ds1682.o
obj-$(CONFIG_SENSORS_PCA963X)	+= pca963x.o
obj-$(CONFIG_SENSORS_TSL2550)	+= tsl2550.o
obj-$(CONFIG_TWL4030_MADC)	+= twl4030-madc.o
obj-$(CONFIG_TWL4030_POWEROFF)  += twl4030-poweroff.o
obj-$(CONFIG_SENSORS_AKM8975) 	+= akm8975.o
obj-$(CONFIG_SENSORS_ADBS_A320) += adbs-a320.o
obj-$(CONFIG_SENSORS_MMA7455) += mma7455.o
obj-$(CONFIG_SENSORS_TMD27713T)   += taos.o
obj-$(CONFIG_SENSORS_MMC3280MS)   += mmc3280ms.o
obj-$(CONFIG_ECOMPASS)   += mecs.o

ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif
