/**************************************************
** ���ܣ�ͣ����������Ϣ�ɼ�������ܸ���
** Ӳ�������մ�����BH1750��GY-30����TM1650�����
** ���ڣ�2015-5-8
**************************************************/
#include <ioCC2530.h>

#include "LightControl.h"

#define uint unsigned int
#define uchar unsigned char

#define SlaveAddress 0x46

#define SCL P1_2
#define SDA P1_3

uchar ge,shi,bai,qian,wan;  //���ݱ���
uchar Light[5];             //��������
unsigned char tab[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71};  //�����

void Delay_1u(unsigned int times) //��ʱus
{
  while(times--)
  {
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop");
  }
}

void Delay_1ms(unsigned int times)  //��ʱms
{
    unsigned int i;
    for(i=0; i<times; i++)       
        Delay_1u(1000);            
}

void WriteSDA1(void)  //SDA���1,�൱��51�����SDA=1   
{
    P1DIR |= 0x08;
    SDA = 1;
}    
void WriteSDA0(void)  //SDA ���0   
{
    P1DIR |= 0x08;
    SDA = 0;
}  
void WriteSCL1(void)  //SCL ���1   
{
    P1DIR |= 0x04;
    SCL = 1;
}
void WriteSCL0(void)  //SCL ���0   
{
    P1DIR |= 0x04;
    SCL = 0;
}  
void ReadSDA(void)  //��������SDA��ӦIO��DIR���Խ�������   
{
    P1DIR &= 0xF7;
}

/* ��ʼ�ź� */
void START()
{
    WriteSDA1();
    WriteSCL1();
    Delay_1u(5);
    WriteSDA0();
    Delay_1u(5);
    WriteSCL0();
    Delay_1u(5);
}

/* ֹͣ�ź� */
void STOP()
{
    WriteSDA0();
    WriteSCL1();
    Delay_1u(5);
    WriteSDA1();
    Delay_1u(5);
    WriteSCL0();
    Delay_1u(5);
}

/* ����Ӧ���ź�-��ڲ���:ack (0:ACK 1:NAK) */
void SendACK(uchar ack)
{
   if(ack)
   {
       WriteSDA1();
   }
   else
   {
       WriteSDA0();
   }  
     
       Delay_1u(5);
       WriteSCL1();   
       Delay_1u(5);
       WriteSCL0();
}

/* ����Ӧ���ź� */
uchar BH1750_RecvACK()
{        
    //unsigned int i=10;
    ReadSDA();F0=SDA;
    WriteSCL1();
    Delay_1u(5);
    WriteSCL0();
    Delay_1u(5);
    P1DIR |= 0x08;
    return F0;
}

/* ����0����SCLΪ�ߵ�ƽʱʹSDA�ź�Ϊ�� */
void SEND_0_1(void)   /* SEND ACK */
{
    WriteSDA0();
    Delay_1u(5);
    WriteSCL1();
    Delay_1u(5);
    WriteSCL0();
    Delay_1u(5);
}

/* ����1����SCLΪ�ߵ�ƽʱʹSDA�ź�Ϊ�� */
void SEND_1_1(void)
{
    WriteSDA1();
    Delay_1u(5);
    WriteSCL1();
    Delay_1u(5);
    WriteSCL0();
    Delay_1u(5);
}

/* ��IIC���߷���һ���ֽ����� */
void BH1750_SendByte(uchar dat)
{
    char i;
    WriteSCL0();
    for(i=0;i<8;i++)
    {
        if((dat<<i)&0x80)
        {
            SEND_1_1();
        }
        else
        {
            SEND_0_1();
        }
    }
    BH1750_RecvACK();     
}

/* ��IIC���߽���һ���ֽ����� */
uchar BH1750_RecvByte()
{
    char b=0,i;    
    WriteSCL0();
    Delay_1u(5);
    for(i=0;i<8;i++)
    {   
        ReadSDA();
        WriteSCL1();
        Delay_1u(5);
        F0=SDA; //�Ĵ����е�һλ,���ڴ洢SDA�е�һλ����
        if(F0)
        {
            b=b<<1;
            b=b|0x01;
        }
        else
        {b=b<<1;}
        WriteSCL0();
        Delay_1u(5);
    }
    P1DIR |= 0x08;
    return b;
}

