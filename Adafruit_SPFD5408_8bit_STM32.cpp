// IMPORTANT: LIBRARY MUST BE SPECIFICALLY CONFIGURED FOR EITHER TFT SHIELD
// OR BREAKOUT BOARD USAGE.  SEE RELEVANT COMMENTS IN Adafruit_SPFD5408_8bit_STM32.h

// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#include "Adafruit_SPFD5408_8bit_STM32.h"

#define SPFD5408_SOFTRESET			0x01
#define SPFD5408_SLEEPIN			0x10
#define SPFD5408_SLEEPOUT			0x11
#define SPFD5408_NORMALDISP			0x13
#define SPFD5408_INVERTOFF			0x20
#define SPFD5408_INVERTON			0x21
#define SPFD5408_GAMMASET			0x26
#define SPFD5408_DISPLAYOFF			0x28
#define SPFD5408_DISPLAYON			0x29
#define SPFD5408_COLADDRSET			0x2A
#define SPFD5408_PAGEADDRSET		0x2B
#define SPFD5408_MEMORYWRITE		0x2C
#define SPFD5408_PIXELFORMAT		0x3A
#define SPFD5408_FRAMECONTROL		0xB1
#define SPFD5408_DISPLAYFUNC		0xB6
#define SPFD5408_ENTRYMODE			0xB7
#define SPFD5408_POWERCONTROL1		0xC0
#define SPFD5408_POWERCONTROL2		0xC1
#define SPFD5408_VCOMCONTROL1		0xC5
#define SPFD5408_VCOMCONTROL2		0xC7
#define SPFD5408_MEMCONTROL			0x36
#define SPFD5408_MADCTL				0x36
#define SPFD5408_MADCTL_MY			0x80
#define SPFD5408_MADCTL_MX			0x40
#define SPFD5408_MADCTL_MV			0x20
#define SPFD5408_MADCTL_ML			0x10
#define SPFD5408_MADCTL_RGB			0x00
#define SPFD5408_MADCTL_BGR			0x08
#define SPFD5408_MADCTL_MH			0x04
#define SPFD5408_COLADDREND_HI		0x04
#define SPFD5408_COLADDREND_LO		0x05
#define SPFD5408_ROWADDREND_HI		0x08
#define SPFD5408_ROWADDREND_LO		0x09

Adafruit_SPFD5408_8bit_STM32::Adafruit_SPFD5408_8bit_STM32(void)
	: Adafruit_GFX(TFTWIDTH, TFTHEIGHT)
{
	//Set command lines as output
	//Note: CRH and CRL are both 32 bits wide
	//Each pin is represented by 4 bits 0x3 (hex) sets that pin to O/P
	TFT_CNTRL->regs->CRL = (TFT_CNTRL->regs->CRL & 0xFFFF0000) | 0x00003333;
	CS_IDLE; // Set all control bits to HIGH (idle)
	CD_DATA; // Signals are ACTIVE LOW
	WR_IDLE;
	RD_IDLE;
	reset();
	//set up 8 bit parallel port to write mode.
	setWriteDir();
}

/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::begin()
{
	uint8_t i = 0;

	reset();
	delay(200);
	CS_ACTIVE;
	writeRegister8(SPFD5408_SOFTRESET, 0);
	delay(50);
	writeRegister8(SPFD5408_DISPLAYOFF, 0);
	writeRegister8(SPFD5408_POWERCONTROL1, 0x23);
	writeRegister8(SPFD5408_POWERCONTROL2, 0x10);
	writeRegister16(SPFD5408_VCOMCONTROL1, 0x2B2B);
	writeRegister8(SPFD5408_VCOMCONTROL2, 0xC0);
	writeRegister8(SPFD5408_MEMCONTROL, SPFD5408_MADCTL_MY | SPFD5408_MADCTL_BGR);
	writeRegister8(SPFD5408_PIXELFORMAT, 0x55);
	writeRegister16(SPFD5408_FRAMECONTROL, 0x001B);
	writeRegister8(SPFD5408_ENTRYMODE, 0x07);
	writeRegister8(SPFD5408_SLEEPOUT, 0);
	delay(150);
	writeRegister8(SPFD5408_DISPLAYON, 0);
	delay(500);
	
	writeRegister8(SPFD5408_INVERTON, 0);
	setAddrWindow(0, 0, TFTWIDTH - 1, TFTHEIGHT - 1);
}

