#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/log2.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <media/v4l2-subdev.h>
#include <linux/videodev2.h>

#include <media/ar0130.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/soc_camera.h>
#include "ar0130_data.h"

#define AR0130_ROW_START		0x01
#define		AR0130_ROW_START_MIN		0
#define		AR0130_ROW_START_MAX		1280
#define		AR0130_ROW_START_DEF		0	
#define AR0130_COLUMN_START		0x02
#define		AR0130_COLUMN_START_MIN		0
#define		AR0130_COLUMN_START_MAX		960
#define		AR0130_COLUMN_START_DEF		0
#define AR0130_WINDOW_HEIGHT		0x03
#define		AR0130_WINDOW_HEIGHT_MIN	360
#define		AR0130_WINDOW_HEIGHT_MAX	960
#define		AR0130_WINDOW_HEIGHT_DEF	960
#define AR0130_WINDOW_WIDTH		0x04
#define		AR0130_WINDOW_WIDTH_MIN		640
#define		AR0130_WINDOW_WIDTH_MAX		1280
#define		AR0130_WINDOW_WIDTH_DEF		1280

#define AR0130_PIXEL_ARRAY_WIDTH	1280
#define AR0130_PIXEL_ARRAY_HEIGHT	960

#define MAX_WIDTH   		1280
#define MAX_HEIGHT  		960
#define VGA_WIDTH		640
#define VGA_HEIGHT		480
#define ENABLE			1
#define DISABLE			0

#define AR0130_CHIP_ID 		0x2402
#define AR0130_RESET_REG 	0x301A
#define AR0130_STREAM_ON	0x10DC
#define AR0130_STREAM_OFF	0x10D8
#define AR0130_SEQ_PORT		0x3086	
#define AR0130_TEST_REG		0x3070
#define	AR0130_TEST_PATTERN	0x0000
/*
0 = Normal operation. Generate output data from pixel array
1 = Solid color test pattern.
2 = Full color bar test pattern
3 = Fade to grey color bar test pattern
256 = Marching 1 test pattern (12 bit)
*/

struct ar0130_frame_size {
	u16 width;
	u16 height;
};

const static struct ar0130_frame_size ar0130_supported_framesizes[] = {
	{  640,  360 },
	{  640,  480 },
	{ 1280,  720 },
	{ 1280,  960 },
};

enum resolution {
AR0130_640x360_BINNED,
AR0130_640x480_BINNED,
AR0130_720P_60FPS,
AR0130_FULL_RES_45FPS
};

struct ar0130_priv {
	struct v4l2_subdev subdev;
	struct media_pad pad;
	struct v4l2_rect crop;  /* Sensor window */
	struct v4l2_rect curr_crop;
	struct v4l2_mbus_framefmt format;
	enum resolution res_index;
	struct v4l2_ctrl_handler ctrls;
	struct ar0130_platform_data *pdata;
	struct mutex power_lock; /* lock to protect power_count */
	struct ar0130_pll_divs *pll;
	int power_count;
	int autoexposure;
	u16 xskip;
	u16 yskip;
	
	/* cache register values */
	u16 output_control;
};

/************************************************************************
			Helper Functions
************************************************************************/
/**
 * to_ar0130 - A helper function which returns pointer to the private data structure
 * @client: pointer to i2c client
 * 
 */
static struct ar0130_priv *to_ar0130(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ar0130_priv, subdev);
}

/**
 * reg_read - reads the data from the given register
 * @client: pointer to i2c client
 * @command: address of the register which is to be read
 *
 */
static int ar0130_reg_read(struct i2c_client *client, u16 command)
{
	struct i2c_msg msg[2];
	u8 buf[2];
	int ret;

	/* 16 bit addressable register */
	command = swab16(command);
	
	msg[0].addr  = client->addr;
	msg[0].flags = 0;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *)&command;;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD; //1
	msg[1].len   = 2;
	msg[1].buf   = buf;

	/*
	* if return value of this function is < 0,
	* it mean error.
	* else, under 16bit is valid data.
	*/
	ret = i2c_transfer(client->adapter, msg, 2);
	
	if (ret < 0)
		return ret;

	memcpy(&ret, buf, 2);
	return swab16(ret);
	
	v4l_err(client, "Read from offset 0x%x error %d", command, ret);
	return ret;
}

