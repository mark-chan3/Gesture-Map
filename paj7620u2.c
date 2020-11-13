#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include "paj7620u2.h"
#include "paj7620u2_cfg.h"


//led初始化
void LED_init(void)
{
	int ret1 = 1;
	int ret2 = 1;
	
	ret1 = gpio_request(LED_sleep_IO, "for class");
	ret2 = gpio_request(LED_gesture_IO, "for class");

	if (ret1 < 0 || ret2 < 0)
		printk("request io error");
	
	gpio_direction_output(LED_sleep_IO, 1);
	gpio_direction_output(LED_gesture_IO, 1);
}

//gpio释放
void GS_IO_release(void)
{
	gpio_free(SDA_IO);
	gpio_free(SCL_IO);
	gpio_free(LED_sleep_IO);
	gpio_free(LED_gesture_IO);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////i2c操作


//PAJ2670 I2C初始化
void GS_i2c_init(void)
{
	int ret1 = 1;
	int ret2 = 1;
	
	ret1 = gpio_request(SCL_IO, "for class");
	ret2 = gpio_request(SDA_IO, "for class");

	if (ret1 < 0 || ret2 < 0)
		printk("request io error");
	
	gpio_direction_output(SCL_IO, 1);
	gpio_direction_output(SDA_IO, 1);
	
}

//产生IIC起始信号
static void GS_IIC_Start(void)
{
	GS_SDA_OUT();//sda线输出
	GS_IIC_SDA_UP;	  	  
	GS_IIC_SCL_UP;
	udelay(4);
 	GS_IIC_SDA_DOWN;//START:when CLK is high,DATA change form high to low 
	udelay(4);
	GS_IIC_SCL_DOWN;//钳住I2C总线，准备发送或接收数据 
	udelay(4);
}

//产生IIC停止信号
static void GS_IIC_Stop(void)
{
	GS_SDA_OUT();//sda线输出
	GS_IIC_SCL_DOWN;
	GS_IIC_SDA_DOWN;//STOP:when CLK is high DATA change form low to high
 	udelay(4);
	GS_IIC_SCL_UP;
	GS_IIC_SDA_UP;//发送I2C总线结束信号
	udelay(4);							   	
}

//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
static u8 GS_IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	GS_SDA_IN();  //SDA设置为输入  
	GS_IIC_SDA_UP;
	udelay(4);	   
	GS_IIC_SCL_UP;
	udelay(4); 
	while(gpio_get_value(SDA_IO))
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			printk("\n No Ack \n");
			GS_IIC_Stop();
			return 1;
		}
	}
	GS_IIC_SCL_DOWN;//时钟输出0 	   
	return 0;  
}

//产生ACK应答
static void GS_IIC_Ack(void)
{
	GS_IIC_SCL_DOWN;
	GS_SDA_OUT();
	GS_IIC_SDA_DOWN;
	udelay(4);	   
	GS_IIC_SCL_UP;
	udelay(4); 
	GS_IIC_SCL_DOWN;
}

//不产生ACK应答		    
static void GS_IIC_NAck(void)
{
	GS_IIC_SCL_DOWN;
	GS_SDA_OUT();
	GS_IIC_SDA_UP;
	udelay(4);	   
	GS_IIC_SCL_UP;
	udelay(4); 
	GS_IIC_SCL_DOWN;
}

//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
static void GS_IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
	GS_SDA_OUT(); 	    
    GS_IIC_SCL_DOWN;//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {              
		if((txd&0x80)>>7){
			GS_IIC_SDA_UP;
		}
		else
			GS_IIC_SDA_DOWN;
		txd<<=1; 	  
		udelay(5);
		GS_IIC_SCL_UP;
		udelay(5);
		GS_IIC_SCL_DOWN;
		udelay(5);
    }	 
} 

//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
static u8 GS_IIC_Read_Byte(u8 ack)
{
	u8 i,receive=0;
	GS_SDA_IN();//SDA设置为输入
	for(i=0;i<8;i++ )
	{
		GS_IIC_SCL_DOWN;
		udelay(4);
		GS_IIC_SCL_UP;
		receive<<=1;
		if(gpio_get_value(SDA_IO))
			receive++;   
	 	udelay(4);
	}					 
	if (!ack)
		GS_IIC_NAck();//发送nACK
	else
		GS_IIC_Ack(); //发送ACK   
	return receive;
}

//PAJ7620U2写一个字节数据
u8 GS_Write_Byte(u8 REG_Address,u8 REG_data)
{
	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID);
	if(GS_IIC_Wait_Ack())
	{
		GS_IIC_Stop();//释放总线
		return 1;//没应答则退出

	}
	GS_IIC_Send_Byte(REG_Address);
	GS_IIC_Wait_Ack();	
	GS_IIC_Send_Byte(REG_data);
	GS_IIC_Wait_Ack();	
	GS_IIC_Stop();

	return 0;
}

