#ifndef SERIAL_H
#define SERIAL_H

void serial_init();
void serial_shutdown();
unsigned char rx_is_char_waiting();
unsigned char rx_from_buf();
void tx_to_buf(unsigned char u8Byte);

void tx_think();

#define SERIAL_INT_ENABLE() UCSR0B |= (1 << RXCIE0) /* Enable the USART Recieve Complete interrupt (USART_RXC) */
#define SERIAL_INT_DISABLE() UCSR0B &= ~(1 << RXCIE0) /* Disable the USART Recieve Complete interrupt (USART_RXC) */

#endif // SERIAL_H
