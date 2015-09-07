#include <linux/i2c.h>
#include <linux/kernel.h>
#include "lt070me05000.h"

extern s32 bsp_disp_get_panel_info(u32 screen_id, disp_panel_para *info);
static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

static u8 const mipi_dcs_pixel_format[4] = {0x77,0x66,0x66,0x55};
#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 0, val)
//#define power_en(val)  sunxi_lcd_gpio_set_value(sel, 0, val)

static struct i2c_adapter *tv_i2c_adapter;
static struct i2c_client *tv_i2c_client;
static char modules_name[32] = "TC358770A";
static u32 tv_i2c_id = 1;
static u32 tv_i2c_used = 1;//Ä¬ÈÏÊÇ0µÄ

static int tv_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	disp_vdevice_init_data init_data;

	memset(&init_data, 0, sizeof(disp_vdevice_init_data));
	tv_i2c_client = client;
#if 0
	init_data.disp = 0;//tv_screen_id;
	memcpy(init_data.name, modules_name, 32);
	init_data.type = DISP_OUTPUT_TYPE_TV;
	init_data.fix_timing = 0;

	init_data.func.enable = gm7121_tv_open;
	init_data.func.disable = gm7121_tv_close;
	init_data.func.get_HPD_status = gm7121_tv_get_hpd_status;
	init_data.func.set_mode = gm7121_tv_set_mode;
	init_data.func.mode_support = gm7121_tv_get_mode_support;
	init_data.func.get_video_timing_info = gm7121_tv_get_video_timing_info;
	init_data.func.get_interface_para = gm7121_tv_get_interface_para;
	init_data.func.get_input_csc = gm7121_tv_get_input_csc;
	disp_vdevice_get_source_ops(&tv_source_ops);
	tv_parse_config();

	gm7121_device = disp_vdevice_register(&init_data);
#endif
	return 0;
}

static int __devexit tv_i2c_remove(struct i2c_client *client)
{
#if 0
		if(gm7121_device)
			disp_vdevice_unregister(gm7121_device);
		gm7121_device = NULL;
#endif
    return 0;
}

static const struct i2c_device_id tv_i2c_id_table[] = {
    { "tv_i2c", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, tv_i2c_id_table);

static int tv_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    if(tv_i2c_id == client->adapter->nr){
        const char *type_name = "tv_i2c";
        tv_i2c_adapter = client->adapter;
        printk("[DISP_I2C] tv_i2c_detect, get right i2c adapter, id=%d\n", tv_i2c_adapter->nr);
        strlcpy(info->type, type_name, I2C_NAME_SIZE);
        return 0;
    }
    return -ENODEV;
}

static  unsigned short normal_i2c[] = {0xf, I2C_CLIENT_END};

static struct i2c_driver tv_i2c_driver = {
    .class = I2C_CLASS_HWMON,
    .probe        = tv_i2c_probe,
    .remove        = __devexit_p(tv_i2c_remove),
    .id_table    = tv_i2c_id_table,
    .driver    = {
        .name    = "tv_i2c",
        .owner    = THIS_MODULE,
    },
    .detect        = tv_i2c_detect,
    .address_list    = normal_i2c,
};

static int  tv_i2c_init(void)
{
    int ret;
    int value;

    ret = disp_sys_script_get_item("dp_para", "tv_twi_used", &value, 1);
    if(1 == ret)
    {
        tv_i2c_used = value;
        if(tv_i2c_used == 1)
        {
            ret = disp_sys_script_get_item("dp_para", "tv_twi_id", &value, 1);
            tv_i2c_id = (ret == 1)? value:tv_i2c_id;

            ret = disp_sys_script_get_item("dp_para", "tv_twi_addr", &value, 1);
            normal_i2c[0] = (ret == 1)? value:normal_i2c[0];

            return i2c_add_driver(&tv_i2c_driver);
        }
    }
    return 0;
}

static void tv_i2c_exit(void)
{
    i2c_del_driver(&tv_i2c_driver);
}

static s32 tv_i2c_write(u8 count, u8 d1, u8 d2, u8 d3, u8 d4, u8 d5, u8 d6)
{
    u8 i;
    s32 ret = 0;
    u8 i2c_data[count];
    u8 parker[6];
    parker[0] = d1;
    parker[1] = d2;
    parker[2] = d3;
    parker[3] = d4;
    parker[4] = d5;
    parker[5] = d6;

    struct i2c_msg msg;

    for(i=0;i<count;i++)
    {
	i2c_data[i]=parker[i];
    }
   

    printk("[i2c_write]  count = %d, data=", count);
    for(i=0;i<count;i++)
    {
	printk("0x%x ", i2c_data[i]);
    }
 

    if(tv_i2c_used)
    {
        msg.addr = tv_i2c_client->addr;
        msg.flags = 0;
        msg.len = count;
        msg.buf = i2c_data;
        ret = i2c_transfer(tv_i2c_adapter, &msg, 1);
    }

	printk("\n");
	if(ret < 0) {
	printk("tv_i2c_write error!!!\n");
	}
	return ; 
}


