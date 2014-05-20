#include <linux/gpio.h>
#include <linux/regulator/machine.h>

#include <plat/i2c.h>
#ifdef CONFIG_VIDEO_MT9P031
#include <media/mt9p031.h>
#endif
#ifdef CONFIG_VIDEO_MT9D131
#include <media/mt9d131.h>
#endif
#ifdef CONFIG_VIDEO_MT9P006
#include <media/mt9p006.h>
#endif
#ifdef CONFIG_VIDEO_MT9P017
#include <media/mt9p017.h>
#endif
#ifdef CONFIG_VIDEO_MT9V128
#include <media/mt9v128.h>
#endif
#ifdef CONFIG_VIDEO_MT9V034
#include <media/mt9v034.h>
#endif
#ifdef CONFIG_VIDEO_MT9T111
#include <media/mt9t111.h>
#endif
#ifdef CONFIG_VIDEO_MT9V032
#include <media/mt9v032.h>
#endif
#ifdef CONFIG_VIDEO_AR0130
#include <media/ar0130.h>
#endif
#include <asm/mach-types.h>
#include "devices.h"
#include "../../../drivers/media/video/omap3isp/isp.h"


static struct regulator *reg_1v8, *reg_2v8;

#ifdef CONFIG_VIDEO_MT9P031
#define MT9P031_RESET_GPIO	98
#define MT9P031_XCLK		ISP_XCLK_A
#define MT9P031_EXT_FREQ	21000000

static int beagle_cam_set_xclk(struct v4l2_subdev *subdev, int hz)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	return isp->platform_cb.set_xclk(isp, hz, MT9P031_XCLK);
}

static int beagle_cam_reset(struct v4l2_subdev *subdev, int active)
{
	/* Set RESET_BAR to !active */
	gpio_set_value(MT9P031_RESET_GPIO, !active);

	return 0;
}

static struct mt9p031_platform_data beagle_mt9p031_platform_data = {
	.set_xclk	= beagle_cam_set_xclk,
	.reset		= beagle_cam_reset,
	.ext_freq	= MT9P031_EXT_FREQ,
	.target_freq	= 48000000,
	.version	= MT9P031_COLOR_VERSION,
//	.version	= MT9P031_MONOCHROME_VERSION,
};

static struct i2c_board_info mt9p031_camera_i2c_device = {
	I2C_BOARD_INFO("mt9p031", 0x48),
	.platform_data = &beagle_mt9p031_platform_data,
};

static struct isp_subdev_i2c_board_info mt9p031_camera_subdevs[] = {
	{
		.board_info = &mt9p031_camera_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group beagle_camera_subdevs[] = {
	{
		.subdevs = mt9p031_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift = 0,
				.clk_pol = 1,
				.bridge = ISPCTRL_PAR_BRIDGE_DISABLE,
			}
		},
	},
	{ },
};
#endif //mt9p031

// MT9D131 Support
#ifdef CONFIG_VIDEO_MT9D131
#define MT9D131_RESET_GPIO	98
#define MT9D131_XCLK		ISP_XCLK_A
#define MT9D131_EXT_FREQ	24000000

static int beagle_cam_set_xclk(struct v4l2_subdev *subdev, int hz)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	return isp->platform_cb.set_xclk(isp, hz, MT9D131_XCLK);
}

static int beagle_cam_reset(struct v4l2_subdev *subdev, int active)
{
	/* Set RESET_BAR to !active */
	gpio_set_value(MT9D131_RESET_GPIO, !active);

	return 0;
}

static struct mt9d131_platform_data beagle_mt9d131_platform_data = {
	.set_xclk	= beagle_cam_set_xclk,
	.reset		= beagle_cam_reset,
	.ext_freq	= MT9D131_EXT_FREQ,
	.target_freq	= 48000000,
	.version	= MT9D131_COLOR_VERSION,
};

static struct i2c_board_info mt9d131_camera_i2c_device = {
	I2C_BOARD_INFO("mt9d131", MT9D131_I2C_ADDR),
	.platform_data = &beagle_mt9d131_platform_data,
};

static struct isp_subdev_i2c_board_info mt9d131_camera_subdevs[] = {
	{
		.board_info = &mt9d131_camera_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group beagle_camera_subdevs[] = {
	{
		.subdevs = mt9d131_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift = 0x2,
				.clk_pol = 0,
				.bridge = ISPCTRL_PAR_BRIDGE_BENDIAN,
				//.bridge = ISPCTRL_PAR_BRIDGE_DISABLE,
			}
		},
	},
	{ },
};
#endif //mt9d131

// MT9P006 Support
#ifdef CONFIG_VIDEO_MT9P006
#define MT9P006_RESET_GPIO	98
#define MT9P006_XCLK		ISP_XCLK_A
#define MT9P006_EXT_FREQ	24000000

