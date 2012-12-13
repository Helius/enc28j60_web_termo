/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 * See http://www.gnu.org/licenses/gpl.html
 *
 * Ethernet remote device and sensor
 * UDP and HTTP interface
        url looks like this http://baseurl/password/command
        or http://baseurl/password/
 *
 * Chip type           : Atmega88 or Atmega168 or Atmega328 with ENC28J60
 * Note: there is a version number in the text. Search for tuxgraphics
 *********************************************/
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "type.h"
#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "timeout.h"
#include "avr_compat.h"
#include "net.h"
#include "1wire.h"
#include <avr/interrupt.h>

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24};
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
static uint8_t myip[4] = {192,168,1,10}; /** home local ip */
//static uint8_t myip[4] = {192,168,10,188}; /** vers local ip */

// listen port for tcp/www (max range 1-254)
#define MYWWWPORT 80
//
// listen port for udp
#define MYUDPPORT 1200

#define BUFFER_SIZE 550
static uint8_t buf[BUFFER_SIZE+1];

// the password string (only the first 5 char checked), (only a-z,0-9,_ characters):
//static char password[]="secret"; // must not be longer than 9 char


uint8_t verify_password(char *str)
{
        return(0);
}



// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf,uint8_t on_off)
{
        uint16_t plen;

        plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));

        plen=fill_tcp_data_p(buf,plen,PSTR("#"));
        //plen=fill_tcp_data(buf,plen,&TemArr[0][0]);

        return(plen);
}

void Init_Timer0 (void){
    TCCR0 = ((1 << CS02) | ( 1 << CS00));   // prescaller 1024
    TIMSK |= 1 << TOIE0;                    // разрешить прерывания
}



void PrintWord(WORD val, char * Str)
{

    BYTE i;
    Str[0] = 0;
    Str[1] = 0;
    Str[2] = 0;
    Str[3] = 0;
    Str[4] = 0;


    while(val >= 10000){
        val -= 10000;
        Str[0]++;
    }
    while(val >= 1000){
        val -= 1000;
        Str[1]++;
    }
    while(val >= 100){
        val -= 100;
        Str[2]++;
    }
    while(val >= 10){
        val -= 10;
        Str[3]++;
    }

    Str[4] = val;

    for(i = 0; i < 5; i++)
    {
            Str[i] += 0x30;
    }
}



//*******************************************************************
// читаем температуру с DS1820
BYTE ReadTempr(void)
{
 BYTE TemArr[5];
 BYTE DS_Buff[2];

    //DS_Buff[0] = 0;
/*
    * Посылаем импульс сброса и принимаем ответ термометра.
    * Посылаем команду Skip ROM [CCh].
    * Посылаем команду Convert T [44h].
    * Формируем задержку минимум 750мс.
    * Посылаем импульс сброса и принимаем ответ термометра.
    * Посылаем команду Skip ROM [CCh].
    * Посылаем команду Read Scratchpad [BEh].
    * Читаем данные из промежуточного ОЗУ (8 байт) и CRC.
    * Проверяем CRC, и если данные считаны верно, вычисляем температуру.
*/
    OneWire_ResetDevice();
    OneWire_WriteByte(0xcc);
    OneWire_WriteByte(0xbe);

    OneWire_ReadData(DS_Buff, 2);
    //OneWire_CRC_calc(DS_Buff, 9);

    TemArr[1] = 0;
    TemArr[2] = 0;

    if(DS_Buff[1] & 1)  // если температура отрицательная
    {
        TemArr[0] = '-';
        DS_Buff[0] = 0xFF - DS_Buff[0];
    }
    else
    {
        TemArr[0] = '+';
    }
    DS_Buff[0] >>= 1;

    while(DS_Buff[0] >= 10)
    {
        DS_Buff[0] -= 10;
        TemArr[1]++;
    }
    TemArr[2] = DS_Buff[0];

    TemArr[1] +=0x30;
    TemArr[2] +=0x30;

    TemArr[3] = 'C';
    TemArr[4] = 0;

    OneWire_ResetDevice();
    OneWire_WriteByte(0xcc);
    OneWire_WriteByte(0x44);
    return 0;
}



// that is in 30Hz intervals
ISR(TIMER0_OVF_vect)
{
static BYTE TickPerSec = 0;



    TickPerSec++;
    if(TickPerSec > 30)
    {
        ReadTempr();
        TGLBIT(PORTD, PORTD0); // мигаем светодиодом
        TickPerSec = 0;
    }
}



