/* 	
 	Использование прерываний.
*/
                  
#include <LPC23xx.H>

#define STB 26 //Port1.26
#define CLK 27 //Port1.27
#define DIO	28 //Port1.28

static unsigned int n=1;

void delay(unsigned int t) {
		//Сбросить таймер
		T0TC = 0x00000000;
		//Установить задержку в мс в регистре совпадения MCR
		T0MR0 = t;
		//Запустить таймер
		T0TCR = 0x00000001;
		//Ожидаем окончания счета
		while (T0TCR&0x1) {};
}

void tm1638_sendbyte(unsigned int x) {

		unsigned int i;
		IODIR1 |= (1<<DIO);//Устанавливаем пин DIO на вывод
		for(i = 0; i < 8; i++)
    {
			IOCLR1=(1<<CLK);//Сигнал CLK устанавливаем в 0
			delay(1);//Задержка 
			if (x&1) 	{IOSET1=(1<<DIO);} //Устанавливаем значение на выходе DIO
			else 			{IOCLR1=(1<<DIO);}
			delay(1);//Задержка
      x  >>= 1;
      IOSET1=(1<<CLK);//Сигнал CLK устанавливаем в 1
      delay(2);			
    }

}

unsigned int tm1638_receivebyte() {

		unsigned int i;
		unsigned int x=0;
		IODIR1 &= ~(1<<DIO);//Устанавливаем пин DIO на ввод
		for(i = 0; i < 32; i++)
    {
			IOCLR1=(1<<CLK);//Сигнал CLK устанавливаем в 0
			delay(1);//Задержка 
			if (IOPIN1&(1<<DIO)) {
				x |= (1<<i);
			}
			delay(1);//Задержка
      IOSET1=(1<<CLK);//Сигнал CLK устанавливаем в 1
      delay(2);			
    }
	return x;
}

void tm1638_sendcmd(unsigned int x)
{
			//Устанавливаем пассивный высокий уровень сигнала STB
			IOSET1=(1<<STB);
			//Устанавливаем пины CLK,DIO,STB на вывод
			IODIR1 = (1<<CLK)|(1<<DIO)|(1<<STB);
			//Устанавливаем активный низкий уровень сигнала STB
			IOCLR1=(1<<STB);
			tm1638_sendbyte(x);
}


void tm1638_setadr(unsigned int adr) {
	   	//Установить адрес регистра LED инидикации
		tm1638_sendcmd(0xC0|adr);	
}

void tm1638_init() {
		unsigned int i;
		//Разрешить работу индикации
		tm1638_sendcmd(0x88);	
		//Установить режим адресации: автоинкремент
		tm1638_sendcmd(0x40);
   	//Установить адрес регистра LED инидикации
		tm1638_setadr(0);
		//Сбросить все 
		for (i=0;i<=0xf;i++)
			tm1638_sendbyte(0);
		//Установить режим адресации: фиксированный
		tm1638_sendcmd(0x44);
}


void Timer0_Init(void){
		//Предделитель таймера = 12000
		T0PR = 12000;
		//Сбросить счетчик и делитель
		T0TCR = 0x00000002;
		//При совпадении останавливаем, сбрасываем таймер
		T0MCR = 0x00000006;
		//Регистр совпадения = 1000 (1 Гц)
		T0MR0 = 1000;
}	

void Timer1_Int (void) __irq	
{
		//n+=2;
		tm1638_sendbyte(0);	//Выключаем светодиоды
	  n+=2;
		if (n > 0xf) n=1; 	//Изменение состояния
		tm1638_setadr(n);  //Устанавливаем новый адрес регистра	
		tm1638_sendbyte(1);//Передать данные
		VICVectAddr = 0; /*Сбросить VIC*/
}

void Timer1_Init(void){
		//Предделитель таймера = 12000
		T1PR = 12000;
		//Сбросить счетчик и делитель
		T1TCR = 0x00000002;
		//При совпадении сбрасываем таймер и вызываем прерывание
		T1MCR = 0x00000003;
		//Регистр совпадения = 1000 (1 Гц)
		T1MR0 = 1000;
		//Записать адрес обработчика прерывания в таблицу векторов
		VICVectAddr5 = (unsigned)Timer1_Int;	
		//Настроить канал обработки
		VICVectCntl5 = 0x00000024;
		//Разрешить прерывания							 
		VICIntEnable |= 0x00000020;	
		//Запустить таймер
		T1TCR = 0x00000001;
}

int main (void) {

  Timer0_Init(); /* Настроить таймер 0*/	
	tm1638_init();/* Конфигурируем TM1638 */	
  Timer1_Init(); /* Настроить таймер 1*/	
	while (1); /*Бесконечный цикл*/
}