static int beagle_cam_set_xclk(struct v4l2_subdev *subdev, int hz)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	return isp->platform_cb.set_xclk(isp, hz, MT9P006_XCLK);
}

static int beagle_cam_reset(struct v4l2_subdev *subdev, int active)
{
	/* Set RESET_BAR to !active */
	gpio_set_value(MT9P006_RESET_GPIO, !active);

	return 0;
}

static struct mt9p006_platform_data beagle_mt9p006_platform_data = {
	.set_xclk	= beagle_cam_set_xclk,
	.reset		= beagle_cam_reset,
	.ext_freq	= MT9P006_EXT_FREQ,
	.target_freq	= 48000000,
	//.version	= MT9P006_COLOR_VERSION,
};

static struct i2c_board_info mt9p006_camera_i2c_device = {
	I2C_BOARD_INFO("mt9p006", MT9P006_I2C_ADDR),
	.platform_data = &beagle_mt9p006_platform_data,
};

static struct isp_subdev_i2c_board_info mt9p006_camera_subdevs[] = {
	{
		.board_info = &mt9p006_camera_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group beagle_camera_subdevs[] = {
	{
		.subdevs = mt9p006_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift = 1,
				.clk_pol = 1,
				.bridge = ISPCTRL_PAR_BRIDGE_DISABLE,
				//.bridge = ISPCTRL_PAR_BRIDGE_DISABLE,
			}
		},
	},
	{ },
};
#endif //mt9p006

// MT9P017 Support
#ifdef CONFIG_VIDEO_MT9P017
#define MT9P017_RESET_GPIO	98
#define MT9P017_XCLK		ISP_XCLK_A
#define MT9P017_EXT_FREQ	24000000


static int beagle_cam_set_xclk(struct v4l2_subdev *subdev, int hz)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	return isp->platform_cb.set_xclk(isp, hz, MT9P017_XCLK);
}

static int beagle_cam_reset(struct v4l2_subdev *subdev, int active)
{
	/* Set RESET_BAR to !active */
	gpio_set_value(MT9P017_RESET_GPIO, !active);

	return 0;
}

static struct mt9p017_platform_data beagle_mt9p017_platform_data = {
	.set_xclk	= beagle_cam_set_xclk,
	.reset		= beagle_cam_reset,
	.ext_freq	= MT9P017_EXT_FREQ,
	.target_freq	= 48000000,
};

static struct i2c_board_info mt9p017_camera_i2c_device = {
	I2C_BOARD_INFO("mt9p017", MT9P017_I2C_ADDR),
	.platform_data = &beagle_mt9p017_platform_data,
};

static struct isp_subdev_i2c_board_info mt9p017_camera_subdevs[] = {
	{
		.board_info = &mt9p017_camera_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group beagle_camera_subdevs[] = {
	{
		.subdevs = mt9p017_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift = 0x1,
				.clk_pol = 0,
				.bridge = ISPCTRL_PAR_BRIDGE_DISABLE,
				}
		},
	},
	{ },
};
#endif //mt9p017

// MT9V128 Support
#ifdef CONFIG_VIDEO_MT9V128
#define MT9V128_RESET_GPIO	98
#define MT9V128_XCLK		ISP_XCLK_A
#define MT9V128_EXT_FREQ	27000000


static int beagle_cam_set_xclk(struct v4l2_subdev *subdev, int hz)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	return isp->platform_cb.set_xclk(isp, hz, MT9V128_XCLK);
}

static int beagle_cam_reset(struct v4l2_subdev *subdev, int active)
{
	/* Set RESET_BAR to !active */
	gpio_set_value(MT9V128_RESET_GPIO, !active);

	return 0;
}

static struct mt9v128_platform_data beagle_mt9v128_platform_data = {
	.set_xclk	= beagle_cam_set_xclk,
	.reset		= beagle_cam_reset,
	.ext_freq	= MT9V128_EXT_FREQ,
	.target_freq	= 54000000,
	.version        = MT9V128_COLOR_VERSION,
};

static struct i2c_board_info mt9v128_camera_i2c_device = {
	I2C_BOARD_INFO("mt9v128", MT9V128_I2C_ADDR),
	.platform_data = &beagle_mt9v128_platform_data,
};

static struct isp_subdev_i2c_board_info mt9v128_camera_subdevs[] = {
	{
		.board_info = &mt9v128_camera_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group beagle_camera_subdevs[] = {
	{
		.subdevs = mt9v128_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift = 1,
				.clk_pol = 0,
			//	.bridge = ISPCTRL_PAR_BRIDGE_BENDIAN,
				.bridge = ISPCTRL_PAR_BRIDGE_DISABLE,
				}
		},
	},
	{ },
};
#endif //mt9v128