/**
 * reg_write - writes the data into the given register
 * @client: pointer to i2c client
 * @command: address of the register in which to write
 * @data: data to be written into the register
 *
 */
static int ar0130_reg_write(struct i2c_client *client, u16 command,
                       u16 data)
{
	struct i2c_msg msg;
	u8 buf[4];
	int ret;

	/* 16-bit addressable register */

	command = swab16(command);
	data = swab16(data);
	
	memcpy(buf + 0, &command, 2);
	memcpy(buf + 2, &data, 2);
	msg.addr  = client->addr;
	msg.flags = 0;
	msg.len   = 4;
	msg.buf   = buf;
	
	/* i2c_transfer return message length, but this function should return 0 if correct case */
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret >= 0)
		return 0;
	else
		v4l_err(client, "Write failed at 0x%X error %d\n", swab16(command), ret);
	
	return ret;
}

/**
 * ar0130_calc_size - Find the best match for a requested image capture size
 * @width: requested image width in pixels
 * @height: requested image height in pixels
 *
 * Find the best match for a requested image capture size.  The best match
 * is chosen as the nearest match that has the same number or fewer pixels
 * as the requested size, or the smallest image size if the requested size
 * has fewer pixels than the smallest image.
 */
static int ar0130_calc_size(unsigned int request_width,
				unsigned int request_height)
{
	int i = 0;
	unsigned long requested_pixels = request_width * request_height;

	for (i = 0; i < ARRAY_SIZE(ar0130_supported_framesizes); i++) {
		if (ar0130_supported_framesizes[i].height
			* ar0130_supported_framesizes[i].width >= requested_pixels)
		return i;
	}	

	/* couldn't find a match, return the max size as a default */
	return (ARRAY_SIZE(ar0130_supported_framesizes) - 1);
}

/**
 * ar0130_v4l2_try_fmt_cap - Find the best match for a requested image capture size
 * @ar0130_frame_size: a ointer to the structure which specifies requested image size
 * 
 * Find the best match for a requested image capture size.  The best match
 * is chosen as the nearest match that has the same number or fewer pixels
 * as the requested size, or the smallest image size if the requested size
 * has fewer pixels than the smallest image.
 */
static int ar0130_v4l2_try_fmt_cap(struct ar0130_frame_size *requestedsize) 
{
	int isize;
		
	isize = ar0130_calc_size(requestedsize->width,requestedsize->height);
	
	requestedsize->width = ar0130_supported_framesizes[isize].width;
	requestedsize->height = ar0130_supported_framesizes[isize].height;
	
	return isize;
}

/**
 * ar0130_reset - Soft resets the sensor
 * @client: pointer to the i2c client
 * 
 */
static int ar0130_reset(struct i2c_client *client)
{
        int ret;

        ret = ar0130_reg_write(client, AR0130_RESET_REG, 0x0001);
        if (ret < 0)
                return ret;
		
        mdelay(10);

        ret = ar0130_reg_write(client, AR0130_RESET_REG, AR0130_STREAM_OFF);
        if (ret < 0)
                return ret;

        return 0;
}

/**
 * ar0130_pll_enable - enable the sensor pll
 * @client: pointer to the i2c client
 * 
 */
static int ar0130_pll_enable(struct i2c_client *client)
{
	int ret;

	ret = ar0130_reg_write(client, 0x302C, 0x0002);		// VT_SYS_CLK_DIV
	ret |= ar0130_reg_write(client, 0x302A, 0x0004);	// VT_PIX_CLK_DIV
	ret |= ar0130_reg_write(client, 0x302E, 0x0002);	// PRE_PLL_CLK_DIV
	ret |= ar0130_reg_write(client, 0x3030, 0x002C);	// PLL_MULTIPLIER
	ret |= ar0130_reg_write(client, 0x30B0, 0x1300);	// DIGITAL_TEST

	mdelay(100);
	
	return ret;
}

