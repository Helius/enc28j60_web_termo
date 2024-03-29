/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 *
 * Tuxgraphics AVR webserver/ethernet board
 *
 * http://tuxgraphics.org/electronics/
 * Chip type           : Atmega88/168/328 with ENC28J60
 *********************************************/
#include <avr/io.h>
#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "timeout.h"
#include "avr_compat.h"
#include "net.h"

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24};
static uint8_t myip[4] = {192,168,10,10};
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX

#define BUFFER_SIZE 250
static uint8_t buf[BUFFER_SIZE+1];

int main(void){


        uint16_t plen;
        uint8_t i=0;

        // set the clock speed to 8MHz
        // set the clock prescaler. First write CLKPCE to enable setting of clock the
        // next four instructions.
        // CLKPR=(1<<CLKPCE);
        // CLKPR=0; // 8 MHZ

        _delay_ms(20);

        // LED
        /* enable PB1, LED as output */
        DDRB|= (1<<DDB1);

        /* one blink for indicate of start programm */
        cbi(PORTB, PORTB1); // on
        _delay_ms(100);
        sbi(PORTB,PORTB1);  // off
        _delay_ms(100);
        cbi(PORTB,PORTB1);  // on

        /*initialize enc28j60*/
        enc28j60Init(mymac);
        enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
        _delay_ms(10);


	/* Magjack leds configuration, see enc28j60 datasheet, page 11 */
	// LEDA=greed LEDB=yellow
	//
	// 0x880 is PHLCON LEDB=on, LEDA=on
	// enc28j60PhyWrite(PHLCON,0b0000 1000 1000 00 00);
	enc28j60PhyWrite(PHLCON,0x880);
	_delay_ms(250); _delay_ms(250);
	//
	// 0x990 is PHLCON LEDB=off, LEDA=off
	// enc28j60PhyWrite(PHLCON,0b0000 1001 1001 00 00);
	enc28j60PhyWrite(PHLCON,0x990);
	_delay_ms(250); _delay_ms(250);
	//
	// 0x880 is PHLCON LEDB=on, LEDA=on
	// enc28j60PhyWrite(PHLCON,0b0000 1000 1000 00 00);
	enc28j60PhyWrite(PHLCON,0x880);
	_delay_ms(250); _delay_ms(250);
	//
	// 0x990 is PHLCON LEDB=off, LEDA=off
	// enc28j60PhyWrite(PHLCON,0b0000 1001 1001 00 00);
	enc28j60PhyWrite(PHLCON,0x990);
	_delay_ms(250); _delay_ms(250);
	//
        // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
        // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
        enc28j60PhyWrite(PHLCON,0x476);
	_delay_ms(100);

        //init the ethernet/ip layer:
        init_ip_arp_udp_tcp(mymac,myip,80);

    // "initialisation done" led off
    sbi(PORTB, PORTB1);

        while(1){
                // get the next new packet:
                plen = enc28j60PacketReceive(BUFFER_SIZE, buf);

                /*plen will ne unequal to zero if there is a valid
                 * packet (without crc error) */
                if(plen==0){
                        sbi(PORTB, PORTB1);
                        continue;
                }
                cbi(PORTB, PORTB1);
                // arp is broadcast if unknown but a host may also
                // verify the mac address by sending it to
                // a unicast address.
                if(eth_type_is_arp_and_my_ip(buf,plen)){
                        make_arp_answer_from_request(buf);
                        continue;
                }
                // check if ip packets (icmp or udp) are for us:
                if(eth_type_is_ip_and_my_ip(buf,plen)==0){
                        continue;
                }

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
        }
        return (0);
}