// MT9V034 Support
#ifdef CONFIG_VIDEO_MT9V034
#define MT9V034_RESET_GPIO	98
#define MT9V034_XCLK		ISP_XCLK_A
#define MT9V034_EXT_FREQ	27000000
/* 
   sysclk = clk_27 (on board)
   SW4: connect pin.3 to pin.1 
*/

static int beagle_cam_set_xclk(struct v4l2_subdev *subdev, int hz)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	return isp->platform_cb.set_xclk(isp, hz, MT9V034_XCLK);
}

static int beagle_cam_reset(struct v4l2_subdev *subdev, int active)
{
	/* Set RESET_BAR to !active */
	gpio_set_value(MT9V034_RESET_GPIO, !active);

	return 0;
}

static struct mt9v034_platform_data beagle_mt9v034_platform_data = {
	.set_xclk	= beagle_cam_set_xclk,
	.reset		= beagle_cam_reset,
	.ext_freq	= MT9V034_EXT_FREQ,
	.target_freq	= 48000000,
	.version        = MT9V034_COLOR_VERSION,
};

static struct i2c_board_info mt9v034_camera_i2c_device = {
	I2C_BOARD_INFO("mt9v034", MT9V034_I2C_ADDR),
	.platform_data = &beagle_mt9v034_platform_data,
};

static struct isp_subdev_i2c_board_info mt9v034_camera_subdevs[] = {
	{
		.board_info = &mt9v034_camera_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group beagle_camera_subdevs[] = {
	{
		.subdevs = mt9v034_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift = 1,
				.clk_pol = 0,
				.bridge = ISPCTRL_PAR_BRIDGE_DISABLE,
				}
		},
	},
	{ },
};
#endif //mt9v034

// MT9T111 Support
#ifdef CONFIG_VIDEO_MT9T111
#define MT9T111_RESET_GPIO	98
#define MT9T111_XCLK		ISP_XCLK_A
#define MT9T111_EXT_FREQ	24000000

static int beagle_cam_set_xclk(struct v4l2_subdev *subdev, int hz)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	return isp->platform_cb.set_xclk(isp, hz, MT9T111_XCLK);
}

static int beagle_cam_reset(struct v4l2_subdev *subdev, int active)
{
	/* Set RESET_BAR to !active */
	gpio_set_value(MT9T111_RESET_GPIO, !active);

	return 0;
}

static struct mt9t111_platform_data beagle_mt9t111_platform_data = {
	.set_xclk	= beagle_cam_set_xclk,
	.reset		= beagle_cam_reset,
	.ext_freq	= MT9T111_EXT_FREQ,
	.target_freq	= 24000000,
};

static struct i2c_board_info mt9t111_camera_i2c_device = {
	I2C_BOARD_INFO("mt9t111", MT9T111_I2C_ADDR),
	.platform_data = &beagle_mt9t111_platform_data,
};

static struct isp_subdev_i2c_board_info mt9t111_camera_subdevs[] = {
	{
		.board_info = &mt9t111_camera_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group beagle_camera_subdevs[] = {
	{
		.subdevs = mt9t111_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift = 0x2,
				.clk_pol = 1,
				.bridge = ISPCTRL_PAR_BRIDGE_BENDIAN,
				}
		},
	},
	{ },
};
#endif //mt9t111

// MT9V032 Support
#ifdef CONFIG_VIDEO_MT9V032
#define MT9V032_RESET_GPIO	98
#define MT9V032_XCLK		ISP_XCLK_A
#define MT9V032_EXT_FREQ	27000000
/* 
   sysclk = clk_27 (on board)
   SW4: connect pin.3 to pin.1 
*/

static int beagle_cam_set_xclk(struct v4l2_subdev *subdev, int hz)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	return isp->platform_cb.set_xclk(isp, hz, MT9V032_XCLK);
}

static int beagle_cam_reset(struct v4l2_subdev *subdev, int active)
{
	/* Set RESET_BAR to !active */
	gpio_set_value(MT9V032_RESET_GPIO, !active);

	return 0;
}

static struct mt9v032_platform_data beagle_mt9v032_platform_data = {
	.set_xclk	= beagle_cam_set_xclk,
	.reset		= beagle_cam_reset,
	.ext_freq	= MT9V032_EXT_FREQ,
	.target_freq	= 48000000,
	.version        = MT9V032_COLOR_VERSION,
};

static struct i2c_board_info mt9v032_camera_i2c_device = {
	I2C_BOARD_INFO("mt9v032", MT9V032_I2C_ADDR),
	.platform_data = &beagle_mt9v032_platform_data,
};

static struct isp_subdev_i2c_board_info mt9v032_camera_subdevs[] = {
	{
		.board_info = &mt9v032_camera_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group beagle_camera_subdevs[] = {
	{
		.subdevs = mt9v032_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift = 1,
				.clk_pol = 0,
				.bridge = ISPCTRL_PAR_BRIDGE_DISABLE,
				}
		},
	},
	{ },
};
#endif //mt9v032