/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::reset(void)
{
	CS_IDLE;
	WR_IDLE;
	RD_IDLE;

	// toggle RST low to reset
	if (TFT_RST > 0) {
		pinMode(TFT_RST, OUTPUT);
		digitalWrite(TFT_RST, HIGH);
		delay(10);
		digitalWrite(TFT_RST, LOW);
		delay(10);
		digitalWrite(TFT_RST, HIGH);
		delay(100);
	}

	// Data transfer sync
	CS_ACTIVE;
	CD_COMMAND;
	write8(0x00);
	for (uint8_t i = 0; i < 3; i++) WR_STROBE; // Three extra 0x00s
	CS_IDLE;
}

/*****************************************************************************/
// Sets the LCD address window.
// Relevant to rect/screen fills and H/V lines.  Input coordinates are
// assumed pre-sorted (e.g. x2 >= x1).
/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::setAddrWindow(int x1, int y1, int x2, int y2)
{
	uint32_t t;

	CS_ACTIVE;

	t = x1;
	t <<= 16;
	t |= x2;
	writeRegister32(SPFD5408_COLADDRSET, t);
	t = y1;
	t <<= 16;
	t |= y2;
	writeRegister32(SPFD5408_PAGEADDRSET, t);

	CS_IDLE;
}

/*****************************************************************************/
// Fast block fill operation for fillScreen, fillRect, H/V line, etc.
// Requires setAddrWindow() has previously been called to set the fill
// bounds.  'len' is inclusive, MUST be >= 1.
/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::flood(uint16_t color, uint32_t len)
{
	uint16_t blocks;
	uint8_t  i, hi = color >> 8,
		lo = color;

	CS_ACTIVE;
	CD_COMMAND;
	write8(SPFD5408_MEMORYWRITE);

	// Write first pixel normally, decrement counter by 1
	CD_DATA;
	write8(hi);
	write8(lo);
	len--;

	blocks = (uint16_t)(len / 64); // 64 pixels/block

	if (hi == lo) {
		// High and low bytes are identical.  Leave prior data
		// on the port(s) and just toggle the write strobe.
		while (blocks--) {
			i = 16; // 64 pixels/block / 4 pixels/pass
			do {
				WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // 2 bytes/pixel
				WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // x 4 pixels
			} while (--i);
		}
		// Fill any remaining pixels (1 to 64)
		for (i = (uint8_t)len & 63; i--; ) {
			WR_STROBE;
			WR_STROBE;
		}
	}
	else {
		while (blocks--) {
			i = 16; // 64 pixels/block / 4 pixels/pass
			do {
				write8(hi); write8(lo); write8(hi); write8(lo);
				write8(hi); write8(lo); write8(hi); write8(lo);
			} while (--i);
		}
		for (i = (uint8_t)len & 63; i--; ) {
			write8(hi);
			write8(lo);
		}
	}
	CS_IDLE;
}

/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::drawFastHLine(int16_t x, int16_t y, int16_t length, uint16_t color)
{
	int16_t x2;

	// Initial off-screen clipping
	if ((length <= 0) ||
		(y < 0) || (y >= TFTHEIGHT) ||
		(x >= TFTWIDTH) || ((x2 = (x + length - 1)) < 0)) return;

	if (x < 0) {        // Clip left
		length += x;
		x = 0;
	}
	if (x2 >= TFTWIDTH) { // Clip right
		x2 = TFTWIDTH - 1;
		length = x2 - x + 1;
	}

	setAddrWindow(x, y, x2, y);
	flood(color, length);
	setLR();
}

