#ifndef __PAJ7620U2_IIC_H
#define __PAJ7620U2_IIC_H


#define SCL_IO 63
#define SDA_IO 62
#define LED_sleep_IO 61
#define LED_gesture_IO 60

#define GS_SDA_IN()  {gpio_direction_input(SDA_IO);}	//����ģʽ
#define GS_SDA_OUT() {gpio_direction_output(SDA_IO, 1);}    //���ģʽ

//IO��������	 
#define GS_IIC_SCL_UP      {gpio_set_value(SCL_IO, 1);} 		//SCL����
#define GS_IIC_SCL_DOWN    {gpio_set_value(SCL_IO, 0);} 		//SCL����
#define GS_IIC_SDA_UP      {gpio_set_value(SDA_IO, 1);} 		//SDA����
#define GS_IIC_SDA_DOWN    {gpio_set_value(SDA_IO, 0);} 		//SDA����


u8 GS_Write_Byte(u8 REG_Address,u8 REG_data);
u8 GS_Read_Byte(u8 REG_Address);
u8 GS_Read_nByte(u8 REG_Address,u16 len,u8 *buf);
void GS_i2c_init(void);
void GS_WakeUp(void);

#endif