//AR0130 Support
#ifdef CONFIG_VIDEO_AR0130
#define AR0130_RESET_GPIO      98
#define AR0130_XCLK            ISP_XCLK_A
#define AR0130_EXT_FREQ        27000000

static int beagle_cam_set_xclk(struct v4l2_subdev *subdev, int hz)
{
        struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

        return isp->platform_cb.set_xclk(isp, hz, AR0130_XCLK);
}

static int beagle_cam_reset(struct v4l2_subdev *subdev, int active)
{
        /* Set RESET_BAR to !active */
        gpio_set_value(AR0130_RESET_GPIO, !active);

        return 0;
}

static struct ar0130_platform_data beagle_ar0130_platform_data = {
        .set_xclk       = beagle_cam_set_xclk,
        .reset          = beagle_cam_reset,
        .ext_freq       = AR0130_EXT_FREQ,
        .target_freq    = 48000000,
        .version        = AR0130_COLOR_VERSION,
};

static struct i2c_board_info ar0130_camera_i2c_device = {
        I2C_BOARD_INFO("ar0130", AR0130_I2C_ADDR),
        .platform_data = &beagle_ar0130_platform_data,
};

static struct isp_subdev_i2c_board_info ar0130_camera_subdevs[] = {
        {
                .board_info = &ar0130_camera_i2c_device,
                .i2c_adapter_id = 2,
        },
        { NULL, 0, },
};

static struct isp_v4l2_subdevs_group beagle_camera_subdevs[] = {
        {
                .subdevs = ar0130_camera_subdevs,
                .interface = ISP_INTERFACE_PARALLEL,
                .bus = {
                        .parallel = {
                                .data_lane_shift = 0,
                                .clk_pol = 0,
                                .bridge = ISPCTRL_PAR_BRIDGE_DISABLE,
                                }
                },
        },
        { },
};
#endif //ar0130

static struct isp_platform_data beagle_isp_platform_data = {
	.subdevs = beagle_camera_subdevs,
};

static int __init beagle_camera_init(void)
{
	if (!machine_is_omap3_beagle() || !cpu_is_omap3630())
		return 0;

	reg_1v8 = regulator_get(NULL, "cam_1v8");
	if (IS_ERR(reg_1v8))
		pr_err("%s: cannot get cam_1v8 regulator\n", __func__);
	else
		regulator_enable(reg_1v8);

	reg_2v8 = regulator_get(NULL, "cam_2v8");
	if (IS_ERR(reg_2v8))
		pr_err("%s: cannot get cam_2v8 regulator\n", __func__);
	else
		regulator_enable(reg_2v8);

	omap_register_i2c_bus(2, 100, NULL, 0);
#ifdef CONFIG_VIDEO_MT9P031
	gpio_request(MT9P031_RESET_GPIO, "cam_rst");
	gpio_direction_output(MT9P031_RESET_GPIO, 0);
#endif
#ifdef CONFIG_VIDEO_MT9D131
	gpio_request(MT9D131_RESET_GPIO, "cam_rst");
	gpio_direction_output(MT9D131_RESET_GPIO, 0);
#endif
#ifdef CONFIG_VIDEO_MT9P006
	gpio_request(MT9P006_RESET_GPIO, "cam_rst");
	gpio_direction_output(MT9P006_RESET_GPIO, 0);
#endif
#ifdef CONFIG_VIDEO_MT9P017
	gpio_request(MT9P017_RESET_GPIO, "cam_rst");
	gpio_direction_output(MT9P017_RESET_GPIO, 0);
#endif
#ifdef CONFIG_VIDEO_MT9V128
	gpio_request(MT9V128_RESET_GPIO, "cam_rst");
	gpio_direction_output(MT9V128_RESET_GPIO, 0);
#endif
#ifdef CONFIG_VIDEO_MT9V034
	gpio_request(MT9V034_RESET_GPIO, "cam_rst");
	gpio_direction_output(MT9V034_RESET_GPIO, 0);
#endif
#ifdef CONFIG_VIDEO_MT9T111
	gpio_request(MT9T111_RESET_GPIO, "cam_rst");
	gpio_direction_output(MT9T111_RESET_GPIO, 0);
#endif
#ifdef CONFIG_VIDEO_MT9V032
	gpio_request(MT9V032_RESET_GPIO, "cam_rst");
	gpio_direction_output(MT9V032_RESET_GPIO, 0);
#endif
#ifdef CONFIG_VIDEO_AR0130
        gpio_request(AR0130_RESET_GPIO, "cam_rst");
        gpio_direction_output(AR0130_RESET_GPIO, 0);
#endif

	omap3_init_camera(&beagle_isp_platform_data);
	return 0;
}
late_initcall(beagle_camera_init);
