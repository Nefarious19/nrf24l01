/*                  e-gadget.header
 * nRF24L01.c
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
#include <string.h>
#include <avr/pgmspace.h>

#include "usart.h"

#include "SPI.h"
#include "nRF24L01.h"
#include "nRF24L01_memory_map.h"

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// declarations of internal tool functions
//..................................................................................................................
void nRF_Read_Registers( uint8_t register_name, uint8_t * buffer_for_registers_content, uint8_t length_of_content );  //reads given ammount of bytes from choosen register
void nRF_Write_Register( uint8_t register_name, uint8_t * buffer_for_registers_content, uint8_t length_of_content ); //writes given ammount of bytes to choosen register
void nRF_Set_Active_DataPipe_And_ACK (uint8_t DataPipe, uint8_t on_or_off, uint8_t ACK_on_or_off );					 //This function is used to set active payload/payloads and enable/disable ACK functions
void nRF_Set_PAYLOAD_Width ( uint8_t payload, uint8_t width );  													 //this function is used to set payload length. Use this function if dynamic payload length is disabled
void nRF_Set_Dynamic_Payload_State_On_Data_Pipe(uint8_t data_pipe_number, uint8_t on_off ); 						 //this function is used to set
void nRF_Set_Data_Speed_And_Reciver_Power(uint8_t Data_rate, uint8_t power);                                         //this function is used to set transmission speed and reciver power
void nRF_Set_State_And_Width_Of_CRC( uint8_t one_or_two_bytes , uint8_t on_or_off);                                  //this function is used to set CRC state and width.
//__________________________________________________________________________________________________________________


//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// declaration of global variables which stores values depending on the transmission states
//..................................................................................................................
volatile uint8_t TX_flag;   //data sent event flag
volatile uint8_t RX_flag;   //data ready event flag
//..................................................................................................................
volatile uint8_t disable_dynamic_payload;
volatile uint8_t payload_width;
volatile uint8_t first_time = 0;
volatile uint8_t set_first_time_into_RX_MODE_after_TX_MODE = 0;
//__________________________________________________________________________________________________________________



//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// declaration of the buffer which stores incoming payloads data
//..................................................................................................................
uint8_t nRF_RX_bufffer[MAXIMUM_PAYLOAD_SIZE+1]; //reciver buffer
//__________________________________________________________________________________________________________________


//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// callback function declaration
//........................................................................................................................
static void ( * nRF_RX_Event_Callback ) ( void * nRF_RX_buff , uint8_t len );
//________________________________________________________________________________________________________________________


//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function which is used to register your own callback function
//........................................................................................................................
void register_nRF_RX_Event_Callback ( void ( * callback )( void * nRF_RX_buff , uint8_t len ) )
{
	nRF_RX_Event_Callback = callback; //in this line we give an addres to our callback function
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// recived data event function
//........................................................................................................................
void nRF_RX_EVENT (void)
{
#if USE_IRQ == 0
	if ( nRF_Data_Ready() )  RX_flag = 1;
	else RX_flag = 0;
#endif

	if(set_first_time_into_RX_MODE_after_TX_MODE == 1)
	{
		nRF_RX_Power_Up();
		set_first_time_into_RX_MODE_after_TX_MODE = 0;
	}
	if ( RX_flag )			// if RX_flag == 1 then: - je¿eli zmienna RX_flag == 1 to:
	{
		CE_LOW;
		uint8_t fifo_status; //variable which stores FIFO_STATUS register value
		uint8_t len;         //variable which stores length of recived payload
		uint8_t i;           //index variable for loop
		uint8_t * wsk;       //pointer for first byte of reciver buffer nRF_RX_buffer
		RX_flag = 0;         //reset the RX_flag

		//do...while loop. this loop executes until RX_FIFO is empty

		do
		{
			wsk = nRF_RX_bufffer;        				//wsk equals to addres of first byte nRF_RX_bufffer
			CSN_LOW;                     				//chip select low
			SPI_WriteByte(R_RX_PL_WID);  				//send: read length of payload command
			len = SPI_WriteReadByte(NOP);				//read length
			CSN_HIGH;                    				//chip select high
			if ( len > MAXIMUM_PAYLOAD_SIZE ) break;       				//if len is bigger than maximum payload size then break
			CSN_LOW;                    				//chip select low
			SPI_WriteByte( R_RX_PAYLOAD ); //read payload command
			i = len;
			while(i--)   //read data until len == 0
			{
				*wsk++ = SPI_WriteReadByte(NOP);
			}
			CSN_HIGH;                    //csn goes high - csn stan wysoki
			fifo_status = nRF_Read_One_Byte_From_Register(FIFO_STATUS); //read FIFO_STATUS register
			if ( len &&  nRF_RX_Event_Callback ) ( * nRF_RX_Event_Callback )( nRF_RX_bufffer, len ); //if callback function is registered and len is bigger	 //than 0 send recived data to your callback function
		}
		while ( !( fifo_status & (1<<RX_EMPTY) ));  //if RX_EMPTY bit is LOW then loop must execute.,

		CSN_LOW;
		uint8_t status = SPI_WriteReadByte(NOP);
		CSN_HIGH;
		status |= (1<<RX_DR);							//reset intterupt bit on status variable
		nRF_Config_Register(STATUS, status);			//save status variable in STATUS register
		CE_HIGH;
	}
}



//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// INITIALIAZE NRF function
//........................................................................................................................

void nRF_init( void )
{

	init_SPI(); //initialize SPI (software or hardware, you must choose it in SPI.h file)

	#if USE_IRQ == 1
	Initialize_INTERRUPT_For_nRF(); //initialize INTx or PCINTx interrupt for nRF, you must do it on your Own
	#endif

	nRF_SET_Transmitter_Adres(PSTR("ATNEL"));		//set transmitter addres
	nRF_SET_Reciver_Addres(RX_ADDR_P0, PSTR("ATNEL"));						//set reciving addres for datapipe0
	nRF_Config_Register( CONFIG, nRF24L01_CONFIG);         					//Write interrupt masks nRF_CONFIG to CONGIG register of nRF
	nRF_Clear_RX();                                       				    //Clear RX FIFO
	nRF_Clear_TX();                                        				    //Clear TX FIFO
	nRF_Set_State_And_Width_Of_CRC( ONE_BYTE , ON );        			    //ON == Enable CRC; OFF == disable CRC, ONE_BYTE or TWO_BYTES - width of CRC;
	nRF_Set_Channel(10);                                   				    //Set channel number
    nRF_Set_Active_DataPipe_And_ACK ( ERX_P0, ON, ACK_ON );					//Set which datapipe state you want to change, in this case datapipe = 0 (ERX_P0), ON - means enable this datapipe, ACK_ON means to enable ACK for choosen data pipe.
	nRF_Set_Retransmission_Time_And_Ammount(WAIT_4000uS , RETR_15_TIMES );  //set time between retransmissions and ammount of retranssmisions
	nRF_Set_Data_Speed_And_Reciver_Power(TRANS_SPEED_1MB, RF_PWR_0dB );     //Set transmision speed and reciver power
	nRF_Set_Dynamic_Payload_State_On_Data_Pipe( DPL_P0 , ON );              //Enable dynamic payload of choosen datapipe


	RX_flag = 1;		//Clear RX flag
	TX_flag = 0;        //Clear TX flag

	nRF_Config_Register(STATUS, (1<<TX_DS) | (1<<RX_DR) | (1<<MAX_RT));    //Clear interrupt bits in STATUS register

}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to read one byte from register given as attribute of the function
//........................................................................................................................
uint8_t nRF_Read_One_Byte_From_Register(uint8_t register_name)
{
	uint8_t data;       											//temporary variable data is used to store and return value of choosen register
	CSN_LOW;                                                        //chip select low
	SPI_WriteByte( R_REGISTER | (REGISTER_MASK & register_name) );  //send R_REGISTER command with number of choosen register (SEE nRF24L01_memory_map.h)
	data = SPI_WriteReadByte(NOP);                                  //write to "data", value from choosen register
	CSN_HIGH;                                                       //chip select high
	return data;                                                    //return data
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to write data to the choosen register
//........................................................................................................................
void nRF_Config_Register(uint8_t register_name, uint8_t value)
{
	CSN_LOW;														//make CSN low
	SPI_WriteByte( W_REGISTER | (REGISTER_MASK & register_name) );  //send information to nRF which register you want to write
	SPI_WriteByte( value );                                         //send value that you want write to this register
	CSN_HIGH;                                                       //make CSN high
}


//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to read data from the choosen register to buffer
//........................................................................................................................
void nRF_Read_Registers( uint8_t register_name, uint8_t * buffer_for_registers_content, uint8_t length_of_content )
{
	CSN_LOW;																								 	//make CSN low
	SPI_WriteByte( R_REGISTER | ( REGISTER_MASK & register_name ) );										 	//send information to nRF which register you want to read
	SPI_WriteReadDataBuffer( buffer_for_registers_content, buffer_for_registers_content, length_of_content );	//read from nRF given ammount of bytes to the buffer
	CSN_HIGH;       																						 	//make CSN high
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to write data from the buffer to the choosen register
//........................................................................................................................
void nRF_Write_Register( uint8_t register_name, uint8_t * buffer_for_registers_content, uint8_t length_of_content )
{
	CSN_LOW;																									//make CSN low
	SPI_WriteByte( W_REGISTER | ( REGISTER_MASK & register_name ) );								//send information to nRF which register you want to write
	SPI_WriteReadDataBuffer( buffer_for_registers_content, buffer_for_registers_content, length_of_content );	//write to nRF given ammount of bytes from the buffer
	CSN_HIGH;																									//make CSN low
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to set transmitter addres.
//........................................................................................................................
void nRF_SET_Transmitter_Adres(const char * addres)
{
	char TX_addres[TX_ADDRES_LENGTH];														//declaration of TX_addres temporary buffer
	for(uint8_t i = 0; i < TX_ADDRES_LENGTH; i++ )                                          //until i is smaller than TX_ADDRES LENGTH copy data from flash to temporary TX_addres buffer
	{
		TX_addres[i] = pgm_read_byte(addres++);
	}
	if( TX_ADDRES_LENGTH > 5 ) nRF_Write_Register(TX_ADDR, (uint8_t *) TX_addres, 5);		//if TX_ADDRES_LENGTH is bigger than 5 bytes send only 5 bytes
	else if( TX_ADDRES_LENGTH < 3 ) nRF_Write_Register(TX_ADDR, (uint8_t *) TX_addres, 3);  //if TX_ADDRES_LENGTH is smaller than 3 bytes send 3 bytes
	else nRF_Write_Register(TX_ADDR, (uint8_t *) TX_addres , TX_ADDRES_LENGTH);             //else send given by attribute ammount of bytes

}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to set transmitter addres.
//........................................................................................................................
void nRF_SET_Reciver_Addres( uint8_t data_pipe, const char * addres )
{
	char RX_addres[RX_ADDRES_LENGTH];														//declaration of RX_addres temporary buffer

	if( data_pipe > RX_ADDR_P5 ) data_pipe = RX_ADDR_P0; 								    //this is a protection. When given by data_pipe variable register addres is smaller
	else if( data_pipe < RX_ADDR_P0 ) data_pipe = RX_ADDR_P0;								// or bigger than possible value, this condition is setting deffault value
	else if( data_pipe > RX_ADDR_P5 ) data_pipe = RX_ADDR_P5;

	if ( data_pipe < RX_ADDR_P2 )															// data_pipe ix equal to RX_ADDR_P0 or RX_ADDR_P1 write 5 bytes of addres to nRF
	{
		for(uint8_t i = 0; i < RX_ADDRES_LENGTH; i++ )
		{
		RX_addres[i] = pgm_read_byte(addres++);
		}

		if( RX_ADDRES_LENGTH > 5 ) nRF_Write_Register(data_pipe, (uint8_t*)RX_addres, 5);
		else if( RX_ADDRES_LENGTH < 3 ) nRF_Write_Register(data_pipe, (uint8_t*) RX_addres, 3);
		else nRF_Write_Register(data_pipe, (uint8_t *) RX_addres, RX_ADDRES_LENGTH);
	}
	else																					//else send only one byte of addres because first four bytes are the same as the RX_ADDR_P1 first four bytes of addres.
	{
		RX_addres[0] = pgm_read_byte(addres);
		nRF_Config_Register(data_pipe, RX_addres[0]);
	}

}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to chcecking RX_DR bit in STATUS register. When this bit is HIGH that means tha data are ready
// function used in pooling mode
//........................................................................................................................
uint8_t nRF_Data_Ready(void)
{
	CSN_LOW;									//make CSN low
	uint8_t data = SPI_WriteReadByte(NOP);		//send to nRF dummy byte and read STATUS register
	CSN_HIGH;									//make CSN high
	return (data & (1<<RX_DR));					//return 0 if data not ready or 1 if data ready
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to set state and width of CRC. First attrbiute of this function is ONE_BYTE or TWO_BYTES,
// second attribute is ON or OFF
//........................................................................................................................
void nRF_Set_State_And_Width_Of_CRC( uint8_t one_or_two_bytes , uint8_t on_or_off)
{
	uint8_t config = nRF_Read_One_Byte_From_Register(CONFIG);				//read data from congig register and save it to config variable

	if ( one_or_two_bytes == ONE_BYTE ) config &= ~(1<<CRCO);				//if first given attribute is ONE_BYTE clear CRCO bit
	else if ( one_or_two_bytes == TWO_BYTES ) config |= (1<<CRCO);			//else if first given attribute is TWO_BYTES set CRCO bit
	else config &= ~(1<<CRCO);												//else given attribute is diffrent set CRCO low

	if (on_or_off == ON)   config |= (1<<EN_CRC);							//if second attribute is equal to ON, set EN_CRC bit
	else if ( on_or_off == OFF )  config &= ~(1<<EN_CRC);					//esle reset the EN_CRC bit

	nRF_Config_Register(CONFIG, config);									//write config content to CONFIG register
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to switch nRF into TX mode
//........................................................................................................................
void nRF_TX_Power_Up(void)
{
	CE_LOW;																	//make CE low
	uint8_t config;															//declaration of temporary variable
	config = nRF_Read_One_Byte_From_Register(CONFIG);						//save copy of config regster in config variable
	config &= ~(1<<PRIM_RX);												//reset PRIM_RX bit
	config |= (1<<PWR_UP);													//set PWR_UP bit
	nRF_Config_Register( CONFIG , config);									//write config variable to CONGIG register																//make CE high
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to switch nRF into RX mode
//........................................................................................................................
void nRF_RX_Power_Up(void)
{
	CE_LOW;																	//make CE_LOW
	uint8_t config;															//declaration of config variable
	config = nRF_Read_One_Byte_From_Register(CONFIG);						//save copy of CONFIG register to config variable
	nRF_Config_Register(CONFIG , config | (1<<PWR_UP) | (1<<PRIM_RX) );		//set PWR_UP and PRIM_RX bits in config variable ande save it in nRF CONFIG register
	CE_HIGH;																//make CE_HIGH
	_delay_us(130);
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to switch nRF into POWER DOWN mode
//........................................................................................................................
void nRF_Power_Down(void)
{
	CE_LOW;																	//make CE_LOW
	uint8_t config;															//declaration of config variable
	config = nRF_Read_One_Byte_From_Register(CONFIG);						//save copy of CONFIG register to config variable
	config &= ~((1<<PWR_UP) | (1<<PRIM_RX));								//reset PWR_UP and PRIM_RX bits
	nRF_Config_Register(CONFIG , config );									//save congig variable in CONFIG register
	nRF_Clear_RX();															//clear RX fifo
	nRF_Clear_TX();															//clear TX fifo
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to set transmission channel
//........................................................................................................................
void nRF_Set_Channel( uint8_t channel )
{
	nRF_Config_Register( RF_CH ,  0x7F & channel );							//save to nRF RF_CH register channel number (0x7f == 0b01111111 is the mask)
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to clear TX fifo
//........................................................................................................................
void nRF_Clear_TX (void)
{
	CSN_LOW;							//make CSN low
	SPI_WriteByte( FLUSH_TX );			//send to nRF FLUSH_TX command what means CLEAR TX FIFO
	CSN_HIGH;							//make CSN high
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to clear RX fifo
//........................................................................................................................
void nRF_Clear_RX (void)
{
	CSN_LOW;							//make CSN low
	SPI_WriteByte( FLUSH_RX );			//send to nRF FLUSH_RX command what means CLEAR RX FIFO
	CSN_HIGH;							//make CSN high
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to set states of datapipes
// first attribute is DataPipe, you must put here name of datapipe tha you want to enable, this name is ERX_Px where x is number of data pipe (see nRF_Init function)
// second attribute is or_on_off, here you put ON or OFF macro. When you put here ON, the choosen data pipe will be turned ON, if put here OFF, datapipe will be turned off
// third and last parameter is ACK_on_or_off, you must put here macros: ACK_ON or ACK_OFF. This parameter turns ACK_ON or turns ACK_OFF
//........................................................................................................................
void nRF_Set_Active_DataPipe_And_ACK (uint8_t DataPipe, uint8_t on_or_off, uint8_t ACK_on_or_off )
{
	uint8_t data = nRF_Read_One_Byte_From_Register(EN_RXADDR);		//read data from EN_RXADDR register save it in data variable

	if(on_or_off == ON) data |= (DataPipe);							//if second attribute is ON, set bit ERX_Px (given by DataPipe parameter)
	else if (on_or_off == OFF) data &= ~(DataPipe);					//else reset this bit

	nRF_Config_Register( EN_RXADDR, data );							//save data variable to EN_RXADDR register

	data = nRF_Read_One_Byte_From_Register(EN__AA);					//read data from EN_AA register and save it in data variable

	if(ACK_on_or_off == ACK_ON) data |= (DataPipe);					//if third parameter is ACK_ON, enable auto ackonwledgements by setting ENAA_Px bit (this same value as ERX_Px)
	else if (ACK_on_or_off == ACK_OFF) data &= ~(DataPipe);			//else turn off auto acknowledgments

	nRF_Config_Register( EN__AA, data );							//save the variable data to EN__AA register
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to send data to another nRF's
// attribute to this function is the pointer to the first byte of transsmision buffer
//........................................................................................................................
void nRF_SendDataToAir( char * data )
{
	if(TX_flag == 1) return;								//if still is in transmitting mode or function isn't used for the first time, return
	TX_flag = 1;											//set TX mode flag to one
	char * wsk = data;										//wsk pointer is equal to the addres of first byte transmission buffer
	uint8_t length;											//declare length variable
	if(disable_dynamic_payload == 1) length = payload_width;//if nRF_Set_PAYLOAD_Width() was used use the length variable is equal to width parameter of tha function
	else length = strlen ( data );						//else length is equal to the size of transmission buffer

	if ( length >= MAXIMUM_PAYLOAD_SIZE ) length = 31;

	nRF_TX_Power_Up();										//turn on transmission mode
	_delay_us(150);
	CSN_LOW;												//make CSN low
	SPI_WriteByte(W_TX_PAYLOAD);							//send to nRF W_TX_PAYLOAD command what means "Write data to TX FIFO"
	while(length--)
	{
		SPI_WriteByte(*wsk++);
	}
	SPI_WriteByte(0);
	CSN_HIGH;												//CSN high
	CE_HIGH;												//start transmision!
	#if USE_IRQ == 0

	uint8_t data1 = 0;

	do
	{
		CSN_LOW;											//make CSN low
		data1 = SPI_WriteReadByte(NOP);						//send to nRF dummy byte and read STATUS register
		CSN_HIGH;											//make CSN high
	}while(!(( data1 & (1<<TX_DS))||( data1 & (1<<MAX_RT))));



	if ( ( data1 & (1<<MAX_RT) ) )							//if maximum ammount of retransmissions is achived,
	{
		CE_LOW;													//make CE low												//clear TX FIFO
		nRF_Config_Register(STATUS, data1 | (1<<MAX_RT) | (1<<TX_DS) );	//reset TX_DS and MAX_RT intterrupt flags in STATUS register
		nRF_Clear_TX();
		set_first_time_into_RX_MODE_after_TX_MODE = 1;
		TX_flag = 0;
	}
															//else if was send or funcion is used for the first time
	if ( ( data1 & (1<<TX_DS)) )
	{
		CE_LOW;
		set_first_time_into_RX_MODE_after_TX_MODE = 1;
		TX_flag = 0;										//TX flag = 0
		nRF_Config_Register(STATUS, data1 | (1<<TX_DS));		//clear TX_DS int flag
	}

	#endif
}
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to set payload width
// first attribute of this function is number or RX payload (RX_PW_Px), second one is the width(1-32 bytes)
// use this function only if the dyanmic payloads are turned off
//........................................................................................................................
void nRF_Set_PAYLOAD_Width ( uint8_t payload, uint8_t width )
{
	nRF_Config_Register( payload, ( 0x1F & width ) );		//config RX_PW_Px with number of bytes
	disable_dynamic_payload = 1;							//set the disable_dynamic_payload flag
	payload_width = width;									//global variable payload_width is equal to width given by attribute
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to number of retransmissions and time beetwen each retransmission
// first attribute is WAIT_XXXX_uS and the second is RETR_X_TIMES (see nRF_memory_map.h)
//........................................................................................................................
void nRF_Set_Retransmission_Time_And_Ammount (uint8_t time, uint8_t ammount)
{
	nRF_Config_Register( SETUP_RETR, ( time | ammount ) );		//write to SETUP_RETR register number of retransmisions and time beetwen them
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to set transmission speed and receiver power
// first attribute is TRANS_SPEED_x and the second is RF_PWR_x (see nRF_memory_map.h)
//........................................................................................................................
void nRF_Set_Data_Speed_And_Reciver_Power(uint8_t Data_rate, uint8_t power)
{
	nRF_Config_Register( RF_SETUP, ( Data_rate | power ));	 	//write to RF_SETUP register transmision speed and and receiver power
}

//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// function used to set dynamic payloads
// first attribute is DPL_Px (number of receiver payload) and the second is ON or OFF(see nRF_memory_map.h)
//........................................................................................................................
void nRF_Set_Dynamic_Payload_State_On_Data_Pipe(uint8_t data_pipe_number, uint8_t on_off )
{
	nRF_Config_Register( DYNPD, data_pipe_number );					//save DPL_Px value to DYNPD register
	uint8_t feature = nRF_Read_One_Byte_From_Register(FEATURE);		//make copy of FEATURE register

	if (on_off == ON)	feature |= (1<<EN_DPL);		//if second attribute is equal to ON
														//set bit EN_DPL
	if (on_off == OFF)	feature &= ~(1<<EN_DPL);	//if second attribute is equal to OFF
														//reset bit EN_DPL
	nRF_Config_Register(FEATURE, feature);			//save feature variable to FEATURE register
}


#if USE_IRQ == 1
//'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''//
//         IRQ INTERRUPT HANDLER section					                   //
//.............................................................................//

//in this function you must put your initialization of external intterup (INTx or PCINTx)
//if you don't use interrupts there is no matter what code is here

void Initialize_INTERRUPT_For_nRF(void)
{
#if ( defined (__AVR_ATmega32__) || defined (__AVR_ATmega16__) )
	GICR   |= (1<<INT2);              //enable INT2 intterupt source in GICR register
	MCUCSR |= (0<<ISC2);              //set intterupt active on falling edge

#elif ( defined(__AVR_ATmega328P__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168P__))
	EIMSK   |= (1<<INT0);              //enable INT0 intterupt source in EIMSK register
	EICRA |= (1<<ISC01);              //set intterupt active on falling edge
#endif

}

//'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// Here is external intterupt handler, if you are using interupts change the name of the vector interrupt ore live the same if you are using the same
//...................................................................................................................................................
ISR(INT0_vect)
{
	CSN_LOW;											//Chip select LOW
	register uint8_t status = SPI_WriteReadByte(NOP);	//read STATUS register value
	CSN_HIGH;											//chip select high

	if ( (status & (1<<RX_DR)) )						//id data recived
	{
		RX_flag = 1;									//RX flag equals to 1
	}

	if ( status & (1<<TX_DS) )							//if data was send
	{
		status |= (1<<TX_DS);							//clear TX_DS bit in status variable
		nRF_Config_Register( STATUS, status );			//save status variable in STATUS register
		TX_flag = 0;									//TX_flag = 0
		set_first_time_into_RX_MODE_after_TX_MODE = 1;
		CE_LOW;
	}

	if ( status & (1<<MAX_RT) )							//id max of retransmissions was achived
	{
		status |= (1<<MAX_RT) | (1<<TX_DS);				//clear MAX_RT and TX_DS bits in status variable
		nRF_Config_Register( STATUS, status );			//save variable status to the STATUS register
		TX_flag = 0;									//clear TX_flag
		set_first_time_into_RX_MODE_after_TX_MODE = 1;
		nRF_Clear_TX();
		CE_LOW;
	}
}

#endif