/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::drawFastVLine(int16_t x, int16_t y, int16_t length, uint16_t color)
{
	int16_t y2;

	// Initial off-screen clipping
	if ((length <= 0) ||
		(x < 0) || (x >= TFTWIDTH) ||
		(y >= TFTHEIGHT) || ((y2 = (y + length - 1)) < 0)) return;
	if (y < 0) {         // Clip top
		length += y;
		y = 0;
	}
	if (y2 >= TFTHEIGHT) { // Clip bottom
		y2 = TFTHEIGHT - 1;
		length = y2 - y + 1;
	}

	setAddrWindow(x, y, x, y2);
	flood(color, length);
	setLR();
}

/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, uint16_t fillcolor)
{
	int16_t  x2, y2;

	// Initial off-screen clipping
	if ((w <= 0) || (h <= 0) ||
		(x1 >= TFTWIDTH) || (y1 >= TFTHEIGHT) ||
		((x2 = x1 + w - 1) < 0) || ((y2 = y1 + h - 1) < 0)) return;
	if (x1 < 0) { // Clip left
		w += x1;
		x1 = 0;
	}
	if (y1 < 0) { // Clip top
		h += y1;
		y1 = 0;
	}
	if (x2 >= TFTWIDTH) { // Clip right
		x2 = TFTWIDTH - 1;
		w = x2 - x1 + 1;
	}
	if (y2 >= TFTHEIGHT) { // Clip bottom
		y2 = TFTHEIGHT - 1;
		h = y2 - y1 + 1;
	}

	setAddrWindow(x1, y1, x2, y2);
	flood(fillcolor, (uint32_t)w * (uint32_t)h);
	setLR();
}

/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::fillScreen(uint16_t color)
{
	setAddrWindow(0, 0, TFTWIDTH - 1, TFTHEIGHT - 1);
	flood(color, (long)TFTWIDTH * (long)TFTHEIGHT);
}

/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	// Clip
	if ((x < 0) || (y < 0) || (x >= TFTWIDTH) || (y >= TFTHEIGHT)) return;

	CS_ACTIVE;

	setAddrWindow(x, y, TFTWIDTH - 1, TFTHEIGHT - 1);
	writeRegister16(0x2C, color);
	CS_ACTIVE;
	CD_COMMAND;
	write8(0x2C);
	CD_DATA;
	write8(color >> 8); write8(color);

	CS_IDLE;
}

/*****************************************************************************/
// Issues 'raw' an array of 16-bit color values to the LCD; used
// externally by BMP examples.  Assumes that setWindowAddr() has
// previously been set to define the bounds.  Max 255 pixels at
// a time (BMP examples read in small chunks due to limited RAM).
/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::pushColors(uint16_t *data, uint8_t len, boolean first)
{
	uint16_t color;
	uint8_t  hi, lo;
	CS_ACTIVE;
	if (first == true) { // Issue GRAM write command only on first call
		CD_COMMAND;
		write8(SPFD5408_MEMORYWRITE);
	}
	CD_DATA;
	while (len--) {
		color = *data++;
		hi = color >> 8; // Don't simplify or merge these
		lo = color;      // lines, there's macro shenanigans
		write8(hi);         // going on.
		write8(lo);
	}
	CS_IDLE;
}

