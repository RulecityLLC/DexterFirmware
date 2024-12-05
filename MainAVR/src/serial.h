#ifndef SERIAL_H
#define SERIAL_H

#define USART_BAUDRATE 115200

void serial_init();
void serial_shutdown();
void send_byte_now(unsigned char ch);
void debug_send(const char *s);	// don't use this for production!
unsigned char rx_is_char_waiting();
unsigned char rx_from_buf();
void tx_to_buf(unsigned char u8Byte);	// NOTE : this _will_ enable tx interrupt
uint8_t tx_buf_is_not_empty();	// returns non-zero if the tx buffer is not empty

extern volatile unsigned char g_u8TxBufCount;

// uncomment for production, comment out for debugging since TX interrupts mess up stepping through code
#ifndef DEBUG
#define TX_USE_INT
#endif

#ifdef TX_USE_INT

 // Enable the USART Data Register Empty interrupt (if data is available in TX buffer, the ISR _must_ not be enabled if TX buffer is empty)
#define TX_INT_ENABLE() if (g_u8TxBufCount != 0) UCSR0B |= (1 << UDRIE0)

 // Disable the USART Data Register Empty interrupt
#define TX_INT_DISABLE() UCSR0B &= ~(1 << UDRIE0);

#else
#define TX_INT_ENABLE()
#define TX_INT_DISABLE()

void tx_think();

#endif // TX_USE_INT

#endif // SERIAL_H
