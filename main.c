/*                  e-gadget.header
 * main.c
 *
 *  Created on: 2015-04-07
 *    Modyfied: 2015-04-07 19:45:16
 *      Author: Nefarious19
 *
 * Project name: "NRF24"
 *
 *
 *          MCU: ATmega32
 *        F_CPU: 16 000 000 Hz
 *
 *    Flash: 2 992 bytes   [ 9,1 % ]
 *      RAM:  189 bytes   [ 9,2 % ]
 *   EEPROM:  0 bytes   [ 0,0 % ]
 *
 */


#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>


//#include "usart.h"
#include "nRF24L01.h"


void moja_funkcja ( void * nRF_RX_buff , uint8_t len );

int main (void)
{
	//led do migania
	DDRB  |= (1<<PB0);
	PORTB &= ~(1<<PB0);

	//inicjalizacja usarta
//	init_USART(__UBRR);

	//inicjalizacja nrfa
    nRF_init();
    register_nRF_RX_Event_Callback(moja_funkcja);

    //odpalamy przerwanko
    sei();


    nRF_RX_Power_Up(); //odpalamy nRFA!

	while(1)
	{
		nRF_RX_EVENT();
	}
}


void moja_funkcja( void * nRF_RX_buff, uint8_t len )
{
//	char buffer[10];
//	usart_send_str( "ODEBRANO BAJTÓW: " );
//	usart_send_str( itoa(len,buffer,10) );
//	ENTER_NEW;
//	usart_send_str( (char *) nRF_RX_buff );
//	ENTER_NEW;
//	nRF_SendDataToAir("DANE OK");
	if (!strcmp_P( nRF_RX_buff , PSTR("ZAPAL_LED"))) PORTB |= (1<<PB0);
	if (!strcmp_P( nRF_RX_buff , PSTR("ZGAS_LED"))) PORTB &= ~(1<<PB0);
}