/**
 * ar0130_power_on - power on the sensor
 * @ar0130: pointer to private data structure
 * 
 */
static int ar0130_power_on(struct ar0130_priv *ar0130)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0130->subdev);

        /* Ensure RESET_BAR is low */
        if (ar0130->pdata->reset) {
                ar0130->pdata->reset(&ar0130->subdev, 1);
                msleep(1);
        }

        /* Enable clock */
        if (ar0130->pdata->set_xclk) {
                ar0130->pdata->set_xclk(&ar0130->subdev,
                ar0130->pdata->ext_freq);
                msleep(1);
		}
		
        /* Now RESET_BAR must be high */
        if (ar0130->pdata->reset) {
                ar0130->pdata->reset(&ar0130->subdev, 0);
                msleep(1);
        }

	ar0130_reset(client);

        return 0;
}

/**
 * ar0130_power_off - power off the sensor
 * @ar0130: pointer to private data structure
 * 
 */
static int ar0130_power_off(struct ar0130_priv *ar0130)
{
	if (ar0130->pdata->set_xclk)
		ar0130->pdata->set_xclk(&ar0130->subdev, 0);
	
	return 0;
}

static struct v4l2_mbus_framefmt *
__ar0130_get_pad_format(struct ar0130_priv *ar0130, struct v4l2_subdev_fh *fh,
			unsigned int pad, u32 which)
{
	switch (which) {
		case V4L2_SUBDEV_FORMAT_TRY:
			return v4l2_subdev_get_try_format(fh, pad);
		case V4L2_SUBDEV_FORMAT_ACTIVE:
			return &ar0130->format;
		default:
			return NULL;
	}
}

static int ar0130_linear_mode_setup(struct i2c_client *client)
{
	int i, ret;

	ret = ar0130_reg_write(client, 0x3088, 0x8000);		// SEQ_CTRL_PORT
	for(i = 0; i < 80; i++)
		ret |= ar0130_reg_write(client, AR0130_SEQ_PORT, ar0130_linear_data[i]);
 
	ret |= ar0130_reg_write(client, 0x309E, 0x0000);	// DCDS_PROG_START_ADDR
	ret |= ar0130_reg_write(client, 0x30E4, 0x6372);	// ADC_BITS_6_7
	ret |= ar0130_reg_write(client, 0x30E2, 0x7253);	// ADC_BITS_4_5
	ret |= ar0130_reg_write(client, 0x30E0, 0x5470);	// ADC_BITS_2_3
	ret |= ar0130_reg_write(client, 0x30E6, 0xC4CC);	// ADC_CONFIG1
	ret |= ar0130_reg_write(client, 0x30E8, 0x8050);	// ADC_CONFIG2

	mdelay(200);

	ret |= ar0130_reg_write(client, 0x3082, 0x0029);	// OPERATION_MODE_CTRL
	ret |= ar0130_reg_write(client, 0x30B0, 0x1300);	// DIGITAL_TEST
	ret |= ar0130_reg_write(client, 0x30D4, 0xE007);	// COLUMN_CORRECTION
	ret |= ar0130_reg_write(client, 0x301A, 0x10DC);	// RESET_REGISTER
	ret |= ar0130_reg_write(client, 0x301A, 0x10D8);	// RESET_REGISTER
	ret |= ar0130_reg_write(client, 0x3044, 0x0400);	// DARK_CONTROL
	ret |= ar0130_reg_write(client, 0x3EDA, 0x0F03);	// DAC_LD_14_15
	ret |= ar0130_reg_write(client, 0x3ED8, 0x01EF);	// DAC_LD_12_13
	ret |= ar0130_reg_write(client, 0x3012, 0x02A0);	// COARSE_INTEGRATION_TIME

	return ret;
}