static s32 tv_i2c_read(u8 count)
{
	s32 ret = 0;
	u8 data[4];
	struct i2c_msg msg;
        u8 i;

	msg.addr   = tv_i2c_client->addr;
	msg.flags  = I2C_M_RD;
	msg.len	   = count;
	msg.buf	   = data;

	if(tv_i2c_used) {
		ret = i2c_transfer(tv_i2c_adapter, &msg, 1);
	}

	if(ret < 0)
		printk("i2c_read error!!!\n");

	printk("[i2c_read]  data=", ret);

	for(i=0;i<count;i++)
	{
		printk("0x%x ",data[i]);
	}
        printk("\n");

	return ;
}


static void LCD_cfg_panel_info(panel_extend_para * info)
{
#if 0
	u32 i = 0, j=0;
	u32 items;
	u8 lcd_gamma_tbl[][2] =
	{
		//{input value, corrected value}
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	{
		{LCD_CMAP_G0,LCD_CMAP_B1,LCD_CMAP_G2,LCD_CMAP_B3},
		{LCD_CMAP_B0,LCD_CMAP_R1,LCD_CMAP_B2,LCD_CMAP_R3},
		{LCD_CMAP_R0,LCD_CMAP_G1,LCD_CMAP_R2,LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3,LCD_CMAP_G2,LCD_CMAP_B1,LCD_CMAP_G0},
		{LCD_CMAP_R3,LCD_CMAP_B2,LCD_CMAP_R1,LCD_CMAP_B0},
		{LCD_CMAP_G3,LCD_CMAP_R2,LCD_CMAP_G1,LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl)/2;
	for(i=0; i<items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for(j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) + (lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
#endif
	return ;
}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 100);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 200);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 100);     //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	200);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);   //close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(u32 sel)
{

        /*sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power
	sunxi_lcd_power_enable(sel, 1);//config lcd_power pin to open lcd power0
	sunxi_lcd_delay_ms(4);
	sunxi_lcd_power_enable(sel, 2);//config lcd_power pin to open lcd power2
	sunxi_lcd_delay_ms(2);
        sunxi_lcd_power_enable(sel, 3);//config lcd_power pin to open lcd power3 */
	sunxi_lcd_delay_ms(5);
	panel_reset(1);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
        /*sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_power_disable(sel, 3);//config lcd_power pin to close lcd power2
	sunxi_lcd_delay_ms(4);
	sunxi_lcd_power_disable(sel, 2);//config lcd_power pin to close lcd power1
	sunxi_lcd_delay_ms(2);
	sunxi_lcd_power_disable(sel, 1);//config lcd_power pin to close lcd power
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power*/
	return ;
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
}

static void LCD_panel_init(u32 sel)
{
	disp_panel_para *panel_info = disp_sys_malloc(sizeof(disp_panel_para));
    	tv_i2c_init();


	tv_i2c_write(2,0x05,0x00,0x00,0x00,0x00,0x00);
        tv_i2c_read(4);

	printk("\n//Setup Main Link\n");
	tv_i2c_write(6,0x06,0xa0,0x95,0x40,0x00,0x00);
	tv_i2c_write(6,0x09,0x18,0x06,0x01,0x00,0x00);

	printk("\n//DP-PHY/PLLs\n");
	tv_i2c_write(6,0x08,0x00,0x0f,0x00,0x00,0x00);
	tv_i2c_write(6,0x09,0x00,0x05,0x00,0x00,0x00);
	sunxi_lcd_delay_ms(1);
	tv_i2c_write(6,0x09,0x14,0x3d,0x82,0x22,0x00);
	tv_i2c_write(6,0x09,0x08,0x05,0x00,0x00,0x00);

	printk("\n//Reset/Enable Main Link(s)\n");
	tv_i2c_write(6,0x08,0x00,0x1f,0x00,0x00,0x00);
	tv_i2c_write(6,0x08,0x00,0x0f,0x00,0x00,0x00);
	tv_i2c_write(2,0x08,0x00,0x00,0x00,0x00,0x00);
        tv_i2c_read(4);

	printk("\n//Read DP Rx Link Capability\n");
	tv_i2c_write(6,0x06,0x64,0x3f,0x06,0x01,0x00);
	tv_i2c_write(6,0x06,0x68,0x01,0x00,0x00,0x00);
	tv_i2c_write(6,0x06,0x60,0x09,0x00,0x00,0x00);
	tv_i2c_write(2,0x06,0x8c,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);

	tv_i2c_write(2,0x06,0x7c,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);

	tv_i2c_write(6,0x06,0x68,0x02,0x00,0x00,0x00);
	tv_i2c_write(6,0x06,0x60,0x09,0x00,0x00,0x00);
	tv_i2c_write(2,0x06,0x8c,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);

	tv_i2c_write(2,0x06,0x7c,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);

	printk("\n//Setup Link & DPRx Config for Training\n");
	tv_i2c_write(6,0x06,0x68,0x00,0x01,0x00,0x00);
	tv_i2c_write(6,0x06,0x6c,0x0a,0x02,0x00,0x00);
	tv_i2c_write(6,0x06,0x60,0x08,0x01,0x00,0x00);
	tv_i2c_write(6,0x06,0x68,0x08,0x01,0x00,0x00);
	tv_i2c_write(6,0x06,0x6c,0x01,0x00,0x00,0x00);
	tv_i2c_write(6,0x06,0x60,0x08,0x00,0x00,0x00);

	printk("\n//Set DPCD 00102h for Training Pat 1\n");
	tv_i2c_write(6,0x06,0xe4,0x21,0x00,0x00,0x00);
	tv_i2c_write(6,0x06,0xd8,0x0d,0x00,0x00,0x76);
	
	printk("\n//Set DP0 Trainin Pattern 1\n");
	tv_i2c_write(6,0x06,0xa0,0x95,0x41,0x00,0x00);
		
	printk("\n//Enable DP0 to start Link Training\n");
	tv_i2c_write(6,0x06,0x00,0x41,0x00,0x00,0x00);
	sunxi_lcd_delay_ms(750);
	tv_i2c_write(2,0x06,0xd0,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);

	printk("\n//Set DPCD 00102h for Link Traing Pat 2\n");
	tv_i2c_write(6,0x06,0xe4,0x22,0x00,0x00,0x00);

	printk("\n//Set DP0 Trainin Pattern 2\n");
	tv_i2c_write(6,0x06,0xa0,0x95,0x42,0x00,0x00);
	sunxi_lcd_delay_ms(750);
	tv_i2c_write(2,0x06,0xd0,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);

	printk("\n//Clear DPCD 00102h\n");
	tv_i2c_write(6,0x06,0x68,0x02,0x01,0x00,0x00);
	tv_i2c_write(6,0x06,0x6c,0x00,0x00,0x00,0x00);
	tv_i2c_write(6,0x06,0x60,0x08,0x00,0x00,0x00);

	printk("\n//Clear DP0 Trainin Pattern\n");
	tv_i2c_write(6,0x06,0xa0,0x95,0x40,0x00,0x00);

	printk("\n//Read DPCD 0x00200-0x00204\n");
	tv_i2c_write(6,0x06,0x68,0x00,0x02,0x00,0x00);
	tv_i2c_write(6,0x06,0x60,0x09,0x04,0x00,0x00);
	tv_i2c_write(2,0x06,0x8c,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);

	tv_i2c_write(2,0x06,0x7c,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);
	tv_i2c_write(2,0x06,0x80,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);

	printk("\nEnable ASSR\n");
	tv_i2c_write(6,0x06,0x68,0x0a,0x01,0x00,0x00);
	tv_i2c_write(6,0x06,0x6c,0x00,0x00,0x00,0x00);
	tv_i2c_write(6,0x06,0x60,0x08,0x00,0x00,0x00);

	printk("\n//DSI0 Setting\n");
	tv_i2c_write(6,0x01,0x3c,0x0b,0x00,0x08,0x00);
	tv_i2c_write(6,0x01,0x14,0x07,0x00,0x00,0x00);
	tv_i2c_write(6,0x01,0x64,0x08,0x00,0x00,0x00);
	tv_i2c_write(6,0x01,0x68,0x08,0x00,0x00,0x00);
	tv_i2c_write(6,0x01,0x6c,0x08,0x00,0x00,0x00);
	tv_i2c_write(6,0x01,0x70,0x08,0x00,0x00,0x00);
	tv_i2c_write(6,0x01,0x34,0x1f,0x00,0x00,0x00);
	tv_i2c_write(6,0x02,0x10,0x1f,0x00,0x00,0x00);
	tv_i2c_write(6,0x01,0x04,0x01,0x00,0x00,0x00);
	tv_i2c_write(6,0x02,0x04,0x01,0x00,0x00,0x00);

	printk("\n//DSI1 Setting\n");
	tv_i2c_write(6,0x11,0x3c,0x08,0x00,0x06,0x00);
	tv_i2c_write(6,0x11,0x14,0x05,0x00,0x00,0x00);
	tv_i2c_write(6,0x11,0x64,0x06,0x00,0x00,0x00);
	tv_i2c_write(6,0x11,0x68,0x06,0x00,0x00,0x00);
	tv_i2c_write(6,0x11,0x6c,0x06,0x00,0x00,0x00);
	tv_i2c_write(6,0x11,0x70,0x06,0x00,0x00,0x00);
	tv_i2c_write(6,0x11,0x34,0x1f,0x00,0x00,0x00);
	tv_i2c_write(6,0x12,0x10,0x1f,0x00,0x00,0x00);
	tv_i2c_write(6,0x11,0x04,0x01,0x00,0x00,0x00);
	tv_i2c_write(6,0x12,0x04,0x01,0x00,0x00,0x00);

	printk("\n//Combiner Logic\n");
	tv_i2c_write(6,0x04,0x80,0x00,0x00,0x00,0x00);
	tv_i2c_write(6,0x04,0x84,0x00,0x00,0x80,0x07);
	tv_i2c_write(6,0x04,0x88,0x00,0x00,0x00,0x00);
	tv_i2c_write(6,0x04,0x8c,0x00,0x00,0x00,0x00);
	tv_i2c_write(6,0x04,0x90,0x00,0x00,0x00,0x00);
	tv_i2c_write(6,0x04,0x94,0x00,0x00,0x00,0x00);

	printk("\n//LCD Ctl Frame Size\n");
	tv_i2c_write(6,0x04,0x50,0x00,0x01,0xa0,0x00);
	tv_i2c_write(6,0x04,0x54,0x2c,0x00,0x94,0x00);
	tv_i2c_write(6,0x04,0x58,0x80,0x07,0x58,0x00);
	tv_i2c_write(6,0x04,0x5c,0x05,0x00,0x24,0x00);
	tv_i2c_write(6,0x04,0x60,0x38,0x04,0x04,0x00);
	tv_i2c_write(6,0x04,0x64,0x01,0x00,0x00,0x00);

	printk("\n//Color Bar Setup\n");
	tv_i2c_write(6,0x0a,0x44,0x2c,0x80,0x05,0x80);
	tv_i2c_write(6,0x0a,0x48,0x98,0x08,0x65,0x04);
	tv_i2c_write(6,0x0a,0x4c,0xc0,0x00,0x29,0x00);
	tv_i2c_write(6,0x0a,0x50,0x80,0x07,0x38,0x04);
	tv_i2c_write(6,0x0a,0x64,0x2c,0x80,0x05,0x80);
	tv_i2c_write(6,0x0a,0x68,0x98,0x08,0x65,0x04);
	tv_i2c_write(6,0x0a,0x6c,0xc0,0x00,0x29,0x00);
	tv_i2c_write(6,0x0a,0x70,0x80,0x07,0x38,0x04);

	printk("\n//DP Main Stream Attirbutes\n");
	tv_i2c_write(6,0x06,0x44,0x40,0x08,0x1f,0x00);
	tv_i2c_write(6,0x06,0x48,0x98,0x08,0x65,0x04);
	tv_i2c_write(6,0x06,0x4c,0xc0,0x00,0x29,0x00);
	tv_i2c_write(6,0x06,0x50,0x80,0x07,0x38,0x04);
	tv_i2c_write(6,0x06,0x54,0x2c,0x80,0x05,0x80);

	printk("\n//DP Flow Shape & TimeStamp\n");
	tv_i2c_write(6,0x06,0x58,0x20,0x00,0xbf,0x1e);

	printk("\n//Nvid Setting\n");
	tv_i2c_write(6,0x06,0x14,0xc0,0xa8,0x00,0x00);

	printk("\n//Enable Video and DP link\n");
	tv_i2c_write(6,0x06,0x00,0x41,0x00,0x00,0x00);
	tv_i2c_write(6,0x06,0x00,0x43,0x00,0x00,0x00);

	printk("\n//SYSCTRL\n");
	tv_i2c_write(6,0x05,0x10,0x01,0x00,0x00,0x00);

	printk("\n//Color Bar Start\n");
	tv_i2c_write(6,0x0a,0x60,0x03,0xff,0x00,0x00);
	tv_i2c_write(6,0x0a,0x40,0x02,0xff,0x00,0x00);
	sunxi_lcd_delay_ms(70);
	tv_i2c_write(2,0x06,0x10,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);
	tv_i2c_write(2,0x06,0x14,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);
	tv_i2c_write(2,0x06,0x18,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);
	tv_i2c_write(2,0x05,0x08,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);
	tv_i2c_write(2,0x02,0x20,0x00,0x00,0x00,0x00);
	tv_i2c_read(4);



	bsp_disp_get_panel_info(sel, panel_info);
	sunxi_lcd_dsi_clk_enable(sel);
	disp_sys_free(panel_info);
	return ;
}

static void LCD_panel_exit(u32 sel)
{
	return 0;
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_set_bright(u32 sel, u32 bright)
{

	return 0;
}

__lcd_panel_t lt070me05000_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "lt070me05000",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
		.set_bright = LCD_set_bright,
	},
};
