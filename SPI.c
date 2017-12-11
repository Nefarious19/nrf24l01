/*                  e-gadget.header
 * SPI.c
 *
 *  Created on: 
 *    Modyfied: 2015-04-13 21:18:47
 *      Author: Nefarious19
 *
 * Project name: "NRF24"
 *
 *	This is AVR GCC library for nRF24L01 module, ver. 1.0
 *	It can be only used by REGISTERED USERS of www.forum.atnel.pl, 
 *	they must only leave this header in they C code.
 *	
 *	Library code was written on the basis of:
 *	
 *	https://www.sklep.atnel.pl/pl/p/AVR-Microcontrollers-C-Programming-Basics-EN-BOOK-DVD/103
 *	https://www.sklep.atnel.pl/pl/p/Jezyk-C-Pasja-programowania-mikrokontrolerow-8-bitowych-Wydanie-II-Ksiazka-DVD/104
 *	
 *	"
 *	
 *	This library uses also some ideas from:
 *	
 *	http://gizmosnack.blogspot.com/2013/04/tutorial-nrf24l01-and-avr.html
 *	http://www.tinkerer.eu/AVRLib/nRF24L01/
 *
 *
 */



#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "SPI.h"
#include "nRF24L01.h"
#include "nRF24L01_memory_map.h"

#if USE_SOFTWARE_SPI == 0


void init_SPI(void)
{
	IRQ_DDR_PORT &= ~(1<<IRQ);
	CE_DDR_PORT |= (1<<CE);
	CSN_DDR_PORT |= (1<<CSN);
	SS_DDR_PORT |= (1<<SS);
	MOSI_DDR_PORT |= (1<<MOSI);
	MISO_DDR_PORT &= ~(1<<MISO);
	SCK_DDR_PORT |= (1<<SCK);

	IRQ_PORT |= (1<<IRQ);
	CE_PORT  &= ~(1<<CE);
	CSN_PORT |= (1<<CSN);
	SS_PORT  |= (1<<SS);
	MOSI_PORT |= (1<<MOSI);
	SCK_PORT  |= (1<<SCK);
//SET pins MOSI, CSN(chip select),SCK, i CE(control RX, TX nRFA) as ouputs
	SPCR |= (0<<SPIE) | //turn OFF SPI intterrupt
			(1<<SPE)  | //enable SPI module
			(0<<DORD) | //set data order
			(1<<MSTR) | //enable SPI in master mode
			(0<<CPOL) | //SCK polarity, rising edge is active
			(0<<CPHA) | //data polarity, read data on rising edge
			(0<<SPR0) | //SCK speed
			(01<<SPR1) ; //
	SPSR |= (1<<SPI2X); //SPI sck 2 times faster
}



uint8_t SPI_WriteReadByte( uint8_t data )
{
	SPDR = data;          		 //send byte to SPDR register

	while( !(SPSR & (1<<SPIF)) );  //wait for data shifting
	return SPDR;          		 //return data from the SPDR (here are now data from the nRF)
}


void SPI_WriteByte( uint8_t data )
{
	SPDR = data;          		//send byte to SPDR register
	while(!(SPSR & (1<<SPIF))); //wait for data shifting

}




void SPI_WriteDataBuffer ( uint8_t * data_buffer_out , uint8_t length )
{
	uint8_t * pointer;				//declare pointer
	uint8_t i;						//declare indexing variable
	pointer = data_buffer_out;		//pointer is showing on first byte of "data_buffer_out"
	for( i = 0; i < length; i++ )	//until "i" is smaller than "length"
	{
		SPI_WriteByte( *pointer++ );//send bytes to nRF
	}
}



void SPI_WriteReadDataBuffer (uint8_t * data_buffer_in, uint8_t * data_buffer_out, uint8_t length )
{
	uint8_t * pointer1;				//declare pointer1
	uint8_t * pointer2;				//declare pointer2
	uint8_t i;						//declare indexing variable

	pointer1 = data_buffer_out;		//pointer1 is showing on first byte of "data_buffer_out"
	pointer2 = data_buffer_in;		//pointer2 is showing on first byte of "data_buffer_in"


	for( i = 0; i < length; i++ )	//until "i" is smaller than "length"
	{

		*pointer2++ = SPI_WriteReadByte( *pointer1++ ); //send bytes to nRF and read data back from nRF

	}


}

