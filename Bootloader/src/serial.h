#ifndef SERIAL_H
#define SERIAL_H

void serial_init();
unsigned char rx_is_char_waiting();
unsigned char rx_from_buf();
void tx_to_buf(unsigned char u8Byte);

void tx_think();

extern volatile unsigned char *g_pTxBufStart;
extern volatile unsigned char *g_pTxBufEnd;

#define SERIAL_SHUTDOWN() UCSR0B &= ~(1 << RXCIE0); /* Disable the USART Recieve Complete interrupt (USART_RXC) */

#endif // SERIAL_H
