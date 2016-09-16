#include <LPC23xx.H> 

/* This number applies to all FULLCAN IDs, explicit STD IDs, group STD IDs, 
explicit EXT IDs, and group EXT IDs. */ 
#define ACCF_IDEN_NUM			4

#define CAN_MEM_BASE		0xE0038000

/* Identifiers for FULLCAN, EXP STD, GRP STD, EXP EXT, GRP EXT */
#define FULLCAN_ID				0x100
#define EXP_STD_ID				0x100
#define GRP_STD_ID				0x200
#define EXP_EXT_ID				0x100000
#define GRP_EXT_ID				0x200000

/* Mode desription */
#define MODE_SELFTEST				0x00000004
#define MODE_NORMAL					0x00000000 


unsigned tmp;

void CAN1IRQ (void)	__irq 
{
		tmp 				= 	CAN1RDA;							//Сохранить данные
		CAN1CMR 			=	0x00000004; 					//Освободить входной буфер
        if(CAN1SR & 0x00000004)					//Если Tx Buffer 1 свободен
        {
            CAN1TFI1 		= 		0x80040000;			//Установить DLC = 4 байта 	
            CAN1TID1 		= 		EXP_EXT_ID;			//Установить расширенный формат
            CAN1TDA1		=			tmp+1;	
          //  CAN1CMR 		= 		0x00000021;			//Передать данные через буфер 1
        }
		VICVectAddr 		= 	0x00000000;						//Очистить флаг прерывания
}

void CAN_Set_Filter(void)
{
	unsigned address = 0;	
	unsigned i;
	unsigned ID_high, ID_low;

	// Set explicit standard Frame  
	CAN_SFF_SA = address;
	for ( i = 0; i < ACCF_IDEN_NUM; i += 2 )
	{
			ID_low = (i << 29) | (EXP_STD_ID << 16);
			ID_high = ((i+1) << 13) | (EXP_STD_ID << 0);
			*((volatile unsigned *)(CAN_MEM_BASE + address)) = ID_low | ID_high;
			address += 4; 
	}
			
	// Set group standard Frame 
	CAN_SFF_GRP_SA = address;
	for ( i = 0; i < ACCF_IDEN_NUM; i += 2 )
	{
			ID_low = (i << 29) | (GRP_STD_ID << 16);
			ID_high = ((i+1) << 13) | (GRP_STD_ID << 0);
			*((volatile unsigned *)(CAN_MEM_BASE + address)) = ID_low | ID_high;
			address += 4; 
	}
	 
	// Set explicit extended Frame 
	CAN_EFF_SA = address;
	for ( i = 0; i < ACCF_IDEN_NUM; i++  )
	{
			ID_low = (i << 29) | (EXP_EXT_ID << 0);
			*((volatile unsigned *)(CAN_MEM_BASE + address)) = ID_low;
			address += 4; 
	}

	// Set group extended Frame 
	CAN_EFF_GRP_SA = address;
	for ( i = 0; i < ACCF_IDEN_NUM; i++  )
	{
			ID_low = (i << 29) | (GRP_EXT_ID << 0);
			*((volatile unsigned *)(CAN_MEM_BASE + address)) = ID_low;
			address += 4; 
	}
		 
	// Set End of Table 
	CAN_EOT = address;
}

void CAN2_Init(unsigned MOD)
{
	PCONP |=  1<<14;                          //Включить питание модуля CAN2
	PINSEL0 |= 0x00000A00; //Включить альтернативные функции Port0.0(CAN2.RD) и Port0.1 (CAN2.TD) (значение 01)
	CAN2MOD = 0x00000001; //Сброс CAN контроллера
	CAN2GSR = 0;	// Сброс счетчика ошибок
	CAN2BTR |= 0x00000096; //Установить делитель частоты CAN для передачи на скорость 100Khz (скорость зависит от TSEG1,TSEG2)
	CAN_AFMR =	0x00000001; //Запретить работу фильтра для установки параметров и адресов

	CAN_Set_Filter();
	CAN_AFMR = 0x00000000;			//Enable all mesages
	CAN2MOD = MOD;			//Устрановить режим
}

void CAN1_Init(unsigned MOD)
{
	PCONP |=  1<<13;                          //Включить питание модуля CAN2
	PINSEL0 |= 0x00000005; //Включить альтернативные функции Port0.0(CAN2.RD) и Port0.1 (CAN2.TD) (значение 01)
	CAN1MOD = 0x00000001; //Сброс CAN контроллера
	CAN1GSR = 0;	// Сброс счетчика ошибок
	CAN1BTR |= 0x00000096; //Установить делитель частоты CAN для передачи на скорость 100Khz (скорость зависит от TSEG1,TSEG2)
	CAN1IER				=	0x00000001;	//РАзрешить прерывание при приеме 
	CAN1MOD = MOD;			//Устрановить режим 
  VICVectAddr23 = (unsigned)CAN1IRQ;	//Записать адрес обработчика прерывания в таблицу векторов
  VICIntEnable |= 0x00800000;	//Разрешить прерывания


}

int main(void)
{
	

	//Тестирование приемо-передатчика
	CAN1_Init(MODE_NORMAL);	
	CAN2_Init(MODE_NORMAL);	


	if(CAN2SR & 0x00000004)					//Проверка буфера CAN2.TX
					{
	 CAN2TFI1 = 0x00040000; //Установить длину DLC = 4 байта 	
	 CAN2TID1 = EXP_STD_ID; //Установка адреса стандартного фрейма 11 бит
	 CAN2TDA1 = 0xABCDEF01; //Записать данные для передачи
	 CAN2CMR = 0x00000030;	//Передать данные
					}
			while(!(CAN1GSR & 0x1));
			tmp 				= 	CAN1RDA; //Прочитать полученные данные и очистить почтовый ящик
			if (tmp != 0xABCDEF01) return 0; //Если возникла ошибка - останов программы!
    CAN2CMR 			=	0x00000004; //Очистить входной буфер
    CAN2MOD 	= 		0x00000001;	 //Сброс модуля CAN
    CAN2MOD 	= 		0x00000000;	 //Выключить контроллер CAN
		PCONP   &=  ~(1<<14);//Отключить питание CAN2
}