static int ar0130_set_resolution(struct i2c_client *client, enum resolution res_index)
{
	int ret;

	switch(res_index){
	case AR0130_FULL_RES_45FPS: //1280x960
		ret = ar0130_reg_write(client, 0x3032, 0x0000);		// DIGITAL_BINNING
		ret |= ar0130_reg_write(client, 0x3002, 0x0002);	// Y_ADDR_START
		ret |= ar0130_reg_write(client, 0x3004, 0x0000);	// X_ADDR_START
		ret |= ar0130_reg_write(client, 0x3006, 0x03C1);	// Y_ADDR_END
		ret |= ar0130_reg_write(client, 0x3008, 0x04FF);	// X_ADDR_END
		ret |= ar0130_reg_write(client, 0x300A, 0x03DE);	// FRAME_LENGTH_LINES
		ret |= ar0130_reg_write(client, 0x300C, 0x0672);	// LINE_LENGTH_PCK
		break;
	case AR0130_720P_60FPS: //1280x720 
		ret = ar0130_reg_write(client, 0x3032, 0x0000);		// DIGITAL_BINNING
		ret |= ar0130_reg_write(client, 0x3002, 0x0002);	// Y_ADDR_START
		ret |= ar0130_reg_write(client, 0x3004, 0x0000);	// X_ADDR_START
		ret |= ar0130_reg_write(client, 0x3006, 0x02D1);	// Y_ADDR_END
		ret |= ar0130_reg_write(client, 0x3008, 0x04FF);	// X_ADDR_END
		ret |= ar0130_reg_write(client, 0x300A, 0x02EF);	// FRAME_LENGTH_LINES
		ret |= ar0130_reg_write(client, 0x300C, 0x0672);	// LINE_LENGTH_PCK
		break;
	case AR0130_640x480_BINNED: //(640,480):
		ret = ar0130_reg_write(client, 0x3032, 0x0002);		// DIGITAL_BINNING
		ret |= ar0130_reg_write(client, 0x3002, 0x0002);	// Y_ADDR_START
		ret |= ar0130_reg_write(client, 0x3004, 0x0000);	// X_ADDR_START
		ret |= ar0130_reg_write(client, 0x3006, 0x03C1);	// Y_ADDR_END
		ret |= ar0130_reg_write(client, 0x3008, 0x04FF);	// X_ADDR_END
		ret |= ar0130_reg_write(client, 0x300A, 0x03DE);	// FRAME_LENGTH_LINES
		ret |= ar0130_reg_write(client, 0x300C, 0x0672);	// LINE_LENGTH_PCK
		ret |= ar0130_reg_write(client, 0x306E, 0x9010);	// DATAPATH_SELECT
		break;
	case AR0130_640x360_BINNED: //(640,360):
		ret = ar0130_reg_write(client, 0x3032, 0x0002);		// DIGITAL_BINNING
		ret |= ar0130_reg_write(client, 0x3002, 0x0002);	// Y_ADDR_START
		ret |= ar0130_reg_write(client, 0x3004, 0x0000);	// X_ADDR_START
		ret |= ar0130_reg_write(client, 0x3006, 0x02D1);	// Y_ADDR_END
		ret |= ar0130_reg_write(client, 0x3008, 0x04FF);	// X_ADDR_END
		ret |= ar0130_reg_write(client, 0x300A, 0x03DE);	// FRAME_LENGTH_LINES
		ret |= ar0130_reg_write(client, 0x300C, 0x0672);	// LINE_LENGTH_PCK
		ret |= ar0130_reg_write(client, 0x306E, 0x9010);	// DATAPATH_SELECT
		break;
	default: // AR0130_FULL_RES_45FPS
		ret = ar0130_reg_write(client, 0x3032, 0x0000);		// DIGITAL_BINNING
		ret |= ar0130_reg_write(client, 0x3002, 0x0002);	// Y_ADDR_START
		ret |= ar0130_reg_write(client, 0x3004, 0x0000);	// X_ADDR_START
		ret |= ar0130_reg_write(client, 0x3006, 0x03C1);	// Y_ADDR_END
		ret |= ar0130_reg_write(client, 0x3008, 0x04FF);	// X_ADDR_END
		ret |= ar0130_reg_write(client, 0x300A, 0x03DE);	// FRAME_LENGTH_LINES
		ret |= ar0130_reg_write(client, 0x300C, 0x0672);	// LINE_LENGTH_PCK
		break;
	}
	
	return ret;
}