//PAJ7620U2读一个字节数据
u8 GS_Read_Byte(u8 REG_Address)
{
	u8 REG_data;
	
	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID);//发写命令
	if(GS_IIC_Wait_Ack())
	{
		 GS_IIC_Stop();//释放总线
		 return 0;//没应答则退出
	}		
	GS_IIC_Send_Byte(REG_Address);
	GS_IIC_Wait_Ack();
	GS_IIC_Start(); 
	GS_IIC_Send_Byte(PAJ7620_ID|0x01);//发读命令
	GS_IIC_Wait_Ack();
	REG_data = GS_IIC_Read_Byte(0);
	GS_IIC_Stop();

	return REG_data;
}
//PAJ7620U2读n个字节数据
u8 GS_Read_nByte(u8 REG_Address,u16 len,u8 *buf)
{
	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID);//发写命令
	if(GS_IIC_Wait_Ack()) 
	{
		GS_IIC_Stop();//释放总线
		return 1;//没应答则退出
	}
	GS_IIC_Send_Byte(REG_Address);
	GS_IIC_Wait_Ack();

	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID|0x01);//发读命令
	GS_IIC_Wait_Ack();
	while(len)
	{
		if(len==1)
		{
			*buf = GS_IIC_Read_Byte(0);
		}
		else
		{
			*buf = GS_IIC_Read_Byte(1);
		}
		buf++;
		len--;
	}
	GS_IIC_Stop();//释放总线

	return 0;
	
}
//PAJ7620唤醒
void GS_WakeUp(void)
{
	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID);//发写命令
	GS_IIC_Stop();//释放总线
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////i2c操作

//选择PAJ7620U2 BANK区域
void paj7620u2_selectBank(bank_e bank)
{
	switch(bank)
	{
		case BANK0: GS_Write_Byte(PAJ_REGITER_BANK_SEL,PAJ_BANK0);break;//BANK0寄存器区域
		case BANK1: GS_Write_Byte(PAJ_REGITER_BANK_SEL,PAJ_BANK1);break;//BANK1寄存器区域
	}	
}

//PAJ7620U2唤醒
u8 paj7620u2_wakeup(void)
{ 
	u8 data=0x0a;
	GS_WakeUp();//唤醒PAJ7620U2
	mdelay(5);//唤醒时间>400us
	GS_WakeUp();//唤醒PAJ7620U2
	mdelay(5);//唤醒时间>400us
	paj7620u2_selectBank(BANK0);//进入BANK0寄存器区域
	data = GS_Read_Byte(0x00);//读取状态
	if(data!=0x20) return 0; //唤醒失败
	return 1;
}


//手势识别测试
u16 Gesture_test(void)
{
  	u8 status=1;
	u8 data[2]={0x00};
	u16 gesture_data;
	while(1)
	{	
    	status = GS_Read_nByte(PAJ_GET_INT_FLAG1,2,&data[0]);//读取手势状态			
		if(!status)
		{   
			gesture_data =(u16)data[1]<<8 | data[0];
			return gesture_data;
		}
		mdelay(50);
	}
}

static int paj7620driver_open(struct inode *inode, struct file *file)
{
	
	return nonseekable_open(inode, file);
}

static ssize_t paj7620driver_read(struct file *file, char __user *buf, size_t count, loff_t *ptr)
{
	//printk("enter paj7620driver_read successful \n");
	u16 result = Gesture_test();
	if (result) printk("result= %x \n\n",result);
	copy_to_user(buf, &result, 1);
	return 1;
}

static ssize_t paj7620driver_write(struct file *file, char __user *buf, size_t count, loff_t *ptr)
{
	switch (buf[0])
	{
		case '1':	{gpio_set_value(LED_sleep_IO, 1);  break;}//绿灯关
		case '2':	{gpio_set_value(LED_sleep_IO, 0);  break;}//绿灯开
		case '3':	{gpio_set_value(LED_gesture_IO, 1);  break;}//蓝灯关
		case '4':	{gpio_set_value(LED_gesture_IO, 0);  break;}//蓝灯开

		
	}
	return 1;
}

static const struct file_operations paj7620driver_fops = {
	.owner    = THIS_MODULE,
	.open     = paj7620driver_open,
	.read     = paj7620driver_read,
	.write    = paj7620driver_write, 
};

static struct miscdevice paj7620driver_miscdev = {
	TEMP_MINOR,
	"paj7620",
	&paj7620driver_fops
};


//PAJ7620U2初始化
//返回值：0:失败 1:成功
static u8 __init paj7620u2_init(void)
{
	u8 i;
	u8 status;
	int ret;

	LED_init();//LED控制端口初始化
	
	GS_i2c_init();//I2C初始化
	
    status = paj7620u2_wakeup();//唤醒PAJ7620U2
    if(status < 0) 
		return 0;
	
	paj7620u2_selectBank(BANK0);//进入BANK0寄存器区域
	for(i=0;i<INIT_SIZE;i++)
	{
		GS_Write_Byte(init_Array[i][0],init_Array[i][1]);//初始化PAJ7620U2
	}
	for(i=0;i<GESTURE_SIZE;i++)
	{
		GS_Write_Byte(gesture_arry[i][0],gesture_arry[i][1]);//手势识别模式初始化
	}
  	paj7620u2_selectBank(BANK0);//切换回BANK0寄存器区域

	ret = misc_register(&paj7620driver_miscdev);//注册杂项设备
	if (ret < 0)
		return 0;
	
	return 1;
}

static void __exit paj7620u2_exit(void)
{
	GS_IO_release();
	misc_deregister(&paj7620driver_miscdev);
}


module_init(paj7620u2_init);
module_exit(paj7620u2_exit);

MODULE_LICENSE("GPL");

