#include "..//main.h"
#include "rs485Channel.h"

volatile static uint8_t __RS485_PAUSE_FOR_NEXT_REQUEST__ = 2;

volatile static uint16_t uiSysRS485SendRequestTimer = 5;
volatile static uint16_t uiSysRS485ReciverTimer = 0;

volatile static uint8_t ucRS485txrxBuffer[260];

volatile static uint8_t ucRS485txState;
volatile static uint8_t *ucRS485txBuffer;
volatile static uint8_t ucRS485txBufferLenght;

volatile static uint8_t ucRS485rxState;
volatile static uint8_t *ucRS485rxBuffer;

volatile static uint16_t rs485ReciverTimerOld = 0;

volatile static uint8_t ucChannelCount = 0;
volatile static uint8_t ucRS485ChannelIndex = 0;
volatile static uint8_t ucRS485ChannelCounter = 0;
volatile static LP_OBJ_RS485_CHANNEL lpArrRS485Channel[ 50 ];

volatile static uint8_t usRS485PortAsMaster = 0;
volatile static uint8_t errorFlag = 0;

volatile uint8_t usRS485Port0Led = 0;

void rs485Task( void )
{
	{
		if( !ucChannelCount || !usRS485PortAsMaster )
		{
			return;
		}

		if( !uiSysRS485SendRequestTimer && !uiSysRS485ReciverTimer )
		{
			ucRS485ChannelIndex = ucRS485ChannelCounter;
			if( ++ucRS485ChannelCounter >= ucChannelCount )
			{
				ucRS485ChannelCounter = 0;
				if( !errorFlag )
				{
					usRS485Port0Led ^= 1;
				} else {
					usRS485Port0Led = 1;
					errorFlag = 0;
				}
			}

			if( NULL == lpArrRS485Channel[ ucRS485ChannelIndex ] )
			{
				return;
			}

			if( !lpArrRS485Channel[ ucRS485ChannelIndex ]->ucEnableRequest )
			{
				return;
			}

			lpArrRS485Channel[ ucRS485ChannelIndex ]->ucFlag = 0;
			lpArrRS485Channel[ ucRS485ChannelIndex ]->rs485SetUartSetings( lpArrRS485Channel[ ucRS485ChannelIndex ]->lpObject );
			lpArrRS485Channel[ ucRS485ChannelIndex ]->rs485SendRequestFunc( lpArrRS485Channel[ ucRS485ChannelIndex ]->lpObject );

			uiSysRS485ReciverTimer		= lpArrRS485Channel[ ucRS485ChannelIndex ]->msReadTimeOut;
			uiSysRS485SendRequestTimer	= lpArrRS485Channel[ ucRS485ChannelIndex ]->msReadTimeOut + __RS485_PAUSE_FOR_NEXT_REQUEST__;

			//tx_counter++;
			//tx_counter = tx_counter;
		}

		if( uiSysRS485ReciverTimer )
		{
			if( ucRS485txState == ucRS485txBufferLenght && ucRS485rxState )
			{
				rs485ReciverTimerOld = uiSysRS485ReciverTimer;

				if( lpArrRS485Channel[ ucRS485ChannelIndex ]->rs485GetResponseFunc( lpArrRS485Channel[ ucRS485ChannelIndex ]->lpObject, (uint8_t*)ucRS485rxBuffer, ucRS485rxState ) )
				{
					lpArrRS485Channel[ ucRS485ChannelIndex ]->rs485ClrTimeOutError( lpArrRS485Channel[ ucRS485ChannelIndex ]->lpObject );
					lpArrRS485Channel[ ucRS485ChannelIndex ]->rxErrorCounter = 0;
					lpArrRS485Channel[ ucRS485ChannelIndex ]->ucFlag = 1; // OK
					uiSysRS485SendRequestTimer = __RS485_PAUSE_FOR_NEXT_REQUEST__;
					uiSysRS485ReciverTimer = 0;
					//rx_counter++;
					//rx_counter = rx_counter;
				}
			}
		}
		else
		{
			lpArrRS485Channel[ ucRS485ChannelIndex ]->rs485RestoreUartSetings( lpArrRS485Channel[ ucRS485ChannelIndex ]->lpObject );

			if( !lpArrRS485Channel[ ucRS485ChannelIndex ]->ucFlag )
			{
				lpArrRS485Channel[ ucRS485ChannelIndex ]->ucFlag = 2; // Timeout

				if( lpArrRS485Channel[ ucRS485ChannelIndex ]->rxErrorCounter < 5 )
				{
					++lpArrRS485Channel[ ucRS485ChannelIndex ]->rxErrorCounter;
				}
				else
				{
					lpArrRS485Channel[ ucRS485ChannelIndex ]->rs485SetTimeOutError( lpArrRS485Channel[ ucRS485ChannelIndex ]->lpObject );
					errorFlag = 1;
				}
			}
		}
	}
}

void rs485TaskEnable( void )
{
	usRS485PortAsMaster = 1;
}

void rs485TaskDisable( void )
{
	usRS485PortAsMaster = 0;
}

uint8_t rs485TaskIsEnable( void )
{
	return usRS485PortAsMaster;
}

void rs485TaskInit( void )
{
	uint8_t i;

	rs485TaskDisable();

	ucRS485txBuffer = ucRS485txrxBuffer;
	ucRS485rxBuffer = ucRS485txrxBuffer;

	ucChannelCount = 0;
	for( i = 0; i < len_of_array( lpArrRS485Channel ); i++ ) {
		lpArrRS485Channel[i] = NULL;
	}
}

