#include "soft_i2c.h"
#include "board_config.h"
#include "delay.h"

/* ==========================================================================
 *  软件 I2C 主机
 *
 *  时序：每个位约 2us 高 + 2us 低 → 约 250kHz 的 SCL。
 *  如果发现 OLED 显示乱码/花屏，先把 I2C_DELAY_US 调大到 4~5 降低速率，
 *  排除是时序太快导致的（排线长、没有上拉、劣质模块都会导致）。
 *  正常 100~400kHz 的 SSD1306 都能吃下。
 * ========================================================================== */

#define I2C_DELAY_US   2

static void i2c_delay(void)
{
    Delay_Us(I2C_DELAY_US);
}

/* 引脚定义见 board_config.h（I2C_SCL / I2C_SDA 是 sbit） */
#define SCL_HIGH()   (I2C_SCL = 1)
#define SCL_LOW()    (I2C_SCL = 0)
#define SDA_HIGH()   (I2C_SDA = 1)   /* 准双向口写 1 = 释放总线 */
#define SDA_LOW()    (I2C_SDA = 0)
#define SDA_READ()   (I2C_SDA)       /* 读引脚电平 */

void I2c_Init(void)
{
    SDA_HIGH();
    SCL_HIGH();
    i2c_delay();
}

static void i2c_start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    i2c_delay();
    SDA_LOW();          /* SCL 为高时 SDA 下降沿 = START */
    i2c_delay();
    SCL_LOW();
    i2c_delay();
}

static void i2c_stop(void)
{
    SDA_LOW();
    i2c_delay();
    SCL_HIGH();
    i2c_delay();
    SDA_HIGH();         /* SCL 为高时 SDA 上升沿 = STOP */
    i2c_delay();
}

static uint8 i2c_write_byte(uint8 dat)
{
    uint8 i;
    uint8 ack;

    for (i = 0; i < 8; i++) {
        if (dat & 0x80) {
            SDA_HIGH();
        } else {
            SDA_LOW();
        }
        dat = (uint8)(dat << 1);
        i2c_delay();
        SCL_HIGH();
        i2c_delay();
        SCL_LOW();
        i2c_delay();
    }

    SDA_HIGH();         /* 释放 SDA，等从机拉低应答 */
    i2c_delay();
    SCL_HIGH();
    i2c_delay();
    ack = SDA_READ();   /* 0 = 应答 */
    SCL_LOW();
    i2c_delay();

    return ack;
}

uint8 I2c_StartWrite(uint8 dev_addr)
{
    i2c_start();
    return i2c_write_byte(dev_addr);
}

uint8 I2c_WriteByte(uint8 dat)
{
    return i2c_write_byte(dat);
}

void I2c_Stop(void)
{
    i2c_stop();
}