static int ar0130_set_autoexposure(struct i2c_client *client, int enable)
{
	struct ar0130_priv *ar0130 = to_ar0130(client);
	int ret;

	if(enable){
		ar0130->autoexposure = 1;
		ret = ar0130_reg_write(client, 0x3064, 0x1982);		// EMBEDDED_DATA_CTRL
		ret |= ar0130_reg_write(client, 0x3100, 0x001B);	// AE_CTRL_REG
		ret |= ar0130_reg_write(client, 0x3112, 0x029F);	// AE_DCG_EXPOSURE_HIGH_REG
		ret |= ar0130_reg_write(client, 0x3114, 0x008C);	// AE_DCG_EXPOSURE_LOW_REG
		ret |= ar0130_reg_write(client, 0x3116, 0x02C0);	// AE_DCG_GAIN_FACTOR_REG
		ret |= ar0130_reg_write(client, 0x3118, 0x005B);	// AE_DCG_GAIN_FACTOR_INV_REG
		ret |= ar0130_reg_write(client, 0x3102, 0x0384);	// AE_LUMA_TARGET_REG
		ret |= ar0130_reg_write(client, 0x3104, 0x1000);	// AE_HIST_TARGET_REG
		ret |= ar0130_reg_write(client, 0x3126, 0x0080);	// AE_ALPHA_V1_REG
		ret |= ar0130_reg_write(client, 0x311C, 0x03DD);	// AE_MAX_EXPOSURE_REG
		ret |= ar0130_reg_write(client, 0x311E, 0x0002);	// AE_MIN_EXPOSURE_REG
		return ret;
	}
	else {
		ar0130->autoexposure = 0;
		ret = ar0130_reg_write(client, AR0130_RESET_REG, AR0130_STREAM_OFF);
		ret |= ar0130_reg_write(client, 0x3100, 0x001A);	// AE_CTRL_REG
		ret |= ar0130_reg_write(client, AR0130_RESET_REG, AR0130_STREAM_ON);
		return ret;
	}
}

/************************************************************************
                        v4l2_subdev_core_ops
************************************************************************/
static int ar0130_g_chip_ident(struct v4l2_subdev *sd,
				struct v4l2_dbg_chip_ident *id)
{
	id->ident    = V4L2_IDENT_AR0130;
	id->revision = 1;
	
	return 0;
}

static int ar0130_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ar0130_priv *ar0130 = container_of(sd, struct ar0130_priv, subdev);

	switch (ctrl->id) {
		case V4L2_CID_EXPOSURE_AUTO:
			ctrl->value = ar0130->autoexposure;
			break;
	}
	
	return 0;
}

static int ar0130_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;
	
	switch (ctrl->id) {
		case V4L2_CID_EXPOSURE_AUTO:
			ret =  ar0130_set_autoexposure(client, ctrl->value);
		break;
	}

	return ret;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ar0130_g_reg(struct v4l2_subdev *sd,
				struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 data;

	reg->size = 2;
	data = ar0130_reg_read(client, reg->reg);

	reg->val = (__u64)data;
	return 0;
}

static int ar0130_s_reg(struct v4l2_subdev *sd,
				struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	ret = ar0130_reg_write(client, reg->reg, reg->val);
	return ret;
}
#endif