#endif


#if USE_SOFTWARE_SPI == 1

void init_SPI(void)
{
	IRQ_DDR_PORT &= ~(1<<IRQ);
	CE_DDR_PORT |= (1<<CE);
	CSN_DDR_PORT |= (1<<CSN);
	SS_DDR_PORT |= (1<<SS);
	MOSI_DDR_PORT |= (1<<MOSI);
	MISO_DDR_PORT &= ~(1<<MISO);
	SCK_DDR_PORT |= (1<<SCK);

	IRQ_PORT |= (1<<IRQ);
	CE_PORT  &= ~(1<<CE);
	CSN_PORT |= (1<<CSN);
	SS_PORT  |= (1<<SS);
	MOSI_PORT &= ~(1<<MOSI);
	MISO_PORT &= ~(1<<MISO);
	SCK_PORT &= ~(1<<SCK);

	_delay_ms(10);															  //wait 10 miliseconds
}

uint8_t SPI_WriteReadByte( uint8_t data )
{
	register uint8_t i;									//indexing variable "i"

	for( i = 0; i < 8 ; i++ )							//until "i" is smaller than 8
	{
		if( data & 0b10000000 )							// if MSB of byte is equal to one
		{
			MOSI_PORT |= (1<<MOSI);						// MOSI=1;
		}
		else											//else
		{
			MOSI_PORT &= ~(1<<MOSI);						// MOSI=0;
		}

		data = ( data << 1 );           				//shift next bit into MSB

		if( MISO_PIN & (1<<MISO) ) data |= 1;			//if MISO pin is HIGH set LSB of data to ONE
		SCK_PORT |= (1<<SCK);							//make SCK pin high
#if F_CPU > 16000000									//if F_CPU is biger than 16000000
		asm("nop");										//put here asm("nop") command
#endif
		SCK_PORT &= ~(1<<SCK);							//make sck pin low

	}
	return data;          								//return recived data
}

void SPI_WriteByte( uint8_t data )
{
		register uint8_t i;								//indexing variable "i"
		for( i = 0; i < 8 ; i++ )						//until "i" is smaller than 8
		{

			if( data & 0b10000000 )						// if MSB of byte is equal to one
			{
				MOSI_PORT |= (1<<MOSI);					// MOSI=1;
			}
			else										//else
			{
				MOSI_PORT &= ~(1<<MOSI);					//MOSI=0;
			}

			data = ( data << 1 );						//shift data to left
			SCK_PORT |= (1<<SCK);						//SCK high
#if F_CPU > 16000000									//if F_CPU is biger than 16000000
		asm("nop");										//put here asm("nop") command
#endif
			SCK_PORT &= ~(1<<SCK);						//SCK low
		}
}

void SPI_WriteDataBuffer ( uint8_t * data_buffer_out , uint8_t length )
{
	uint8_t * pointer;				//declare pointer
	uint8_t i;						//declare indexing variable
	pointer = data_buffer_out;		//pointer is showing on first byte of "data_buffer_out"
	for( i = 0; i < length; i++ )	//until "i" is smaller than "length"
	{
		SPI_WriteByte( *pointer++ );//send bytes to nRF
	}
}

void SPI_WriteReadDataBuffer (uint8_t * data_buffer_in, uint8_t * data_buffer_out, uint8_t length )
{
	uint8_t * pointer1;				//declare pointer1
	uint8_t * pointer2;				//declare pointer2
	uint8_t i;						//declare indexing variable

	pointer1 = data_buffer_out;		//pointer1 is showing on first byte of "data_buffer_out"
	pointer2 = data_buffer_in;		//pointer2 is showing on first byte of "data_buffer_in"
	for( i = 0; i < length; i++ )	//until "i" is smaller than "length"
	{
		*pointer2++ = SPI_WriteReadByte( *pointer1++ ); //send bytes to nRF and read data back from nRF
	}
}

#endif









