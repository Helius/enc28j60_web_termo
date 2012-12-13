#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#define DDR_TM    DDRC
#define PORT_TM   PORTC
#define PIN_TM    PINC
#define TM        5
#define clkMhz    8

BYTE OneWire_ResetDevice(void);                 // ������ ����� �� ���� � ������� ����������� ����������
void OneWire_WriteByte(BYTE);                   // ����� ���� � ����
void OneWire_ReadData(BYTE*, BYTE);             // ������ len ���� � �����, ����������
BYTE OneWire_CRC_calc(BYTE *, BYTE);            // ������������ CRC ��� ������

//BYTE ReadKey(BYTE *code, BYTE TM);      //*code - ��������� �� �����, ���� ���������� ��� �����, TM - ����� ���� ����� ��� ����� TM0
#endif