static int ar0130_s_power(struct v4l2_subdev *sd, int on)
{
	struct ar0130_priv *ar0130 = container_of(sd, struct ar0130_priv, subdev);
	int ret = 0;

	mutex_lock(&ar0130->power_lock);
	
	/*
	* If the power count is modified from 0 to != 0 or from != 0 to 0,
	* update the power state.
	*/	
	if (ar0130->power_count == !on) {
		if (on) {
			ret = ar0130_power_on(ar0130);
			if (ret) {
				dev_err(ar0130->subdev.v4l2_dev->dev,
				"Failed to power on: %d\n", ret);
				goto out;
			}
		} else 
			ret = ar0130_power_off(ar0130);
	} 
	/* Update the power count. */
	ar0130->power_count += on ? 1 : -1;
	WARN_ON(ar0130->power_count < 0);
out:
	mutex_unlock(&ar0130->power_lock);
	return ret; 
}

/***************************************************
		v4l2_subdev_video_ops	
****************************************************/
static int ar0130_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0130_priv *ar0130 = container_of(sd, struct ar0130_priv, subdev);
	int ret;

	if (!enable) {
		return 0;
	}
	
	ret = ar0130_linear_mode_setup(client);
	if(ret < 0){
		dev_err(ar0130->subdev.v4l2_dev->dev, "Failed to setup linear mode: %d\n", ret);
		return ret;
	}

	ret = ar0130_set_resolution(client, ar0130->res_index);
	if(ret < 0){
		dev_err(ar0130->subdev.v4l2_dev->dev, "Failed to setup resolution: %d\n", ret);
		return ret;
	}

	ret |= ar0130_reg_write(client, AR0130_RESET_REG, AR0130_STREAM_OFF);
	ret |= ar0130_reg_write(client, 0x31D0, 0x0001);	// HDR_COMP

	ret = ar0130_pll_enable(client);
	if(ret < 0){
		dev_err(ar0130->subdev.v4l2_dev->dev, "Failed to enable pll: %d\n", ret);
		return ret;
	}

	ret |= ar0130_reg_write(client, AR0130_RESET_REG, AR0130_STREAM_ON);

	ret |= ar0130_set_autoexposure(client, ENABLE);

	ret |= ar0130_reg_write(client, AR0130_TEST_REG, AR0130_TEST_PATTERN);
	
	return ret;
}

/***************************************************
		v4l2_subdev_pad_ops
****************************************************/
static int ar0130_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_mbus_code_enum *code)
{
	struct ar0130_priv *ar0130 = container_of(sd, struct ar0130_priv, subdev);

	if (code->pad || code->index)
		return -EINVAL;
	
	code->code = ar0130->format.code;
		return 0;
}

static int ar0130_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_frame_size_enum *fse)
{
	struct ar0130_priv *ar0130 = container_of(sd, struct ar0130_priv, subdev);
	
	if (fse->index >= 8 || fse->code != ar0130->format.code)
		return -EINVAL;

	fse->min_width = AR0130_WINDOW_WIDTH_DEF
				/ min_t(unsigned int, 7, fse->index + 1);
	fse->max_width = fse->min_width;
	fse->min_height = AR0130_WINDOW_HEIGHT_DEF / (fse->index + 1);
	fse->max_height = fse->min_height;

	return 0;
}

static int ar0130_get_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct ar0130_priv *ar0130 = container_of(sd, struct ar0130_priv, subdev);
	
	fmt->format = *__ar0130_get_pad_format(ar0130, fh, fmt->pad,
						fmt->which);
	
	return 0;
}

static int ar0130_set_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *format)
{
	struct ar0130_priv *ar0130 = container_of(sd, struct ar0130_priv, subdev);
	struct ar0130_frame_size size;

	size.height 	= format->format.height;
	size.width 	= format->format.width;	
	ar0130->res_index 		= ar0130_v4l2_try_fmt_cap(&size);
	ar0130->crop.width		= size.width;
	ar0130->crop.height		= size.height;
	ar0130->curr_crop.width		= size.width;
	ar0130->curr_crop.height	= size.height;
	ar0130->format.width		= size.width;
	ar0130->format.height		= size.height;
	ar0130->format.code 		= V4L2_MBUS_FMT_SGRBG12_1X12;	

	format->format.width		= size.width;
	format->format.height		= size.height;
	
	return 0;
}

