// Paint example for a LCD board with touch screen.
// Includes touch calibration feature.
// Uses simple touch to select point, colour.
// Uses double click/touch to clear the background in selected colour.
// Uses double click/touch to set paint radius back to original value.

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_SPFD5408_8bit_STM32.h> // Hardware-specific library
#include <TouchScreen_STM32.h>


// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define LED_PIN PC13

Adafruit_SPFD5408_8bit_STM32 tft;

#define BOX_X 30
#define BOX_Y 40

int penradius;

// test colours
#define X_WHITE		0
#define X_YELLOW	(BOX_X)
#define X_CYAN		(BOX_X*2)
#define X_GREEN		(BOX_X*3)
#define X_MAGENTA	(BOX_X*4)
#define X_RED		(BOX_X*5)
#define X_BLUE		(BOX_X*6)
#define X_BLACK		(BOX_X*7)

int oldcolor, currentcolor;
int old_x, current_x;
/*
#define TFT_RD         PA0
#define TFT_WR         PA1
#define TFT_RS         PA2
#define TFT_CS         PA3
*/
// overlaping:
#define XM TFT_RS // 330 Ohm // must be an analog pin !!!
#define YP TFT_CS // 500 Ohm // must be an analog pin !!!
#define XP PB8 //TFT_D0 // 330 Ohm // can be a digital pin
#define YM PB9 //TFT_D1 // 500 Ohm // can be a digital pin

TouchScreen ts = TouchScreen(XP, YP, XM, YM);

/*****************************************************************************/
void setup(void)
{
	Serial.begin(115200);
	
	delay(1000); // allow time for OS to enumerate USB as COM port
	
	Serial.print("\n*** Paint demo with easy touch calibration process ***\n");


	tft.reset();
	tft.begin();
 
	// Touch screen range setting
	// set display X and Y range of the display screen
	// the returned coordinates will be automatically mapped to these values.
	// If this is not used, the default values form the header file are used.
	ts.rangeSet(TFTWIDTH, TFTHEIGHT);	
							

// Calibration
// needs sequentially touching two, diagonally opposite corners (check serial output!)
// the touching can include small circular movements around the corners
// it will keep calibrating the corner as long as pressure is applied
  tft.fillScreen(WHITE);
  tft.setCursor(0, 30);
  tft.setTextColor(BLACK);  tft.setTextSize(1);
  tft.println("Calibration");
  tft.println("Press one corner...");
	Serial.println("Calibrating the touch surface");
	Serial.print("> please press one corner...");
	ts.calibratePoint();
  tft.println("Now press the opposite corner...");
	Serial.print("ok.\n> and now press the diagonally opposite corner...");
	ts.calibratePoint();
  tft.setTextSize(2);
  tft.setCursor(0, 200);
  tft.println("Calibration done.");
	Serial.println("ok.\nCalibration done.");

	delay(1000);

  tft.fillScreen(BLACK);
	// display test color boxes
	tft.fillRect(X_WHITE, 0, BOX_X, BOX_Y, WHITE);
	tft.fillRect(X_YELLOW, 0, BOX_X, BOX_Y, YELLOW);
	tft.fillRect(X_CYAN, 0, BOX_X, BOX_Y, CYAN);
	tft.fillRect(X_GREEN, 0, BOX_X, BOX_Y, GREEN);
	tft.fillRect(X_MAGENTA, 0, BOX_X, BOX_Y, MAGENTA);
	tft.fillRect(X_RED, 0, BOX_X, BOX_Y, RED);
	tft.fillRect(X_BLUE, 0, BOX_X, BOX_Y, BLUE);
	tft.fillRect(X_BLACK, 0, BOX_X, BOX_Y, BLACK);

  currentcolor = oldcolor = WHITE;
  current_x = old_x = X_WHITE;
  tft.drawRect(X_WHITE, 0, BOX_X, BOX_Y, BLACK); // box selection
  penradius = 3;

  pinMode(LED_PIN, OUTPUT);
}

#define PAINT_MARGIN 10

void loop()
{
	TSPoint p;	// a point object holds x, y and z coordinates

	// check for valid touch
	if ( ts.getPoint(&p) ) { //  returns 'true' if touch is detected, otherwise 'false'
		digitalWrite(LED_PIN, LOW);
		// the coordinates will be mapped to the set display ranges by using 'rangeSet()' in setup
		// x, y (in pixels) can be directly used for display routines!

		if (p.y < BOX_Y) {
			// recover old box frame
			tft.drawRect(old_x, 0, BOX_X, BOX_Y, oldcolor);
			// draw white box frame for the new colour
			if (p.x > X_BLACK) {
				currentcolor = BLACK;
				current_x = X_BLACK;
			} else if (p.x > X_BLUE) {
				currentcolor = BLUE;
				current_x = X_BLUE;
			} else if (p.x > X_RED) {
				currentcolor = RED;
				current_x = X_RED;
			} else if (p.x > X_MAGENTA) {
				currentcolor = MAGENTA;
				current_x = X_MAGENTA;
			} else if (p.x > X_GREEN) {
				currentcolor = GREEN;
				current_x = X_GREEN;
			} else if (p.x > X_CYAN) {
				currentcolor = CYAN;
				current_x = X_CYAN;
			} else if (p.x > X_YELLOW) {
				currentcolor = YELLOW;
				current_x = X_YELLOW;
			} else if (p.x > X_WHITE) {
				currentcolor = WHITE;
				current_x = X_WHITE;
			}
			tft.drawRect(current_x, 0, BOX_X, BOX_Y, (currentcolor^0xFFFF));
			
			oldcolor = currentcolor;
			old_x = current_x;

		}
		// check point position, repetition and double click
		if (((p.y-penradius) > (BOX_Y+PAINT_MARGIN)) && ((p.y+penradius) < tft.height())) {
			// painting area
			if (p.dbl) penradius = 3;
			else if (penradius<p.repeat) penradius = p.repeat;
			tft.fillCircle(p.x, p.y, penradius, currentcolor);
		} else if ( p.y<BOX_Y ) {
			// colour selection area
			if (p.dbl) {
				// fill the writing area with current colour
				tft.fillRect(0, BOX_Y+PAINT_MARGIN, TFTWIDTH, TFTHEIGHT-BOX_Y-PAINT_MARGIN, currentcolor);
			}
		}
		digitalWrite(LED_PIN, HIGH);
	}
}

