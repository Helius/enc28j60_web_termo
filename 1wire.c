#include <avr/io.h>
#include "type.h"
#include "1wire.h"


//******************************************************************************
//  Функция делающая задержку от 4 до 65000 тактов
//******************************************************************************
void __delay_cycles(WORD __count)
{
        __count >>= 2;
       __asm__ volatile (
		"1: sbiw %0,1" "\n\t"
		"brne 1b"
		: "=w" (__count)
		: "0" (__count)
        );
}


//******************************************************************************
//  Функция подсчёта CRC для полинома {10011001} (протокол iButton)
//******************************************************************************
BYTE OneWire_CRC_calc(BYTE *data, BYTE len)
{//uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data)
BYTE j, i, crc = 0;

    for(j = 0; j < len; j ++)
    {
        crc = crc ^ data[j];
        for (i = 0; i < 8; i++)
        {
            if (crc & 0x01)
            crc = (crc >> 1) ^ 0x8C;
            else
            crc >>= 1;
        }
    }
return crc;
}

//*****************************************************************************
// Формирует reset на шине, проверяет присутствие устройства
// возвращает
// 0 - устройство инициализировано,
// 1 - нет импульса preset
// 2 - шина просажена и не возвращается в HIGH
BYTE OneWire_ResetDevice(void)
{
  BYTE tcnt;
  if(!(PIN_TM & (1 << TM))) return 2; // шина просажена

  // импульс сброса
  SETBIT(DDR_TM,  TM);                    // выход
  CLRBIT(PORT_TM, TM);                    // низкий уровень
  __delay_cycles(clkMhz * 500);           // 500 мкс низкого уровня
  CLRBIT(DDR_TM,  TM);                    // вход
  SETBIT(PORT_TM, TM);                    // c подтяжкой

  // детектирования импульса присутствия
  tcnt = 0;
  __delay_cycles(clkMhz * 10);
  while(PIN_TM & (1 << TM))
  {
  __delay_cycles(clkMhz * 10);
  tcnt ++;
  // если нет - выходим
  if(tcnt > 10) return 1;
  }

  // детектирования готовности (возвращение линии в HIGH)
  tcnt = 0;
  __delay_cycles(clkMhz * 10);
  while(!(PIN_TM & (1 << TM)))
  {
  __delay_cycles(clkMhz * 20);
  tcnt ++;
  // если линия все еще в низком уровне - гдето КЗ
  if(tcnt > 15) return 2;
  }
  return 0;
}



//*******************************************************************************
// пишет байт в шину (команду)
void OneWire_WriteByte(BYTE Data)
{
    BYTE tcnt;
  __delay_cycles(clkMhz * 20);

  for(tcnt = 0; tcnt < 8; tcnt ++)
  {
    if(!(Data & (1 << tcnt)))
    {
    // если 0 сформировать задний фронт + 60 мкс + передний фронт
      SETBIT(DDR_TM,  TM);                    // выход
      CLRBIT(PORT_TM, TM);                    // низкий уровень
      __delay_cycles(clkMhz * 45);
      SETBIT(PORT_TM, TM);                    // высокий уровень
    }
    else
    {
    // если 1 сформировать задний фронт + 15 мкс + передний фронт + 45 мкс
      SETBIT(DDR_TM,  TM);                    // выход
      CLRBIT(PORT_TM, TM);                    // низкий уровень
      __delay_cycles(clkMhz * 10);
      SETBIT(PORT_TM, TM);                    // высокий уровень
      __delay_cycles(clkMhz * 45);
    }
  __delay_cycles(clkMhz * 20);
  }
}



//****************************************************************************
// читает len байт в буфер, возвращает
void OneWire_ReadData(BYTE * Data, BYTE len)
{
    BYTE i, temp, tcnt;
  // чтение данных
  for(i = 0; i < len; i++) // байтовый цикл
  {
    temp = 0;

    for(tcnt = 0; tcnt < 8; tcnt ++)          // битовый цикл
    {
    // даем строб 5 мкс
      SETBIT(DDR_TM,  TM);                    // выход
      CLRBIT(PORT_TM, TM);                    // низкий уровень
      __delay_cycles(clkMhz * 5);
    // переключаемся на вход с подтяжкой
      CLRBIT(DDR_TM,  TM);                    // вход
      SETBIT(PORT_TM, TM);                    // подтяжка
    // ждем 15 мкс
      __delay_cycles(clkMhz * 15);
    // читаем данные
      temp >>=1;                              // сдвигаем рег.данных
      if(PIN_TM & (1 << TM)) temp |= (1 << 7);// 1
    // продолжаем подтягивать
    __delay_cycles(clkMhz * 45);
    }
    Data[i] = temp;                           // сохраняем прочитаный результат
  }
}




//
//
//
//
//
//
//
////******************************************************************************
////          Функция чтения ключа iButton
//// input:
//// *code - указатель на буфер для ключа 8 байт
//// TM - пин регистра ввода вывода откуда производится чтение
//// output:
//// 0 - ок;
//// 1 - нет импульса присутствия;
//// 2 - ненормальное состояние линии (КЗ?)
//// 3 - ошибка CRC
////******************************************************************************
//BYTE ReadKey(BYTE *code, BYTE TM)
//{
//BYTE tcnt, Data, i;
//
//  // импульс сброса
//  SETBIT(DDR_TM,  TM);                    // выход
//  CLRBIT(PORT_TM, TM);                    // низкий уровень
//  __delay_cycles(clkMhz * 500);           // 500 мкс низкого уровня
//  CLRBIT(DDR_TM,  TM);                    // вход
//  SETBIT(PORT_TM, TM);                    // c подтяжкой
//
//  // детектирования импульса присутствия
//  tcnt = 0;
//  __delay_cycles(clkMhz * 10);
//  while(PIN_TM & (1 << TM))
//  {
//  __delay_cycles(clkMhz * 10);
//  tcnt ++;
//  // если нет - выходим
//  if(tcnt > 10) return 1;
//  }
//
//  // детектирования готовности ключа
//  tcnt = 0;
//  __delay_cycles(clkMhz * 10);
//  while(!(PIN_TM & (1 << TM)))
//  {
//  __delay_cycles(clkMhz * 20);
//  tcnt ++;
//  // если линия все еще в низком уровне - гдето КЗ
//  if(tcnt > 15) return 2;
//  }
//
//  __delay_cycles(clkMhz * 20);
//  Data = 0x33;
//  // если да - передача команды
//  for(tcnt = 0; tcnt < 8; tcnt ++)
//  {
//    if(!(Data & (1 << tcnt)))
//    {
//    // если 0 сформировать задний фронт + 60 мкс + передний фронт
//      SETBIT(DDR_TM,  TM);                    // выход
//      CLRBIT(PORT_TM, TM);                    // низкий уровень
//      __delay_cycles(clkMhz * 45);
//      SETBIT(PORT_TM, TM);                    // высокий уровень
//    }
//    else
//    {
//    // если 1 сформировать задний фронт + 15 мкс + передний фронт + 45 мкс
//      SETBIT(DDR_TM,  TM);                    // выход
//      CLRBIT(PORT_TM, TM);                    // низкий уровень
//      __delay_cycles(clkMhz * 5);
//      SETBIT(PORT_TM, TM);                    // высокий уровень
//      __delay_cycles(clkMhz * 45);
//    }
//  __delay_cycles(clkMhz * 10);
//  }
//
//
//
//  // проверка CRC
//  if (CRC_calc(code) == 0)
//  {
//    return 0;
//  }
//
//return 3;
//  // выходим
//}
