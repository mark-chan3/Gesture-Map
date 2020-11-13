#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include "paj7620u2.h"
#include "paj7620u2_cfg.h"


//led��ʼ��
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

//gpio�ͷ�
void GS_IO_release(void)
{
	gpio_free(SDA_IO);
	gpio_free(SCL_IO);
	gpio_free(LED_sleep_IO);
	gpio_free(LED_gesture_IO);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////i2c����


//PAJ2670 I2C��ʼ��
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

//����IIC��ʼ�ź�
static void GS_IIC_Start(void)
{
	GS_SDA_OUT();//sda�����
	GS_IIC_SDA_UP;	  	  
	GS_IIC_SCL_UP;
	udelay(4);
 	GS_IIC_SDA_DOWN;//START:when CLK is high,DATA change form high to low 
	udelay(4);
	GS_IIC_SCL_DOWN;//ǯסI2C���ߣ�׼�����ͻ�������� 
	udelay(4);
}

//����IICֹͣ�ź�
static void GS_IIC_Stop(void)
{
	GS_SDA_OUT();//sda�����
	GS_IIC_SCL_DOWN;
	GS_IIC_SDA_DOWN;//STOP:when CLK is high DATA change form low to high
 	udelay(4);
	GS_IIC_SCL_UP;
	GS_IIC_SDA_UP;//����I2C���߽����ź�
	udelay(4);							   	
}

//�ȴ�Ӧ���źŵ���
//����ֵ��1������Ӧ��ʧ��
//        0������Ӧ��ɹ�
static u8 GS_IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	GS_SDA_IN();  //SDA����Ϊ����  
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
	GS_IIC_SCL_DOWN;//ʱ�����0 	   
	return 0;  
}

//����ACKӦ��
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

//������ACKӦ��		    
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

//IIC����һ���ֽ�
//���شӻ�����Ӧ��
//1����Ӧ��
//0����Ӧ��			  
static void GS_IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
	GS_SDA_OUT(); 	    
    GS_IIC_SCL_DOWN;//����ʱ�ӿ�ʼ���ݴ���
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

//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
static u8 GS_IIC_Read_Byte(u8 ack)
{
	u8 i,receive=0;
	GS_SDA_IN();//SDA����Ϊ����
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
		GS_IIC_NAck();//����nACK
	else
		GS_IIC_Ack(); //����ACK   
	return receive;
}

//PAJ7620U2дһ���ֽ�����
u8 GS_Write_Byte(u8 REG_Address,u8 REG_data)
{
	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID);
	if(GS_IIC_Wait_Ack())
	{
		GS_IIC_Stop();//�ͷ�����
		return 1;//ûӦ�����˳�

	}
	GS_IIC_Send_Byte(REG_Address);
	GS_IIC_Wait_Ack();	
	GS_IIC_Send_Byte(REG_data);
	GS_IIC_Wait_Ack();	
	GS_IIC_Stop();

	return 0;
}

//PAJ7620U2��һ���ֽ�����
u8 GS_Read_Byte(u8 REG_Address)
{
	u8 REG_data;
	
	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID);//��д����
	if(GS_IIC_Wait_Ack())
	{
		 GS_IIC_Stop();//�ͷ�����
		 return 0;//ûӦ�����˳�
	}		
	GS_IIC_Send_Byte(REG_Address);
	GS_IIC_Wait_Ack();
	GS_IIC_Start(); 
	GS_IIC_Send_Byte(PAJ7620_ID|0x01);//��������
	GS_IIC_Wait_Ack();
	REG_data = GS_IIC_Read_Byte(0);
	GS_IIC_Stop();

	return REG_data;
}
//PAJ7620U2��n���ֽ�����
u8 GS_Read_nByte(u8 REG_Address,u16 len,u8 *buf)
{
	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID);//��д����
	if(GS_IIC_Wait_Ack()) 
	{
		GS_IIC_Stop();//�ͷ�����
		return 1;//ûӦ�����˳�
	}
	GS_IIC_Send_Byte(REG_Address);
	GS_IIC_Wait_Ack();

	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID|0x01);//��������
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
	GS_IIC_Stop();//�ͷ�����

	return 0;
	
}
//PAJ7620����
void GS_WakeUp(void)
{
	GS_IIC_Start();
	GS_IIC_Send_Byte(PAJ7620_ID);//��д����
	GS_IIC_Stop();//�ͷ�����
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////i2c����

//ѡ��PAJ7620U2 BANK����
void paj7620u2_selectBank(bank_e bank)
{
	switch(bank)
	{
		case BANK0: GS_Write_Byte(PAJ_REGITER_BANK_SEL,PAJ_BANK0);break;//BANK0�Ĵ�������
		case BANK1: GS_Write_Byte(PAJ_REGITER_BANK_SEL,PAJ_BANK1);break;//BANK1�Ĵ�������
	}	
}

//PAJ7620U2����
u8 paj7620u2_wakeup(void)
{ 
	u8 data=0x0a;
	GS_WakeUp();//����PAJ7620U2
	mdelay(5);//����ʱ��>400us
	GS_WakeUp();//����PAJ7620U2
	mdelay(5);//����ʱ��>400us
	paj7620u2_selectBank(BANK0);//����BANK0�Ĵ�������
	data = GS_Read_Byte(0x00);//��ȡ״̬
	if(data!=0x20) return 0; //����ʧ��
	return 1;
}


//����ʶ�����
u16 Gesture_test(void)
{
  	u8 status=1;
	u8 data[2]={0x00};
	u16 gesture_data;
	while(1)
	{	
    	status = GS_Read_nByte(PAJ_GET_INT_FLAG1,2,&data[0]);//��ȡ����״̬			
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
		case '1':	{gpio_set_value(LED_sleep_IO, 1);  break;}//�̵ƹ�
		case '2':	{gpio_set_value(LED_sleep_IO, 0);  break;}//�̵ƿ�
		case '3':	{gpio_set_value(LED_gesture_IO, 1);  break;}//���ƹ�
		case '4':	{gpio_set_value(LED_gesture_IO, 0);  break;}//���ƿ�

		
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


//PAJ7620U2��ʼ��
//����ֵ��0:ʧ�� 1:�ɹ�
static u8 __init paj7620u2_init(void)
{
	u8 i;
	u8 status;
	int ret;

	LED_init();//LED���ƶ˿ڳ�ʼ��
	
	GS_i2c_init();//I2C��ʼ��
	
    status = paj7620u2_wakeup();//����PAJ7620U2
    if(status < 0) 
		return 0;
	
	paj7620u2_selectBank(BANK0);//����BANK0�Ĵ�������
	for(i=0;i<INIT_SIZE;i++)
	{
		GS_Write_Byte(init_Array[i][0],init_Array[i][1]);//��ʼ��PAJ7620U2
	}
	for(i=0;i<GESTURE_SIZE;i++)
	{
		GS_Write_Byte(gesture_arry[i][0],gesture_arry[i][1]);//����ʶ��ģʽ��ʼ��
	}
  	paj7620u2_selectBank(BANK0);//�л���BANK0�Ĵ�������

	ret = misc_register(&paj7620driver_miscdev);//ע�������豸
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

