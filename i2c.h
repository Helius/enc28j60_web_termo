#ifndef I2C_H
#define I2C_H
#include "type.h"
/****************************************************************************
	Function definitions
****************************************************************************/

void Init_TWI(void);					//Initilaize TWI
void Send_to_TWI(void);
BYTE Get_byte(void);	                                 //Receive one byte
void Send_start(void);		                //Send a Start condition
void Send_adr(unsigned char);                  //Send a Slave adr.
void Send_stop(void);				        //Send a Stop condition
void Wait_TWI_int(void);				//Wait for TWI interrupt to occur
void Reset_TWI (void);				        //Reset the TWI module and
											//release the bus



#endif
