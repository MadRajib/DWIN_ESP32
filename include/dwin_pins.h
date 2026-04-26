#ifndef DWIN_PINS_H
#define DWIN_PINS_H

// UART pins
#ifndef DWIN_UART_RX
#define DWIN_UART_RX        16
#endif

#ifndef DWIN_UART_TX
#define DWIN_UART_TX        17
#endif

// Encoder pins
#ifndef DWIN_ENCODER_CLK
#define DWIN_ENCODER_CLK    25
#endif

#ifndef DWIN_ENCODER_DT
#define DWIN_ENCODER_DT     26
#endif

#ifndef DWIN_ENCODER_BTN
#define DWIN_ENCODER_BTN    27
#endif

// UART configuration
#ifndef DWIN_UART_NUM
#define DWIN_UART_NUM       2
#endif

#ifndef DWIN_BAUD_RATE
#define DWIN_BAUD_RATE      115200
#endif

// Display Dimensions
#ifndef DWIN_WIDTH
#define DWIN_WIDTH          480
#endif

#ifndef DWIN_HEIGHT
#define DWIN_HEIGHT         270
#endif

#ifndef DWIN_UART_READ_TIMEOUT_MS
#define DIWN_UART_READ_TIMEOUT_MS 1000
#endif 

#endif // DWIN_PINS_H