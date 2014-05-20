//#ifndef _MEDIA_AR0130_H
//#define _MEDIA_AR0130_H
#ifndef __AR0130_H__
#define __AR0130_H__
#define AR0130_I2C_ADDR		0x10 //(0x20 >> 1)
//#define AR0130_I2C_ADDR	0x18 //(0x30 >> 1)

struct v4l2_subdev;

enum {
	AR0130_COLOR_VERSION,
	AR0130_MONOCHROME_VERSION,
};

struct ar0130_platform_data {
	int (*set_xclk)(struct v4l2_subdev *subdev, int hz);
	int (*reset)(struct v4l2_subdev *subdev, int active);
	int ext_freq; /* input frequency to the ar0130 for PLL dividers */
	int target_freq; /* frequency target for the PLL */
	int version;
	unsigned int clk_pol:1;
	void (*set_clock)(struct v4l2_subdev *subdev, unsigned int rate);
};

/*
struct AR0130_platform_data {
	unsigned int clk_pol:1;

	void (*set_clock)(struct v4l2_subdev *subdev, unsigned int rate);
};
*/

#endif