uchar Read_BH1750()
{
    uchar t0,t1,t;
    START();
    BH1750_SendByte(SlaveAddress);      //�����豸��ַ+д�ź�
    if(F0)
    {
        START();                        //��ʼ�ź�
        BH1750_SendByte(SlaveAddress);  //�����豸��ַ+д�ź�
    }
    BH1750_SendByte(0x00);              // �ϵ�
    if(F0)
    {
        BH1750_SendByte(0x00);      
    }
    STOP();
    Delay_1ms(30);

    START();                            //��ʼ�ź�
    BH1750_SendByte(SlaveAddress);      //�����豸��ַ+д�ź�
    if(F0)
    {
        START();                        //��ʼ�ź�
        BH1750_SendByte(SlaveAddress);  //�����豸��ַ+д�ź�
    }
    BH1750_SendByte(0x01);              // �ϵ�
    if(F0)
    {
        BH1750_SendByte(0x01);      
    }
    STOP();

    START();                            //��ʼ�ź�
    BH1750_SendByte(SlaveAddress);      //�����豸��ַ+д�ź�
    if(F0)
    {
        START();                        //��ʼ�ź�
        BH1750_SendByte(SlaveAddress);  //�����豸��ַ+д�ź�
    }
    BH1750_SendByte(0x10);              // H- resolution mode
    if(F0)
    {
        BH1750_SendByte(0x10);      
    }
    STOP();
    Delay_1ms(180);
   
    START();                            //��ʼ�ź�
    BH1750_SendByte(SlaveAddress+1);    //�����豸��ַ+���ź�
    if(F0)
    {
        START();                        //��ʼ�ź�
        BH1750_SendByte(SlaveAddress+1);//�����豸��ַ+д�ź�
    }
   
    t0= BH1750_RecvByte();              //BUF[0]�洢0x32��ַ�е�����
    SendACK(0);                         //���һ��������Ҫ��NOACK

    t1= BH1750_RecvByte();
    SendACK(1);                         //��ӦACK
      
    STOP();                             //ֹͣ�ź�
    t=((t0<<8)+t1)/1.2;
    Delay_1ms(5);
    return t;
}

void Single_Write_BH1750(uchar REG_Address)
{
    START();                         //��ʼ�ź�
    BH1750_SendByte(SlaveAddress);   //�����豸��ַ+д�ź�
    BH1750_SendByte(REG_Address);    //�ڲ��Ĵ�����ַ��
    STOP();                          //����ֹͣ�ź�
}

void Init_BH1750()
{
    BH1750_SendByte(0x01);           //��ʼ��
}

/* �ַ�ת�� */
void conversion(uint temp_data)  //����ת���� ����ʮ���٣�ǧ����
{  
  wan=temp_data/10000+0x30 ;
  temp_data=temp_data%10000;     //ȡ������
  qian=temp_data/1000+0x30 ;
  temp_data=temp_data%1000;      //ȡ������
  bai=temp_data/100+0x30   ;
  temp_data=temp_data%100;       //ȡ������
  shi=temp_data/10+0x30    ;
  temp_data=temp_data%10;        //ȡ������
  ge=temp_data+0x30;        
  Light[0]=(unsigned char)wan;
  Light[1]=(unsigned char)qian;
  Light[2]=(unsigned char)bai;
  Light[3]=(unsigned char)shi;
  Light[4]=(unsigned char)ge;
}

/* ������ݲɼ� */
void Brightness()
{
  Delay_1ms(100);
  Init_BH1750();
  float lx;
  lx = Read_BH1750();
  conversion(lx);    
  Delay_1ms(1000);
}

void TM1650_Write(unsigned char	DATA)	//д���ݺ���
{
  unsigned char i;
  Delay_1u(1);
  WriteSCL0();
  for(i=0;i<8;i++)
  {
    if(DATA&0X80)
            WriteSDA1();
    else
            WriteSDA0();
    DATA<<=1;
    WriteSCL0();
    Delay_1u(1);
    WriteSCL1();
    Delay_1u(1);
    WriteSCL0();
    Delay_1u(1);
  }	
}

void Write_DATA(unsigned char add,unsigned char DATA)   //ָ����ַд������
{
  START();
  TM1650_Write(add);
  SendACK(0);
  TM1650_Write(DATA);
  SendACK(0);
  STOP();
}

/* ��������� */
void num_led(uint num[])
{
  Write_DATA(0x48,0x01);
  Write_DATA(0x68+0,tab[(10)%16]|0x80); //��һλ
  Write_DATA(0x68+2,tab[(0)%16]);       //�ڶ�λ
  Write_DATA(0x68+4,tab[(num[0])%16]);  //����λ
  Write_DATA(0x68+6,tab[(num[1])%16]);  //����λ
  Delay_1ms(500);
}