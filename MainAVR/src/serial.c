#include <avr/io.h> 
#include <avr/interrupt.h>
#include "common.h"
#include "serial.h"

#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1) 

// buffer for receiving serial port characters
// (should be big enough to handle max # of bytes within a 60 Hz interval, which is exactly 192 at our current speed, 11520bytes/sec / 60fields/sec)
volatile unsigned char g_rx_buf[200];
volatile unsigned char *g_pRxBufStart = 0;
volatile unsigned char *g_pRxBufEnd = 0;
// pointer to the last valid byte of data in the array (to make wrapping around easier)
volatile const unsigned char *g_pRxBufLastGoodPtr = (g_rx_buf + sizeof(g_rx_buf) - 1);

// serial port transmit buffer
volatile unsigned char g_tx_buf[100];	// this doesn't need to be that big but we will need to make sure we don't send tons of outgoing stuff and overflow it
volatile unsigned char *g_pTxBufStart = 0;
volatile unsigned char *g_pTxBufEnd = 0;
volatile const unsigned char *g_pTxBufLastGoodPtr = (g_tx_buf + sizeof(g_tx_buf) - 1);
volatile uint8_t g_u8TxBufCount = 0;	// how many bytes are in the tx buffer

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
	TX_INT_DISABLE();	// don't enable until the tx buffer has something in it
}

void serial_shutdown()
{
	TX_INT_DISABLE();
	UCSR0B &= ~(1 << RXCIE0); // Disable the USART Recieve Complete interrupt (USART_RXC) 
	UCSR0B &= ~((1 << RXEN0) | (1 << TXEN0));   // disable transmit and receive
}

void send_byte_now(unsigned char ch)
{
	while ((UCSR0A & (1 << UDRE0)) == 0) {}; // Do nothing until transmission complete flag set
	UDR0 = ch; // Send out the byte value in the variable "ByteToSend" 
}

void debug_send(const char *s)
{
	int i = 0;
	while (s[i] != 0)
	{
		send_byte_now(s[i]);
		i++;
	}
	send_byte_now(13);
	send_byte_now(10);
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
	// disable interrupts since we will be modifying shared data (g_pTxBufEnd, etc)
	// (if interrupts are already disabled due to the ISR disabling them, that is ok)
	TX_INT_DISABLE();

	*g_pTxBufEnd = u8Byte;
	g_pTxBufEnd++;
	TX_WRAP(g_pTxBufEnd);

	g_u8TxBufCount++;

	// enable interrupts since now the buffer is not empty.
	// This must happen since if interrupts are not enabled here, the data will never be transmitted.
	TX_INT_ENABLE();
}

uint8_t tx_buf_is_not_empty()
{
	return g_u8TxBufCount;
}

ISR(USART0_RX_vect) 
{ 
	// store received in our buffer so it can be processed later
	*g_pRxBufEnd = UDR0;
	g_pRxBufEnd++;

	// wraparound if needed
	RX_WRAP(g_pRxBufEnd);

	// We won't check for overflow because our buffer should be big enough that overflow is impossible,
	//  and ISRs should be as short as possible.
}

#ifdef TX_USE_INT

// This interrupt is triggered when the AVR can TX another byte (when data register is empty)
// This ISR _should_ be freely disabled by any timing critical section of code.
ISR(USART0_UDRE_vect)
{
	// it is assumed for performance reasons that if this ISR is enabled that the TX buffer has at least 1 byte in it.
	// This must be true always or the buffer count will get off and Bad Things will happen.

	UDR0 = (*g_pTxBufStart);	// should be save to write to UDR0 first since this ISR cannot be interrupted; this may improve throughput a little bit
	g_pTxBufStart++;
	TX_WRAP(g_pTxBufStart);

	g_u8TxBufCount--;

	// if we now have no bytes left in the buffer, then we must disable this ISR so it doesn't get called
	if (g_u8TxBufCount == 0)
	{
		// this int will be re-enabled when our tx buffer is written to
		TX_INT_DISABLE();
	}
}

#else

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
			g_u8TxBufCount--;
			g_pTxBufStart++;
			TX_WRAP(g_pTxBufStart);
		}
	}
}

#endif // TX_USE_INT
