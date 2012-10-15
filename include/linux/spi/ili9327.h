#ifndef __LINUX_SPI_ILI9327_H
#define __LINUX_SPI_ILI9327_H

/* platform data for the ILI9327 display */

struct ili9327_platform_data {
	int (*power_control)(int enable);
#define POWER_ENABLE	1
#define POWER_DISABLE	0
};

#endif /* __LINUX_SPI_ILI9327_H */
