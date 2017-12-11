/*                  e-gadget.header
 * SPI.h
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

#ifndef SPI_H_
#define SPI_H_
//""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
// define what type of SPI you want to use: 1 - using software SPI, 0 - using hardware SPI
//..............................................................................................................
#define USE_SOFTWARE_SPI 0




void init_SPI(void);							//initialize SPI
uint8_t SPI_WriteReadByte( uint8_t data );		//write/read byte to/from nRF
void SPI_WriteByte( uint8_t data );				//write byte to nRF
void SPI_WriteDataBuffer ( uint8_t * data_buffer_out , uint8_t length );								//write to nRF buffer with data
void SPI_WriteReadDataBuffer ( uint8_t * data_buffer_in, uint8_t * data_buffer_out, uint8_t length );	//write/read to/from nRF buffer with data



#endif /* SPI_H_ */