/*****************************************************************************/
// Pass 8-bit (each) R,G,B, get back 16-bit packed color
/*****************************************************************************/
uint16_t Adafruit_SPFD5408_8bit_STM32::color565(uint8_t r, uint8_t g, uint8_t b)
{
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/*****************************************************************************/
void Adafruit_SPFD5408_8bit_STM32::setRotation(uint8_t x)
{
	// Call parent rotation func first -- sets up rotation flags, etc.
	Adafruit_GFX::setRotation(x);
	// Then perform hardware-specific rotation operations...

	CS_ACTIVE;
	uint16_t t;

	switch (rotation) {
	case 2:
		t = SPFD5408_MADCTL_MX | SPFD5408_MADCTL_BGR;
		break;
	case 3:
		t = SPFD5408_MADCTL_MV | SPFD5408_MADCTL_BGR;
		break;
	case 0:
		t = SPFD5408_MADCTL_MY | SPFD5408_MADCTL_BGR;
		break;
	case 1:
		t = SPFD5408_MADCTL_MX | SPFD5408_MADCTL_MY | SPFD5408_MADCTL_MV | SPFD5408_MADCTL_BGR;
		break;
	}
	writeRegister8(SPFD5408_MADCTL, t); // MADCTL
	setAddrWindow(0, 0, TFTWIDTH - 1, TFTHEIGHT - 1); // CS_IDLE happens here
}

/*****************************************************************************/
uint8_t read8_(void)
{
	RD_ACTIVE;
	delayMicroseconds(10);
	uint8_t temp = ((TFT_DATA->regs->IDR >> TFT_DATA_NIBBLE) & 0x00FF);
	delayMicroseconds(1);
	RD_IDLE;
	delayMicroseconds(1);
	return temp;
}

// speed optimization
static void writeCommand(uint8_t c) __attribute__((always_inline));
/*****************************************************************************/
static void writeCommand(uint8_t c)
{
	CS_ACTIVE;
	CD_COMMAND;
	write8(c);
}

/*****************************************************************************/
// Because this function is used infrequently, it configures the ports for
// the read operation, reads the data, then restores the ports to the write
// configuration.  Write operations happen a LOT, so it's advantageous to
// leave the ports in that state as a default.
/*****************************************************************************/
uint16_t Adafruit_SPFD5408_8bit_STM32::readPixel(int16_t x, int16_t y)
{
	return 0;
}

/*****************************************************************************/
uint16_t Adafruit_SPFD5408_8bit_STM32::readID(void)
{
	return 0x9341;
}

/*****************************************************************************/
uint16_t readReg(uint8_t r)
{
	uint32_t id;
	uint8_t x;

	// try reading register #4
	writeCommand(r);
	setReadDir();  // Set up LCD data port(s) for READ operations
	CD_DATA;
	delayMicroseconds(50);
	read8(x);
	id = x;          // Do not merge or otherwise simplify
	id <<= 8;              // these lines.  It's an unfortunate
	read8(x);
	id |= x;        // shenanigans that are going on.
	id <<= 8;              // these lines.  It's an unfortunate
	read8(x);
	id |= x;        // shenanigans that are going on.
	id <<= 8;              // these lines.  It's an unfortunate
	read8(x);
	id |= x;       // shenanigans that are going on.
	CS_IDLE;
	setWriteDir();  // Restore LCD data port(s) to WRITE configuration

	//Serial.print("Read $"); Serial.print(r, HEX); 
	//Serial.print(":\t0x"); Serial.println(id, HEX);
	return id;
}

/*****************************************************************************/
void writeRegister8(uint8_t a, uint8_t d)
{
	writeCommand(a);
	CD_DATA;
	write8(d);
	CS_IDLE;
}

/*****************************************************************************/
void writeRegister16(uint16_t a, uint16_t d)
{
	writeCommand(a);
	CD_DATA;
	write8(d >> 8);
	write8(d);
	CS_IDLE;
}

/*****************************************************************************/
void writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d)
{
	writeRegister8(aH, d >> 8);
	writeRegister8(aL, d);
}

/*****************************************************************************/
void writeRegister24(uint8_t r, uint32_t d)
{
	writeCommand(r); // includes CS_ACTIVE
	CD_DATA;
	write8(d >> 16);
	write8(d >> 8);
	write8(d);
	CS_IDLE;
}

/*****************************************************************************/
void writeRegister32(uint8_t r, uint32_t d)
{
	writeCommand(r);
	CD_DATA;
	write8(d >> 24);
	write8(d >> 16);
	write8(d >> 8);
	write8(d);
	CS_IDLE;
}

void setLR(void)
{
	writeRegisterPair(SPFD5408_COLADDREND_HI, SPFD5408_COLADDREND_LO, TFTWIDTH - 1);
	writeRegisterPair(SPFD5408_ROWADDREND_HI, SPFD5408_ROWADDREND_LO, TFTHEIGHT - 1);
}
