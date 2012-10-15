#ifndef __LINUX_I2C_MMA7455_H
#define __LINUX_I2C_MMA7455_H

/* platform data for the MMA7455 sensor */

struct mma7455_platform_data {
	int (*power_control)(int enable);
#define POWER_ENABLE	1
#define POWER_DISABLE	0
};

#endif /* __LINUX_I2C_MMA7455_H */