void rs485TimerIsr( void )
{
	if( uiSysRS485SendRequestTimer ) {
		--uiSysRS485SendRequestTimer;
	}

	if( uiSysRS485ReciverTimer ) {
		--uiSysRS485ReciverTimer;
	}
}

uint8_t rs485AddChannel( LP_OBJ_RS485_CHANNEL lpChannel )
{
	lpArrRS485Channel[ ucChannelCount ] = lpChannel;

	if( ++ucChannelCount > len_of_array( lpArrRS485Channel ) ) {
		ucChannelCount = len_of_array( lpArrRS485Channel );
		return 0;
	}

	return 1;
}

void rs485ChannelDefInit( LP_OBJ_RS485_CHANNEL lpChannel )
{
	lpChannel->lpObject = NULL;

	lpChannel->msReadTimeOut = 100;
	lpChannel->ucEnableRequest = 0;
	lpChannel->rxErrorCounter = 0;
	lpChannel->ucFlag = 0;

	lpChannel->rs485SetUartSetings = rs485SetUartSetingsNullFunc;
	lpChannel->rs485RestoreUartSetings = rs485RestoreUartSetingsNullFunc;

	lpChannel->rs485SendRequestFunc = rs485sendRequestNullFunc;
	lpChannel->rs485GetResponseFunc = rs485getResponseNullFunc;

	lpChannel->rs485SetTimeOutError = rs485SetTimeOutErrorNullFunc;
	lpChannel->rs485ClrTimeOutError = rs485ClrTimeOutErrorNullFunc;
}

void rs485sendRequestNullFunc( void *lpObject )
{
}

uint8_t rs485getResponseNullFunc( void *lpObject, uint8_t *rxBuffer, uint8_t len )
{
	return 1;
}

void rs485SetUartSetingsNullFunc( void *lpObject )
{
}

void rs485RestoreUartSetingsNullFunc( void *lpObject )
{
}

void rs485SetTimeOutErrorNullFunc( void *lpObject )
{
}

void rs485ClrTimeOutErrorNullFunc( void *lpObject )
{
}

void rs485SendBuffer( uint8_t *buffer, uint8_t len )
{
	memcpy( (char*)ucRS485txBuffer, buffer, len );

	ucRS485txBufferLenght = len;
	ucRS485rxState = ucRS485txState = 0;

	UCSR0B &= ~( _BV( RXEN0 ) |  _BV( RXCIE0 ) );
	enable_rs485_transmit();

	UDR0 = ucRS485txBuffer[ ucRS485txState ];
}

ISR( SIG_UART_TX )
{
	if( !usRS485PortAsMaster )
	{
		disable_rs485_transmit();
	} else {
		if( ++ucRS485txState != ucRS485txBufferLenght ) {
			UDR0 = ucRS485txBuffer[ ucRS485txState ];
		} else {
			disable_rs485_transmit();
			UCSR0B |= _BV( RXEN0 ) | _BV( RXCIE0 );

			ucRS485txState = ucRS485txBufferLenght = 0;
		}
	}
}

ISR( SIG_UART_RX )
{
	if( !usRS485PortAsMaster )
	{
		pxMBFrameCBByteReceived();
	} else {
		volatile char c = UDR0;

		if( ucRS485rxState < 260 ) {
			ucRS485rxBuffer[ ucRS485rxState ] = c;

			++ucRS485rxState;
		} else {
			ucRS485rxState = 0;
		}
	}
}

ISR( SIG_UART_UDRE )
{
	if( !usRS485PortAsMaster )
	{
		pxMBFrameCBTransmitterEmpty();
	}
}

void initRS485( unsigned long ulBaudRate, uint8_t ucDataBits, uint8_t ucStopBits, uint8_t ucParity )
{
	if( usRS485PortAsMaster ) {
		disable_rs485_transmit();
		
		UCSR0A = 0;
		UCSR0B = 0;
		UCSR0C = 0;

		switch( ulBaudRate ) {
		case 2400: ulBaudRate = 416; break;
		case 4800: ulBaudRate = 207; break;
		case 9600: ulBaudRate = 103; break;
		case 14400: ulBaudRate = 68; break;
		case 19200: ulBaudRate = 51; break;
		case 28800: ulBaudRate = 34; break;
		case 38400: ulBaudRate = 25; break;
		case 57600: ulBaudRate = 16; break;
		case 76800: ulBaudRate = 12; break;
		case 115200: ulBaudRate = 8; break;
		//case 115200: UCSR0C |= 1<<UMSEL0; UCSR0A |= 1<<U2X0; ulBaudRate = 16; break;
		case 230400: ulBaudRate = 3; break;
		case 250000: ulBaudRate = 3; break;
		case 500000: ulBaudRate = 1; break;
		case 1000000: ulBaudRate = 0; break;
		//default: ulBaudRate = UART_BAUD_CALC( ulBaudRate, F_CPU );
		}

		UBRR0H = ulBaudRate>>8;
		UBRR0L = ulBaudRate;

		switch ( ucDataBits ) {
		case 8:
			UCSR0C |= _BV( UCSZ01 ) | _BV( UCSZ00 );
		 break;
		case 7:
			UCSR0C |= _BV( UCSZ01 );
		 break;
		}

		switch ( ucStopBits ) {
		case 1: 
		 break;
		case 2:
			UCSR0C |= _BV( USBS0 );
		 break;
		}

		switch ( ucParity ) {
		case 0: // PAR_NONE:
		 break;

		case 1: // PAR_ODD
			UCSR0C |= _BV( UPM01 ) | _BV( UPM00 );
		 break;

		case 2: // PAR_EVEN
			UCSR0C |= _BV( UPM01 );
		 break;
		}

		UCSR0B = _BV( TXEN0 ) | _BV( TXCIE0 );
	}
}