//********************************************************************
//
int main(void){


        uint16_t plen;
        uint16_t dat_p;
        uint8_t i=0;
        uint8_t cmd_pos=0;
        //int8_t cmd;
        uint8_t payloadlen=0;
        char str[30];
        char cmdval;
        uint16_t TimeToMeasure;

        _delay_loop_1(50); // 12ms

        /* enable PD2/INT0, as input */
        DDRD&= ~(1<<DDD2);
        /* initialize TWI*/

        Init_Timer0();
        /*initialize enc28j60*/
        enc28j60Init(mymac);
        enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
        _delay_loop_1(50); // 12ms

        // LED
        /* enable PB1, LED as output */
        DDRB|= (1<<DDB1);

        /* set output to Vcc, LED off */
        CLRBIT(PORTB, PORTB1);

        // the transistor on PD0
        DDRD|= (1<<DDD0);
        PORTD &= ~(1<<PORTD0);// transistor off


        /* Magjack leds configuration, see enc28j60 datasheet, page 11 */
        // LEDB=yellow LEDA=green
        //
        // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
        // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
        enc28j60PhyWrite(PHLCON,0x476);
        _delay_loop_1(50); // 12ms

        i=1;

        //init the ethernet/ip layer:
        init_ip_arp_udp_tcp(mymac,myip,MYWWWPORT);

        /* set output to GND, red LED on */
        SETBIT(PORTB, PORTB1);
        TimeToMeasure = 50000;
        sei();
        while(1){
                SETBIT(PORTB, PORTB1);
                _delay_ms(10);
                CLRBIT(PORTB, PORTB1);
                
                
                // get the next new packet:
                plen = enc28j60PacketReceive(BUFFER_SIZE, buf);

                /*plen will ne unequal to zero if there is a valid
                 * packet (without crc error) */
                if(plen==0){
                        continue;
                }
                //CLRBIT(PORTB, PORTB1);
                // arp is broadcast if unknown but a host may also
                // verify the mac address by sending it to
                // a unicast address.
                if(eth_type_is_arp_and_my_ip(buf,plen)){
                        make_arp_answer_from_request(buf);
                        continue;
                }

                // check if ip packets are for us:
                if(eth_type_is_ip_and_my_ip(buf,plen)==0){
                        continue;
                }
                // led----------
                if (i){
                        /* set output to Vcc, LED off */
                        PORTB|= (1<<PORTB1);
                        i=0;
                }else{
                        /* set output to GND, LED on */
                        PORTB &= ~(1<<PORTB1);
                        i=1;
                }

                if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V){
                        // a ping packet, let's send pong
                        make_echo_reply_from_request(buf,plen);
                        continue;
                }
                // tcp port www start, compare only the lower byte
                if (buf[IP_PROTO_P]==IP_PROTO_TCP_V&&buf[TCP_DST_PORT_H_P]==0&&buf[TCP_DST_PORT_L_P]==MYWWWPORT){
                        if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V){
                                make_tcp_synack_from_syn(buf);
                                // make_tcp_synack_from_syn does already send the syn,ack
                                continue;
                        }
                        if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V){
                                init_len_info(buf); // init some data structures
                                // we can possibly have no data, just ack:
                                dat_p=get_tcp_data_pointer();
                                if (dat_p==0){
                                        if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V){
                                                // finack, answer with ack
                                                make_tcp_ack_from_any(buf);
                                        }
                                        // just an ack with no data, wait for next packet
                                        continue;
                                }
                                if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
                                        // head, post and other methods:
                                        //
                                        // for possible status codes see:
                                        // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
                                        plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>"));
                                        goto SENDTCP;
                                }
                                // if (cmd==-2) or any other value
                                // just display the status:
                                plen=print_webpage(buf,(PORTD & (1<<PORTD0)));
                                //
SENDTCP:
                                make_tcp_ack_from_any(buf); // send ack for http get
                                make_tcp_ack_with_data(buf,plen); // send data
                                continue;
                        }

                }
                // tcp port www end
                //
                // udp start, we listen on udp port 1200=0x4B0
                if (buf[IP_PROTO_P]==IP_PROTO_UDP_V&&buf[UDP_DST_PORT_H_P]==4&&buf[UDP_DST_PORT_L_P]==0xb0){
                        payloadlen=buf[UDP_LEN_L_P]-UDP_HEADER_LEN;
                        // you must sent a string starting with v
                        // e.g udpcom version 10.0.0.24
                        if (verify_password((char *)&(buf[UDP_DATA_P]))){
                                // find the first comma which indicates
                                // the start of a command:
                                cmd_pos=0;
                                while(cmd_pos<payloadlen){
                                        cmd_pos++;
                                        if (buf[UDP_DATA_P+cmd_pos]==','){
                                                cmd_pos++; // put on start of cmd
                                                break;
                                        }
                                }
                                // a command is one char and a value. At
                                // least 3 characters long. It has an '=' on
                                // position 2:
                                if (cmd_pos<2 || cmd_pos>payloadlen-3 || buf[UDP_DATA_P+cmd_pos+1]!='='){
                                        strcpy(str,"e=no_cmd");
                                        goto ANSWER;
                                }
                                // supported commands are
                                // t=1 t=0 t=?
                                if (buf[UDP_DATA_P+cmd_pos]=='t'){
                                        cmdval=buf[UDP_DATA_P+cmd_pos+2];
                                        if(cmdval=='1'){
                                                PORTD|= (1<<PORTD0);// transistor on
                                                strcpy(str,"t=1");
                                                goto ANSWER;
                                        }else if(cmdval=='0'){
                                                PORTD &= ~(1<<PORTD0);// transistor off
                                                strcpy(str,"t=0");
                                                goto ANSWER;
                                        }else if(cmdval=='?'){
                                                if (PORTD & (1<<PORTD0)){
                                                        strcpy(str,"t=1");
                                                        goto ANSWER;
                                                }
                                                strcpy(str,"t=0");
                                                goto ANSWER;
                                        }
                                }
                                strcpy(str,"e=no_such_cmd");
                                goto ANSWER;
                        }
                        strcpy(str,"e=invalid_pw");
ANSWER:
                        make_udp_reply_from_request(buf,str,strlen(str),MYUDPPORT);
                }
        }
        return (0);
}