static int ar0130_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	a->c.left	= 0;
	a->c.top	= 0;
	a->c.width	= VGA_WIDTH;
	a->c.height	= VGA_HEIGHT;
	a->type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return 0;
}

static int ar0130_s_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	struct v4l2_rect *rect = &a->c;
	struct ar0130_frame_size size;
	struct ar0130_priv *ar0130 = container_of(sd, struct ar0130_priv, subdev);

	size.height = rect->height;
	size.width = rect->width;	
	ar0130_v4l2_try_fmt_cap(&size);
	ar0130->crop.width		= size.width;
	ar0130->crop.height		= size.height;
	ar0130->curr_crop.width		= size.width;
	ar0130->curr_crop.height	= size.height;
	ar0130->format.width		= size.width;
	ar0130->format.height		= size.height;
	ar0130->format.code 		= V4L2_MBUS_FMT_SGRBG12_1X12;	
	
	return 0;
}

/***********************************************************
	V4L2 subdev internal operations
************************************************************/
static int ar0130_registered(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0130_priv *ar0130 = to_ar0130(client);
	s32 data;
	int ret;

	ret = ar0130_power_on(ar0130);
	if (ret < 0) {
		dev_err(&client->dev, "AR0130 power up failed\n");
		return ret;
	}

	/* Read out the chip version register */
	data = ar0130_reg_read(client, 0x3000);
	if (data != AR0130_CHIP_ID) {
		dev_err(&client->dev, "AR0130 not detected, wrong chip ID "
					"0x%4.4X\n", data);
		return -ENODEV;
	}

	dev_info(&client->dev, "AR0130 detected at address 0x%02X: chip ID = 0x%4.4X\n",
			client->addr, AR0130_CHIP_ID);
			
	ret = ar0130_power_off(ar0130);

	return ret;
}

static int ar0130_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct ar0130_priv *ar0130 = container_of(sd, struct ar0130_priv, subdev);
	int ret = 0;
    
	ar0130->crop.width     = AR0130_WINDOW_WIDTH_DEF;
	ar0130->crop.height    = AR0130_WINDOW_HEIGHT_DEF;
	ar0130->crop.left      = AR0130_COLUMN_START_DEF;
	ar0130->crop.top       = AR0130_ROW_START_DEF;
	
	ar0130->format.code 		= V4L2_MBUS_FMT_SGRBG12_1X12;
	ar0130->format.width 		= AR0130_WINDOW_WIDTH_DEF;
	ar0130->format.height 		= AR0130_WINDOW_HEIGHT_DEF;
	ar0130->format.field 		= V4L2_FIELD_NONE;
	ar0130->format.colorspace 	= V4L2_COLORSPACE_SRGB;
    
	ret = ar0130_s_power(sd, 1);
	return ret;
}

static int ar0130_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	int ret = 0;
	
	ret = ar0130_s_power(sd, 0);
	return ret;
}

/***************************************************
		v4l2_subdev_ops
****************************************************/
static struct v4l2_subdev_core_ops ar0130_subdev_core_ops = {
	.g_chip_ident	= ar0130_g_chip_ident,
	.g_ctrl		= ar0130_g_ctrl,
	.s_ctrl		= ar0130_s_ctrl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register	= ar0130_g_reg,
	.s_register	= ar0130_s_reg,
#endif
	.s_power	= ar0130_s_power,
};

static struct v4l2_subdev_video_ops ar0130_subdev_video_ops = {
	.s_stream 	= ar0130_s_stream,
	.g_crop		= ar0130_g_crop,
	.s_crop		= ar0130_s_crop,
};

