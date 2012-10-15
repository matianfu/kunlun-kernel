#ifndef __LINUX_FT5X0X_TS_H__
#define __LINUX_FT5X0X_TS_H__

#define SCREEN_MAX_X    320
#define SCREEN_MAX_Y    480
#define PRESS_MAX       255

#define FT5X0X_I2C_NAME "ft5x0x_ts"

#define FT5X0X_I2C_ADDR (0x38)

#if defined(CONFIG_FT5X0X_GPIO_I2C)
#define FT5X0X_SCL_GPIO (12)
#define FT5X0X_SDA_GPIO (13)
#endif

#define FT5X0X_PENDOWN_GPIO	(153)
#define FT5X0X_RST_GPIO		(179)

#define FT5X0X_NAME	"ft5x0x_Touchscreen"

struct ft5x0x_ts_platform_data{
	u16	intr;		/* irq number	*/
};

enum ft5x0x_ts_regs {
	FT5X0X_REG_PMODE	= 0xA5,	/* Power Consume Mode */
};

//FT5X0X_REG_PMODE
#define PMODE_ACTIVE        0x00
#define PMODE_MONITOR       0x01
#define PMODE_STANDBY       0x02
#define PMODE_HIBERNATE     0x03

#ifndef ABS_MT_TOUCH_MAJOR
#define ABS_MT_TOUCH_MAJOR	0x30	/* touching ellipse */
#define ABS_MT_TOUCH_MINOR	0x31	/* (omit if circular) */
#define ABS_MT_WIDTH_MAJOR	0x32	/* approaching ellipse */
#define ABS_MT_WIDTH_MINOR	0x33	/* (omit if circular) */
#define ABS_MT_ORIENTATION	0x34	/* Ellipse orientation */
#define ABS_MT_POSITION_X	0x35	/* Center X ellipse position */
#define ABS_MT_POSITION_Y	0x36	/* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE	0x37	/* Type of touching device */
#define ABS_MT_BLOB_ID		0x38	/* Group set of pkts as blob */
#endif /* ABS_MT_TOUCH_MAJOR */

#endif
