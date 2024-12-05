#include <avr/io.h> 
#include <avr/interrupt.h>
#include "common.h"
#include "serial2.h"

//////////////////////////////////////////////////

// uncomment for production, comment out for debugging since TX2 interrupts mess up stepping through code
#ifndef DEBUG
#define TX2_USE_INT
#endif

#ifdef TX2_USE_INT

 // Enable the USART Data Register Empty interrupt (if data is available in TX buffer, the ISR _must_ not be enabled if TX buffer is empty)
#define TX2_INT_ENABLE_IF_SUPPORTED() if (g_u8Tx2BufCount != 0) UCSR1B |= (1 << UDRIE1)

 // Disable the USART Data Register Empty interrupt
#define TX2_INT_DISABLE_IF_SUPPORTED() UCSR1B &= ~(1 << UDRIE1);

#else

#define TX2_INT_ENABLE_IF_SUPPORTED()
#define TX2_INT_DISABLE_IF_SUPPORTED()

#endif // TX_USE_INT

//////////////////////////////////////////////////

// buffer for receiving serial port characters
// should be big enough to handle small requests and log messages
volatile unsigned char g_rx2_buf[20];
volatile unsigned char *g_pRx2BufStart = 0;
volatile unsigned char *g_pRx2BufEnd = 0;

// pointer to the last valid byte of data in the array (to make wrapping around easier)
volatile const unsigned char *g_pRx2BufLastGoodPtr = (g_rx2_buf + sizeof(g_rx2_buf) - 1);

// serial port transmit buffer
// (here so we can queue up data instead of sending it immediately; helpful to ensure AUX AVR doesn't miss it)
volatile unsigned char g_tx2_buf[25];	// should be as small as possible; right now VBI packets + build number query seems to be the biggest blob likely to be sent
volatile unsigned char *g_pTx2BufStart = 0;
volatile unsigned char *g_pTx2BufEnd = 0;
volatile const unsigned char *g_pTx2BufLastGoodPtr = (g_tx2_buf + sizeof(g_tx2_buf) - 1);

// how many bytes are in the tx buffer;  so we know when to disable the interrupt
volatile uint8_t g_u8Tx2BufCount = 0;

// Whether TX is enabled or not (can be disabled temporarily to help with AUX AVR flow control)
// We want it enabled by default because most ldp types will never disable it.
static uint8_t g_bTx2Enabled = 1;

#define RX2_WRAP(var) 	if (var > g_pRx2BufLastGoodPtr) var = g_rx2_buf;
#define TX2_WRAP(var) 	if (var > g_pTx2BufLastGoodPtr) var = g_tx2_buf;

void serial2_init(uint32_t u32BitsPerSecond)
{
	uint16_t u16Prescale = (((F_CPU / (u32BitsPerSecond << 4))) - 1);

	// set up receive buffer pointers
	g_pRx2BufStart = g_rx2_buf;
	g_pRx2BufEnd = g_rx2_buf;

	// set up transmit buffer pointers
	g_pTx2BufStart = g_tx2_buf;
	g_pTx2BufEnd = g_tx2_buf;

	UCSR1B |= (1 << RXEN1) | (1 << TXEN1);   // enable transmit and receive
	UCSR1C |= (1 << UCSZ10) | (1 << UCSZ11); // Use 8-bit character sizes 

	UBRR1L = u16Prescale; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register 
	UBRR1H = (u16Prescale >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register 

	TX2_INT_DISABLE_IF_SUPPORTED();	// don't enable until the tx2 buffer has something in it

	UCSR1B |= (1 << RXCIE1); // Enable the USART Recieve Complete interrupt (USART_RXC)
}

void serial2_shutdown()
{
	TX2_INT_DISABLE_IF_SUPPORTED();

	UCSR1B &= ~(1 << RXCIE1); // Disable the USART Recieve Complete interrupt (USART_RXC)

	UCSR1B &= ~((1 << RXEN1) | (1 << TXEN1));   // disable transmit and receive

	// put TX1/RX2 pins in input mode so they don't send out anything
	DDRD &= ~((1 << PD2)|(1 << PD3));

	// disable pull-ups (if enabled)
	PORTD &= ~((1 << PD2)|(1 << PD3));
}

void tx2_enable(uint8_t bEnabled)
{
	if (bEnabled)
	{
		TX2_INT_ENABLE_IF_SUPPORTED();
	}
	else
	{
		TX2_INT_DISABLE_IF_SUPPORTED();
	}

	g_bTx2Enabled = bEnabled;
}

inline void tx2_to_buf(unsigned char u8Byte)
{
#ifdef TX2_USE_INT

	// if buffer is full, then we have to block, otherwise things like firmware updates will fail because buffer will get clobbered.
	while (g_u8Tx2BufCount == (sizeof(g_tx2_buf) - 1));

	// disable interrupts since we will be modifying shared data (g_pTxBufEnd, etc)
	// (if interrupts are already disabled due to the ISR disabling them, that is ok)
	TX2_INT_DISABLE_IF_SUPPORTED();

	*g_pTx2BufEnd = u8Byte;
	g_pTx2BufEnd++;
	TX2_WRAP(g_pTx2BufEnd);

	g_u8Tx2BufCount++;

	// we only want to turn on interrupts if TX2 is enabled; otherwise we'd start sending stuff when we shouldn't
	if (g_bTx2Enabled)
	{
		// enable interrupts since now the buffer is not empty.
		// This must happen since if interrupts are not enabled here, the data will never be transmitted.
		TX2_INT_ENABLE_IF_SUPPORTED();
	}

#else

	// if not using TX interrupts, it means we are most likely debugging, so keep it simple and just block until byte can be sent

	while ((UCSR1A & (1 << UDRE1)) == 0);
	UDR1 = u8Byte;

#endif
}

inline unsigned char rx2_is_char_waiting()
{
	// TODO : do we need to disable RX ISR here since they access shared data?
	return (g_pRx2BufEnd != g_pRx2BufStart);
}

unsigned char rx2_from_buf()
{
	// assumption: rx_is_char_waiting has already been called and returned true
	unsigned char u8Res = *g_pRx2BufStart;
	g_pRx2BufStart++;

	// wraparound if needed
	RX2_WRAP(g_pRx2BufStart);

	return u8Res;
}

ISR(USART1_RX_vect) 
{ 
	// store received in our buffer so it can be processed later
	*g_pRx2BufEnd = UDR1;
	g_pRx2BufEnd++;

	// wraparound if needed
	RX2_WRAP(g_pRx2BufEnd);

	// We won't check for overflow because our buffer should be big enough that overflow is impossible,
	//  and ISRs should be as short as possible.
}

#ifdef TX2_USE_INT

// This interrupt is triggered when the AVR can TX another byte (when data register is empty)
// This ISR _should_ be freely disabled by any timing critical section of code.
ISR(USART1_UDRE_vect)
{
	// it is assumed for performance reasons that if this ISR is enabled that the TX buffer has at least 1 byte in it.
	// This must be true always or the buffer count will get off and Bad Things will happen.

	UDR1 = (*g_pTx2BufStart);	// should be safe to write to UDR1 first since this ISR cannot be interrupted; this may improve throughput a little bit
	g_pTx2BufStart++;
	TX2_WRAP(g_pTx2BufStart);

	g_u8Tx2BufCount--;

	// if we now have no bytes left in the buffer, then we must disable this ISR so it doesn't get called
	if (g_u8Tx2BufCount == 0)
	{
		// this int will be re-enabled when our tx buffer is written to
		TX2_INT_DISABLE_IF_SUPPORTED();
	}
}

#endif // TX2_USE_INT
