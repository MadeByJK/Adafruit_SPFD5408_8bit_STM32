#Adafruit_SPFD5408_8bit_STM32

### Basic info
This is a library for TFT displays controlled by SPFD5408. It's based on stevstrong's Adafruit_TFTLCD_8bit_STM32(https://github.com/stevstrong/Adafruit_TFTLCD_8bit_STM32), who ported Adafruit's TFTLCD-Library https://github.com/adafruit/TFTLCD-Library to STM32.

To use it, place it in <YourUserFolder>\Documents\Arduino\libraries\ or in your Arduino IDE click the "Sketch" menu and then Include Library => Manage Libraries, then find zip file with library.
More info: https://www.arduino.cc/en/Guide/Libraries
Remember to restart your IDE!

Tested with bluepill(STM32F103C8T6).

### Wiring
#### Control pins

Control pins are defined in Adafruit_SPFD5408_8bit_STM32.h in lines 47-66:

```
#define TFT_CNTRL      GPIOA
#define TFT_RD         PA0
#define TFT_WR         PA1
#define TFT_RS         PA2
#define TFT_CS         PA3
#define TFT_RST        PA15

#define TFT_RD_MASK    BIT0
#define TFT_WR_MASK    BIT1
#define TFT_RS_MASK    BIT2
#define TFT_CS_MASK    BIT3

#define RD_ACTIVE    { TFT_CNTRL->regs->BRR  = TFT_RD_MASK; }
#define RD_IDLE      { TFT_CNTRL->regs->BSRR = TFT_RD_MASK; }
#define WR_ACTIVE    { TFT_CNTRL->regs->BRR  = TFT_WR_MASK; }
#define WR_IDLE      { TFT_CNTRL->regs->BSRR = TFT_WR_MASK; }
#define CD_COMMAND   { TFT_CNTRL->regs->BRR  = TFT_RS_MASK; }
#define CD_DATA      { TFT_CNTRL->regs->BSRR = TFT_RS_MASK; }
#define CS_ACTIVE    { TFT_CNTRL->regs->BRR  = TFT_CS_MASK; }
#define CS_IDLE      { TFT_CNTRL->regs->BSRR = TFT_CS_MASK; }
```

If you want to change TFT_RD, TFT_WR, TFT_RS or TFT_CS, remember to update corresponding TFT_x_MASK and (if using diffrent GPIO) x_ACTIVE and x_IDLE.

#### Data pins
Data pins are defined in Adafruit_SPFD5408_8bit_STM32.h in lines 37-41:

```
#define TFT_DATA GPIOB
// Port data bits D0..D7:
// enable only one from below lines corresponding to your HW setup:
//#define TFT_DATA_NIBBLE 0 // take the lower 8 bits: 0..7
#define TFT_DATA_NIBBLE 8 // take the higher 8 bits: 8..15
```

You can easly toggle lower/higher 8 bits of GPIO uncomenting and commenting lines 40 and 41.
To change GPIO change line 37.

For more info about STM32duino GPIO Registers and programming visit https://gist.github.com/iwalpola/6c36c9573fd322a268ce890a118571ca