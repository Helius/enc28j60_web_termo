//#include <includes.h>
#include "type.h"
#include <avr/io.h>
#include "i2c.h"

#define NOTERMO  0
#define YESTERMO 1

 BYTE temperature[2];
 BYTE TStatus;

/****************************************************************************
	Function : char Init_TWI(void)
	Setup the TWI module
	Baudrate 	: 100kHz @ 4MHz system clock
	Own address : OWN_ADR (Defined in TWI_driver.h)
****************************************************************************/

void Init_TWI(void)
{
  //TWAR = 0x99;							//Set own slave address
  TWBR = 250; // bitRate 16 + 2*42 = 100 SCL = SCK/100 = 8000000/100 = 80000
  TWCR = (1<<TWEN);						//Enable TWI-interface
}

/****************************************************************************
	Function : void Wait_TWI_int(void)
	Loop until TWI interrupt flag is set
****************************************************************************/
void Wait_TWI_int(void)
{
    WORD TimeOutT = 0;
    while (!(TWCR & (1<<TWINT)))
    {
    TimeOutT ++;
        if (TimeOutT > 50000)
        {
        TStatus = NOTERMO;
        return;
        }
    }
}

/****************************************************************************
	Function :unsigned char	Send_start(void)
	Send a START condition to the bus and wait for the TWINT get set set to
	see the result. If it failed return the TWSR value, if succes return
	SUCCESS.
****************************************************************************/
void  Send_start(void)
{
	TWCR = ((1<<TWINT)+(1<<TWSTA)+(1<<TWEN));//Send START

	Wait_TWI_int();							//Wait for TWI interrupt flag set

}

/****************************************************************************
	Function :
	Send a STOP condition to the bus
****************************************************************************/
void Send_stop(void)
{
	TWCR = ((1<<TWEN)+(1<<TWINT)+(1<<TWSTO));//Send STOP condition
}

/****************************************************************************
	Function : unsigned char Send_to_TWI(tx_type *data_pack)
	Main TWI communication function. Handles all signaling against the bus.

****************************************************************************/
void Send_to_TWI()
{
        Send_start();				            //Send START and repeated START
        Send_adr(0x99);                         //Send slave address+W/R
        temperature[0] = Get_byte();
        temperature[1] = Get_byte();
        TWCR = ((1<<TWINT)+(1<<TWEN));
        Wait_TWI_int();
        Send_stop();				//Send STOP
}

/****************************************************************************
	Function : unsigned char Send_byte(unsigned char data)
	Send one byte to the bus.
****************************************************************************/
void Send_byte(unsigned char data)
{
	Wait_TWI_int();							//Wait for TWI interrupt flag set

	TWDR = data;
 	TWCR = ((1<<TWINT)+(1<<TWEN));   		//Clear int flag to send byte

	Wait_TWI_int();							//Wait for TWI interrupt flag set

}

/****************************************************************************
	Function : unsigned char Send_adr(unsigned char adr)
	Send a SLA+W/R to the bus
****************************************************************************/
void Send_adr(unsigned char adr)
{
	Wait_TWI_int();							//Wait for TWI interrupt flag set

	TWDR = adr;
	TWCR = ((1<<TWINT)+(1<<TWEN));   		                //Clear int flag to send byte

	Wait_TWI_int();							//Wait for TWI interrupt flag set
}

/****************************************************************************
	Function : unsigned char Get_byte(unsigned char *rx_ptr,char last_byte)
	Wait for TWINT to receive one byte from the slave and send ACK. If this
	is the last byte the master will send NACK to tell the slave that it
	shall stop transmitting.
****************************************************************************/
BYTE Get_byte(void)
{
	Wait_TWI_int();							//Wait for TWI interrupt flag set

	TWCR = ((1<<TWINT)+(1<<TWEA)+(1<<TWEN));

	Wait_TWI_int();							//Wait for TWI interrupt flag set

	return TWDR;							//Save received byte
}

