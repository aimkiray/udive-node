#include <reg52.h>

bit flagXs = 0;
bit flagPkg = 0;         //接收包标志
unsigned char T0RH = 0;
unsigned char T0RL = 0;
unsigned char RxdByte = 0;  //串口接收到的字节
unsigned char RxdArray[8];  //串口接收到的数据包
//数据包传输格式，head，数据长度，发送端口，接收端口
unsigned char send_data[9] = {0xfe, 0x06, 0xa0, 0xa0, 0x01, 0x00, 0x00, 0x00, 0xff};

void ConfigTimer0(unsigned int ms);
void ConfigUART(unsigned int baud);
extern bit Start18B20();
extern bit Get18B20Temp(int *temp);

void main()
{
    bit res;
    int temp;        //当前温度值
	int intT, decT;
	int i;

    EA = 1;
    ConfigTimer0(50);
    Start18B20();
    ConfigUART(9600);
    
    while (1)
    {
		//每秒更新一次温度，或收到上位机请求
        if (flagXs || flagPkg) {
            flagXs = 0;
			flagPkg = 0;
			//读取当前温度
            res = Get18B20Temp(&temp);
			//读取成功
            if (res) {
				//温度值整数部分
				intT = temp >> 4;
				//温度值小数部分
                decT = temp & 0xF;
				send_data[6] = (unsigned char)intT;
				send_data[7] = (unsigned char)decT;
				for (i=0; i<9; i++) {
					SBUF = send_data[i];
					while(TI == 0);
				}
            }
			//重启下一次转换
            Start18B20();
        }
    }
}

void ConfigTimer0(unsigned int ms)
{
    unsigned long tmp;
    
    tmp = 11059200 / 12;
    tmp = (tmp * ms) / 1000;
    tmp = 65536 - tmp;
    tmp = tmp + 12;
    T0RH = (unsigned char)(tmp>>8);
    T0RL = (unsigned char)tmp;
    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = T0RH;
    TL0 = T0RL;
    ET0 = 1;
    TR0 = 1;
}

void ConfigUART(unsigned int baud)
{
    SCON  = 0x50;  //配置串口为模式1
    TMOD &= 0x0F;  //清零T1的控制位
    TMOD |= 0x20;  //配置T1为模式2
    TH1 = 256 - (11059200/12/32)/baud;  //计算T1重载值
    TL1 = TH1;     //初值等于重载值
    ET1 = 0;       //禁止T1中断
    ES  = 1;       //使能串口中断
    TR1 = 1;       //启动T1
}

void InterruptTimer0() interrupt 1
{
    static unsigned char tmrXs = 0;
    
    TH0 = T0RH;
    TL0 = T0RL;
    tmrXs++;
    if (tmrXs >= 100)
    {
        tmrXs = 0;
        flagXs = 1;
    }
}

void InterruptUART() interrupt 4
{
	unsigned char RxCount = 0;
	unsigned char readPkg = 0;
	//接收到字节
    if (RI) {
		RI = 0;
		//RxdByte = SBUF;
		//if (RxdByte == 0xfe) {
		//	readPkg = 1;
		//}
		//if (readPkg && RxCount < 8) {
		//	RxdArray[RxCount] = RxdByte;
		//	RxCount++;
		//} else {
		//	RxCount = 0;
		//	readPkg = 0;
		//	flagPkg = 1;
		//}
		//if (RxdByte == 0xff) {
		//	flagPkg = 1;
		//	RxdByte = 0;
		//	RxCount = 0;
		//}
    }
	//字节发送完毕
    if (TI) {
        TI = 0;
    }
}
