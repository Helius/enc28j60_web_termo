#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#define DDR_TM    DDRC
#define PORT_TM   PORTC
#define PIN_TM    PINC
#define TM        5
#define clkMhz    8

BYTE OneWire_ResetDevice(void);                 // делает сброс на шине и ожидает присутствия устройства
void OneWire_WriteByte(BYTE);                   // пишет байт в шину
void OneWire_ReadData(BYTE*, BYTE);             // читает len байт в буфер, возвращает
BYTE OneWire_CRC_calc(BYTE *, BYTE);            // подсчитывает CRC для буфера

//BYTE ReadKey(BYTE *code, BYTE TM);      //*code - указатель на буфер, куда сохранитсо код ключа, TM - номер пина порта где висит TM0
#endif