static struct v4l2_subdev_pad_ops ar0130_subdev_pad_ops = {
	.enum_mbus_code	 = ar0130_enum_mbus_code,
	.enum_frame_size = ar0130_enum_frame_size,
	.get_fmt  	 = ar0130_get_format,
	.set_fmt 	 = ar0130_set_format,
};

static struct v4l2_subdev_ops ar0130_subdev_ops = {
	.core	= &ar0130_subdev_core_ops,
	.video	= &ar0130_subdev_video_ops,
	.pad	= &ar0130_subdev_pad_ops,
};

/* 
 * Internal ops. Never call this from drivers, only the v4l2 framework can call
 * these ops.
 */
static const struct v4l2_subdev_internal_ops ar0130_subdev_internal_ops = {
	.registered	= ar0130_registered,
	.open		= ar0130_open,
	.close		= ar0130_close,
};

/***************************************************
		I2C driver
****************************************************/
static int ar0130_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct ar0130_platform_data *pdata = client->dev.platform_data;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);
	struct ar0130_priv *ar0130;
	int ret;

	if (pdata == NULL) {
		dev_err(&client->dev, "No platform data\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA)) {
		dev_warn(&client->dev, "I2C-Adapter doesn't support I2C_FUNC_SMBUS_WORD\n");
		return -EIO;
	}

	ar0130 = kzalloc(sizeof(*ar0130), GFP_KERNEL);
	if (ar0130 == NULL)
		return -ENOMEM;

	ar0130->pdata = pdata;
	
	mutex_init(&ar0130->power_lock);
	v4l2_i2c_subdev_init(&ar0130->subdev, client, &ar0130_subdev_ops);
	ar0130->subdev.internal_ops = &ar0130_subdev_internal_ops;

	ar0130->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_init(&ar0130->subdev.entity, 1, &ar0130->pad, 0);
	if (ret < 0)
		goto done;

	ar0130->subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	ar0130->crop.width	= AR0130_WINDOW_WIDTH_DEF;
	ar0130->crop.height 	= AR0130_WINDOW_HEIGHT_DEF;
	ar0130->crop.left	= AR0130_COLUMN_START_DEF;
	ar0130->crop.top	= AR0130_ROW_START_DEF;

	ar0130->format.code 		= V4L2_MBUS_FMT_SGRBG12_1X12;
	ar0130->format.width 		= AR0130_WINDOW_WIDTH_DEF;
	ar0130->format.height 		= AR0130_WINDOW_HEIGHT_DEF;
	ar0130->format.field 		= V4L2_FIELD_NONE;
	ar0130->format.colorspace	= V4L2_COLORSPACE_SRGB;

done:
	if (ret < 0) {
		v4l2_device_unregister_subdev(subdev);
		dev_err(&client->dev, "Probe failed\n");
		media_entity_cleanup(&ar0130->subdev.entity);
		kfree(ar0130);
	}

	return ret;
}

static int ar0130_remove(struct i2c_client *client)
{
	struct ar0130_priv *ar0130 = to_ar0130(client);
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(subdev);
	media_entity_cleanup(&ar0130->subdev.entity);
	kfree(ar0130);
	return 0;
}

static const struct i2c_device_id ar0130_id[] = {
	{ "ar0130", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0130);

static struct i2c_driver ar0130_i2c_driver = {
	.driver = {
		 .name = "ar0130",
	},
	.probe    = ar0130_probe,
	.remove   = ar0130_remove,
	.id_table = ar0130_id,
};

/**************************************************
		Module functions
***************************************************/
static int __init ar0130_module_init(void)
{
	return i2c_add_driver(&ar0130_i2c_driver);
}

static void __exit ar0130_module_exit(void)
{
	i2c_del_driver(&ar0130_i2c_driver);
}
module_init(ar0130_module_init);
module_exit(ar0130_module_exit);

MODULE_DESCRIPTION("CMOS sensor driver for AR0130");
MODULE_AUTHOR("Aptina Imaging");
MODULE_LICENSE("GPL v2");
