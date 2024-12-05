#include <avr/io.h> 
#include <avr/interrupt.h>
#include "serial.h"
#include "platform.h"

#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1) 

// buffer for receiving serial port characters
// (should be big enough to handle a full page of data coming in while our protocol code is busy doing other stuff)
volatile unsigned char g_rx_buf[300];
volatile unsigned char *g_pRxBufStart = 0;
volatile unsigned char *g_pRxBufEnd = 0;
// pointer to the last valid byte of data in the array (to make wrapping around easier)
volatile const unsigned char *g_pRxBufLastGoodPtr = (g_rx_buf + sizeof(g_rx_buf) - 1);

// serial port transmit buffer
volatile unsigned char g_tx_buf[100];	// this doesn't need to be that big but we will need to make sure we don't send tons of outgoing stuff and overflow it
volatile unsigned char *g_pTxBufStart = 0;
volatile unsigned char *g_pTxBufEnd = 0;
volatile const unsigned char *g_pTxBufLastGoodPtr = (g_tx_buf + sizeof(g_tx_buf) - 1);

#define RX_WRAP(var) 	if (var > g_pRxBufLastGoodPtr) var = g_rx_buf;
#define TX_WRAP(var) 	if (var > g_pTxBufLastGoodPtr) var = g_tx_buf;

void serial_init()
{
	// set up receive buffer pointers
	g_pRxBufStart = g_rx_buf;
	g_pRxBufEnd = g_rx_buf;

	// set up transmit buffer pointers
	g_pTxBufStart = g_tx_buf;
	g_pTxBufEnd = g_tx_buf;

	UCSR0B |= (1 << RXEN0) | (1 << TXEN0);   // enable transmit and receive
	UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01); // Use 8-bit character sizes 

	UBRR0L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register 
	UBRR0H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register 

	UCSR0B |= (1 << RXCIE0); // Enable the USART Recieve Complete interrupt (USART_RXC) 
}

unsigned char rx_is_char_waiting()
{
	// TODO : do we need to disable RX ISR here since they access shared data?
	return (g_pRxBufEnd != g_pRxBufStart);
}

unsigned char rx_from_buf()
{
	// assumption: rx_is_char_waiting has already been called and returned true
	unsigned char u8Res = *g_pRxBufStart;
	g_pRxBufStart++;

	// wraparound if needed
	RX_WRAP(g_pRxBufStart);

	return u8Res;
}

void tx_to_buf(unsigned char u8Byte)
{
	*g_pTxBufEnd = u8Byte;
	g_pTxBufEnd++;
	TX_WRAP(g_pTxBufEnd);
}

ISR(RX_INT_vectname) 
{ 
	// store received in our buffer so it can be processed later
	*g_pRxBufEnd = UDR0;
	g_pRxBufEnd++;

	// wraparound if needed
	RX_WRAP(g_pRxBufEnd);

	// We won't check for overflow because our buffer should be big enough that overflow is impossible,
	//  and ISRs should be as short as possible.
}

// Sends 1 byte out to serial port if needed.
// Call this during idle times to clear out transmit buffer.
void tx_think()
{
	// if UART is ready to receive a byte to be transmitted
	if (UCSR0A & (1 << UDRE0))
	{
		// if we have a byte to send
		if (g_pTxBufEnd != g_pTxBufStart)
		{
			UDR0 = (*g_pTxBufStart);	// send to UART
			g_pTxBufStart++;
			TX_WRAP(g_pTxBufStart);
		}
	}
}
