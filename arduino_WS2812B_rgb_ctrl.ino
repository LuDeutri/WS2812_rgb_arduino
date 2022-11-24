#include <WS2812FX.h> // https://github.com/kitesurfer1404/WS2812FX
#include <FastLED.h> // https://github.com/FastLED/FastLED

// --------- config ---------

#define NUM_LEDS 60
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define UPDATES_PER_SECOND 60

#define BRIGHTNESS  200
#define RAINBOW_SAME_COLOUR_LEDS 2
#define TIME_PER_COLOUR_SWITCHING 1000 // Time before switching to the next colour
#define TIME_FADE 10
#define TIME_STARTING_ANIMATION 4000
#define TIME_SPECIAL_LIGHT_SHOW 6000
#define FADE_BRIGHTNESS_START 0
#define FADE_BRIGHTNESS_END 200

// Enable led shows and set the order
int stateArr[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22 };
#define ARRAY_LENGTH_STATES 23

int colourPaletteIndex[] = {1, 2, 3, 4}; // Colours in the switching mode

/*
0    = Start Simulation
1    = Constant Red
2    = Constant Green
3    = Constant Blue
4    = Constant Violett
5    = Constant White
6   = Pulse Red
7   = Pulse Green
8   = Pulse Blue
9   = Pulse Violett
10  = Colour Switching
11  = Colour Fader
12  = Rainbow
13  = Fire
14  = Cyclon
15  = Pacific
16  = Eptileptiker
17  = Twinkle
18  = Theatre Rainbow
19  = Chase Rainbow
20  = Firework with random colours
21  = Wipe with diffrent colours
17  = OFF
*/

//  ------- Fire light simulation -----------
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120
//  ------- /Fire light simulation -----------


// ------------- Twinkle light simulation --------------------
// Overall twinkle speed.
// 0 (VERY slow) to 8 (VERY fast).  
// 4, 5, and 6 are recommended, default is 4.
#define TWINKLE_SPEED 4

// Overall twinkle density.
// 0 (NONE lit) to 8 (ALL lit at once).  
// Default is 5.
#define TWINKLE_DENSITY 5

// How often to change color palettes.
#define SECONDS_PER_PALETTE  30
// Also: toward the bottom of the file is an array 
// called "ActivePaletteList" which controls which color
// palettes are used; you can add or remove color palettes
// from there freely.

// Background color for 'unlit' pixels
// Can be set to CRGB::Black if desired.
CRGB gBackgroundColor = CRGB::Black; 
// Example of dim incandescent fairy light background color
// CRGB gBackgroundColor = CRGB(CRGB::FairyLight).nscale8_video(16);

// If AUTO_SELECT_BACKGROUND_COLOR is set to 1,
// then for any palette where the first two entries 
// are the same, a dimmed version of that color will
// automatically be used as the background color.
#define AUTO_SELECT_BACKGROUND_COLOR 0

// If COOL_LIKE_INCANDESCENT is set to 1, colors will 
// fade out slighted 'reddened', similar to how
// incandescent bulbs change color as they get dim down.
#define COOL_LIKE_INCANDESCENT 1

CRGBPalette16 gCurrentPalette;
CRGBPalette16 gTargetPalette;

// ------------- /Twinkle light simulation --------------------


// --------- /config ---------

// State machine
enum{
  STATE_START,
  STATE_CONSTANT_RED,
  STATE_CONSTANT_GREEN,
  STATE_CONSTANT_BLUE,
  STATE_CONSTANT_VIOLETT,
  STATE_CONSTANT_WHITE,
  STATE_PULSE_RED,
  STATE_PULSE_GREEN,
  STATE_PULSE_BLUE,
  STATE_PULSE_VIOLETT,
  STATE_COLOUR_SWITCHING,
  STATE_COLOUR_FADE,
  STATE_RAINBOW,
  STATE_FIRE,
  STATE_CYCLON,
  STATE_PACIFIC,
  STATE_EPTILEPTIKER,
  STATE_TWINKLE,
  STATE_THEATRE,
  STATE_CHASE,
  STATE_FIREWORK,
  STATE_COLOUR_WIPE,
  STATE_OFF
};

// Colours
CRGB red(255, 0, 0);
CRGB green(0, 255, 0);
CRGB blue(0, 0, 255);
CRGB violett(136, 0, 255);
CRGB white(255, 255, 255);
CRGB rgbOff(0, 0, 0);

CRGB fadeColours[] = {red, green, blue, violett};
#define ARRAY_LENGTH_FADE_COLOURS 4

// Used hardware pins
#define PIN_LED_DATA 5
#define PIN_BUTTON_ON 14
#define PIN_BUTTON_OFF 9
#define PIN_BUTTON_NEXT 3
#define PIN_BUTTON_BACK 20
#define PIN_DEBUG_LED 13

// Button ctrl data input
bool buttonOn = false;
bool buttonOff = false;
bool buttonNext = false;
bool buttonBack = false;


CRGBArray<NUM_LEDS> leds;
WS2812FX ws2812fx = WS2812FX(NUM_LEDS, PIN_LED_DATA, NEO_RGB + NEO_KHZ800);

int state = STATE_OFF;            // Initial state
unsigned long stateChangeTime = 0; 

int counter = 0;

unsigned long colourStartTime = 0;
int actualColourIndex = colourPaletteIndex[0];
CRGB colourCode = white;

bool fadeOut = false;
bool fadedOut = false; // Fade in minimum
int fadeBrightness = 0;
CRGB actualFadeColour = fadeColours[0];
bool colourSwitched = false;
int fadeColoursIndex = 0;

bool specialLightShow = false;

bool gReverseDirection = false;

bool buttonPressed = false;

bool fastLedLibInUse = true; // True if fastLed is used, false if ws2812fx is used

unsigned long startTime = 0;

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

int indexOf(int arr[], int value){
  for(int i=0; i < sizeof(arr); i++){
    if(arr[i] == value)
      return i;
  }
  return 0;
}

int indexOf(CRGB arr[], CRGB value){
  for(int i=0; i < sizeof(arr); i++){
    if(arr[i] == value)
      return i;
  }
  return 0;
}

void setup() {
    delay(200);
    FastLED.addLeds<LED_TYPE, PIN_LED_DATA, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(BRIGHTNESS);
    chooseNextColorPalette(gTargetPalette);

    pinMode(PIN_LED_DATA, OUTPUT);
    pinMode(PIN_BUTTON_ON, INPUT_PULLUP);
    pinMode(PIN_BUTTON_OFF, INPUT_PULLUP);
    pinMode(PIN_BUTTON_NEXT, INPUT_PULLUP);
    pinMode(PIN_BUTTON_BACK, INPUT_PULLUP);
    pinMode(PIN_DEBUG_LED, OUTPUT);

    stateChangeTime = millis();

    ws2812fx.init();
    ws2812fx.setBrightness(BRIGHTNESS);
    ws2812fx.setSpeed(1000);
    ws2812fx.setMode(7);
    ws2812fx.start();
}

void updateInputData() {
  buttonOn = !digitalRead(PIN_BUTTON_ON);
  buttonOff = !digitalRead(PIN_BUTTON_OFF);
  buttonNext = !digitalRead(PIN_BUTTON_NEXT);
  buttonBack = !digitalRead(PIN_BUTTON_BACK);
}

/*
// Go to the next state
*/
void nextState() {
  if (state == STATE_OFF)
    switchState(stateArr[0]);
  else
    state++;

  stateChangeTime = millis();
  FastLED.setBrightness(BRIGHTNESS);
  fastLedLibInUse = true;
}

/*
// Go back to the previous state
*/
void backState() {
  if (state == stateArr[1])
    switchState(stateArr[STATE_OFF-1]);
  else
    state--;

  stateChangeTime = millis();
  FastLED.setBrightness(BRIGHTNESS);
  fastLedLibInUse = true;
}

/*
// Go to the given state
// @param next state
*/
void switchState(int newState) {
  if(newState == state)
    return;

  state = newState;
  stateChangeTime = millis();
  FastLED.setBrightness(BRIGHTNESS);
  fastLedLibInUse = true;
}

/*
// Starts the rigth method if one button is pressed
*/
void missionCtrl() {
  // Wait until the start animation ends
  if(state == STATE_START)
    return;

  bool oneButtonPressed = buttonOn || buttonOff || buttonNext || buttonBack;

  if (!buttonPressed && oneButtonPressed) {
    buttonPressed = true;
    
    if (state == STATE_OFF) {
      if(buttonOn)
       switchState(STATE_START);
    } else {
      if(buttonOn)
        specialLightShow = true;
      else if (buttonOff)
        switchState(STATE_OFF);
      else if (buttonNext) {
        if (state != STATE_OFF - 1) nextState();
        else switchState(stateArr[1]);
      }
      else if (buttonBack) 
       backState();
    }

  } else if (!oneButtonPressed)
    buttonPressed = false;
}

// Show constant the same colour
void loopConstantColour(CRGB colour) {
   for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = colour; 
   }
}

// Pulse one colour 
void loopPulseColour(CRGB colour) {
  // FadeOut:
    // True = Brightness decreases
    // False = Brightness rises

  // Set fadeOut
  if(fadeBrightness <= 0){
    fadeOut = false;
    fadedOut = true;
    fadeBrightness = 0;
  } else if (fadeBrightness >= BRIGHTNESS) {
    fadeOut = true;
    fadedOut = false;
    fadeBrightness = BRIGHTNESS;
  }

  // Brightness rises
  if(!fadeOut){
   EVERY_N_MILLISECONDS(TIME_FADE){
    if(fadeBrightness < BRIGHTNESS * 0.95 && fadeBrightness > BRIGHTNESS * 0.05) fadeBrightness += 2;
    else fadeBrightness++;
   }
  } 
  // Brightness decreases
  else {
    EVERY_N_MILLISECONDS(TIME_FADE){
      if(fadeBrightness < BRIGHTNESS * 0.95 && fadeBrightness > BRIGHTNESS * 0.05) fadeBrightness -= 2;
      else fadeBrightness--;
    }
  }
  
  FastLED.setBrightness(fadeBrightness);
  loopConstantColour(colour);
}

// Switching colours
void loopSwitchingColour() { 
  if(millis() - colourStartTime > TIME_PER_COLOUR_SWITCHING)
    colourStartTime = millis();
  else 
    return;

  actualColourIndex++;   
    
  // Set colour in LEDs
  switch(actualColourIndex) {
    case 1: 
      loopConstantColour(red);
      break;
    case 2:
      loopConstantColour(green);
      break;
    case 3:
      loopConstantColour(blue);
      break;
    case 4:
      loopConstantColour(violett);
      break;
    case 5:
      loopConstantColour(white);
      actualColourIndex = 0;
      break;
    default:
      break;      
  }
}  

/*
// Show an eptileptiker colour feature
*/
void eptileptiker(){
  if (fadedOut)
    actualFadeColour = fadeColours[fadeColoursIndex++];

  if(fadeColoursIndex > ARRAY_LENGTH_FADE_COLOURS) 
    fadeColoursIndex = 0;

  loopPulseColour(actualFadeColour);
}

// Fade switching colours
void loopFadeColours() {
 if (fadedOut && !colourSwitched){
  actualFadeColour = fadeColours[fadeColoursIndex++];
  colourSwitched = true;
 } else if (!fadedOut)
  colourSwitched = false;    

  if(fadeColoursIndex > ARRAY_LENGTH_FADE_COLOURS) fadeColoursIndex = 0;
  
  loopPulseColour(actualFadeColour);
}

// Start rainbow light show
void loopRainbow() {
   fill_rainbow(leds, NUM_LEDS, counter);   
   counter += RAINBOW_SAME_COLOUR_LEDS;
}

// Start cyclon light show
void loopCyclon() {
  static int hue = 0;

	// First slide the led in one direction
	for(int i = 0; i < NUM_LEDS; i++) {
		// Set the i'th led to red 
		leds[i] = CHSV(hue++, 255, 255);
		// Show the leds
		FastLED.show(); 
		// now that we've shown the leds, reset the i'th led to black
		// leds[i] = CRGB::Black;
		fadeall();
		// Wait a little bit before we loop around and do it again
		delay(10);
	}
	// Now go in the other direction.  
	for(int i = (NUM_LEDS)-1; i >= 0; i--) {
		// Set the i'th led to red 
		leds[i] = CHSV(hue++, 255, 255);
		// Show the leds
		FastLED.show();
		// now that we've shown the leds, reset the i'th led to black
		// leds[i] = CRGB::Black;
		fadeall();
		// Wait a little bit before we loop around and do it again
		delay(10);
	}
}

/*
// Show a fire led simulation
*/
void fire2012()
{
// Array of temperature readings at each simulation cell
  static int heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}

// --------------------------------------pacifica theme start --------------------------------------------------
// Here are diffrent methods for the pacifica theme which are used below in the pacifica loop main method

// These three custom blue-green color palettes were inspired by the colors found in
// the waters off the southern coast of California, https://goo.gl/maps/QQgd97jjHesHZVxQ7
//
CRGBPalette16 pacifica_palette_1 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
CRGBPalette16 pacifica_palette_2 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
CRGBPalette16 pacifica_palette_3 = 
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };

// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, int bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    int sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
  int basethreshold = beatsin8( 9, 55, 65);
  int wave = beat8( 7 );
  
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    int threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    int l = leds[i].getAverageLight();
    if( l > threshold) {
      int overage = l - threshold;
      int overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145); 
    leds[i].green= scale8( leds[i].green, 200); 
    leds[i] |= CRGB( 2, 5, 7);
  }
}

void pacifica_loop()
{
  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011,10,13));
  sCIStart2 -= (deltams21 * beatsin88(777,8,11));
  sCIStart3 -= (deltams1 * beatsin88(501,5,7));
  sCIStart4 -= (deltams2 * beatsin88(257,4,6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
  pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
  pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}
// --------------------------------------pacifica theme end --------------------------------------------------


// --------------------------------------twinkle theme start --------------------------------------------------
//  This function takes a time in pseudo-milliseconds,
//  figures out brightness = f( time ), and also hue = f( time )
//  The 'low digits' of the millisecond time are used as 
//  input to the brightness wave function.  
//  The 'high digits' are used to select a color, so that the color
//  does not change over the course of the fade-in, fade-out
//  of one cycle of the brightness wave function.
//  The 'high digits' are also used to determine whether this pixel
//  should light at all during this cycle, based on the TWINKLE_DENSITY.
CRGB computeOneTwinkle( uint32_t ms, uint8_t salt)
{
  uint16_t ticks = ms >> (8-TWINKLE_SPEED);
  uint8_t fastcycle8 = ticks;
  uint16_t slowcycle16 = (ticks >> 8) + salt;
  slowcycle16 += sin8( slowcycle16);
  slowcycle16 =  (slowcycle16 * 2053) + 1384;
  uint8_t slowcycle8 = (slowcycle16 & 0xFF) + (slowcycle16 >> 8);
  
  uint8_t bright = 0;
  if( ((slowcycle8 & 0x0E)/2) < TWINKLE_DENSITY) {
    bright = attackDecayWave8( fastcycle8);
  }

  uint8_t hue = slowcycle8 - salt;
  CRGB c;
  if( bright > 0) {
    c = ColorFromPalette( gCurrentPalette, hue, bright, NOBLEND);
    if( COOL_LIKE_INCANDESCENT == 1 ) {
      coolLikeIncandescent( c, fastcycle8);
    }
  } else {
    c = CRGB::Black;
  }
  return c;
}


// This function is like 'triwave8', which produces a 
// symmetrical up-and-down triangle sawtooth waveform, except that this
// function produces a triangle wave with a faster attack and a slower decay:
//
//     / \ 
//    /     \ 
//   /         \ 
//  /             \ 
//

uint8_t attackDecayWave8( uint8_t i)
{
  if( i < 86) {
    return i * 3;
  } else {
    i -= 86;
    return 255 - (i + (i/2));
  }
}

// This function takes a pixel, and if its in the 'fading down'
// part of the cycle, it adjusts the color a little bit like the 
// way that incandescent bulbs fade toward 'red' as they dim.
void coolLikeIncandescent( CRGB& c, uint8_t phase)
{
  if( phase < 128) return;

  uint8_t cooling = (phase - 128) >> 4;
  c.g = qsub8( c.g, cooling);
  c.b = qsub8( c.b, cooling * 2);
}

// A mostly red palette with green accents and white trim.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 RedGreenWhite_p FL_PROGMEM =
{  CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, 
   CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, 
   CRGB::Red, CRGB::Red, CRGB::Gray, CRGB::Gray, 
   CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green };

// A mostly (dark) green palette with red berries.
#define Holly_Green 0x00580c
#define Holly_Red   0xB00402
const TProgmemRGBPalette16 Holly_p FL_PROGMEM =
{  Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
   Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
   Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
   Holly_Green, Holly_Green, Holly_Green, Holly_Red 
};

// A red and white striped palette
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 RedWhite_p FL_PROGMEM =
{  CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red, 
   CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray,
   CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red, 
   CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray };

// A mostly blue palette with white accents.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 BlueWhite_p FL_PROGMEM =
{  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
   CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
   CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
   CRGB::Blue, CRGB::Gray, CRGB::Gray, CRGB::Gray };

// A pure "fairy light" palette with some brightness variations
#define HALFFAIRY ((CRGB::FairyLight & 0xFEFEFE) / 2)
#define QUARTERFAIRY ((CRGB::FairyLight & 0xFCFCFC) / 4)
const TProgmemRGBPalette16 FairyLight_p FL_PROGMEM =
{  CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, 
   HALFFAIRY,        HALFFAIRY,        CRGB::FairyLight, CRGB::FairyLight, 
   QUARTERFAIRY,     QUARTERFAIRY,     CRGB::FairyLight, CRGB::FairyLight, 
   CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight };

// A palette of soft snowflakes with the occasional bright one
const TProgmemRGBPalette16 Snow_p FL_PROGMEM =
{  0x304048, 0x304048, 0x304048, 0x304048,
   0x304048, 0x304048, 0x304048, 0x304048,
   0x304048, 0x304048, 0x304048, 0x304048,
   0x304048, 0x304048, 0x304048, 0xE0F0FF };

// A palette reminiscent of large 'old-school' C9-size tree lights
// in the five classic colors: red, orange, green, blue, and white.
#define C9_Red    0xB80400
#define C9_Orange 0x902C02
#define C9_Green  0x046002
#define C9_Blue   0x070758
#define C9_White  0x606820
const TProgmemRGBPalette16 RetroC9_p FL_PROGMEM =
{  C9_Red,    C9_Orange, C9_Red,    C9_Orange,
   C9_Orange, C9_Red,    C9_Orange, C9_Red,
   C9_Green,  C9_Green,  C9_Green,  C9_Green,
   C9_Blue,   C9_Blue,   C9_Blue,
   C9_White
};

// A cold, icy pale blue palette
#define Ice_Blue1 0x0C1040
#define Ice_Blue2 0x182080
#define Ice_Blue3 0x5080C0
const TProgmemRGBPalette16 Ice_p FL_PROGMEM =
{
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue2, Ice_Blue2, Ice_Blue2, Ice_Blue3
};


// Add or remove palette names from this list to control which color
// palettes are used, and in what order.
const TProgmemRGBPalette16* ActivePaletteList[] = {
  &RetroC9_p,
  &BlueWhite_p,
  &RainbowColors_p,
  &FairyLight_p,
  &RedGreenWhite_p,
  &PartyColors_p,
  &RedWhite_p,
  &Snow_p,
  &Holly_p,
  &Ice_p  
};


// Advance to the next color palette in the list (above).
void chooseNextColorPalette( CRGBPalette16& pal)
{
  const uint8_t numberOfPalettes = sizeof(ActivePaletteList) / sizeof(ActivePaletteList[0]);
  static uint8_t whichPalette = -1; 
  whichPalette = addmod8( whichPalette, 1, numberOfPalettes);

  pal = *(ActivePaletteList[whichPalette]);
}

//  This function loops over each pixel, calculates the 
//  adjusted 'clock' that this pixel should use, and calls 
//  "CalculateOneTwinkle" on each pixel.  It then displays
//  either the twinkle color of the background color, 
//  whichever is brighter.
void drawTwinkles( CRGBSet& L)
{
  // "PRNG16" is the pseudorandom number generator
  // It MUST be reset to the same starting value each time
  // this function is called, so that the sequence of 'random'
  // numbers that it generates is (paradoxically) stable.
  uint16_t PRNG16 = 11337;
  
  uint32_t clock32 = millis();

  // Set up the background color, "bg".
  // if AUTO_SELECT_BACKGROUND_COLOR == 1, and the first two colors of
  // the current palette are identical, then a deeply faded version of
  // that color is used for the background color
  CRGB bg;
  if( (AUTO_SELECT_BACKGROUND_COLOR == 1) &&
      (gCurrentPalette[0] == gCurrentPalette[1] )) {
    bg = gCurrentPalette[0];
    uint8_t bglight = bg.getAverageLight();
    if( bglight > 64) {
      bg.nscale8_video( 16); // very bright, so scale to 1/16th
    } else if( bglight > 16) {
      bg.nscale8_video( 64); // not that bright, so scale to 1/4th
    } else {
      bg.nscale8_video( 86); // dim, scale to 1/3rd.
    }
  } else {
    bg = gBackgroundColor; // just use the explicitly defined background color
  }

  uint8_t backgroundBrightness = bg.getAverageLight();
  
  for( CRGB& pixel: L) {
    PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
    uint16_t myclockoffset16= PRNG16; // use that number as clock offset
    PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
    // use that number as clock speed adjustment factor (in 8ths, from 8/8ths to 23/8ths)
    uint8_t myspeedmultiplierQ5_3 =  ((((PRNG16 & 0xFF)>>4) + (PRNG16 & 0x0F)) & 0x0F) + 0x08;
    uint32_t myclock30 = (uint32_t)((clock32 * myspeedmultiplierQ5_3) >> 3) + myclockoffset16;
    uint8_t  myunique8 = PRNG16 >> 8; // get 'salt' value for this pixel

    // We now have the adjusted 'clock' for this pixel, now we call
    // the function that computes what color the pixel should be based
    // on the "brightness = f( time )" idea.
    CRGB c = computeOneTwinkle( myclock30, myunique8);

    uint8_t cbright = c.getAverageLight();
    int16_t deltabright = cbright - backgroundBrightness;
    if( deltabright >= 32 || (!bg)) {
      // If the new pixel is significantly brighter than the background color, 
      // use the new color.
      pixel = c;
    } else if( deltabright > 0 ) {
      // If the new pixel is just slightly brighter than the background color,
      // mix a blend of the new color and the background color
      pixel = blend( bg, c, deltabright * 8);
    } else { 
      // if the new pixel is not at all brighter than the background color,
      // just use the background color.
      pixel = bg;
    }
  }
}

void twinkle_loop() {
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) { 
    chooseNextColorPalette( gTargetPalette ); 
  }
  
  EVERY_N_MILLISECONDS( 10 ) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 12);
  }

  drawTwinkles(leds);
}
// -------------------------------------- twinkle theme end --------------------------------------------------

// -------------------------------------- ws2812fx libary methods start --------------------------------------------------
void startingTheme() {
  if(fastLedLibInUse){
    ws2812fx.setColor(0xff000);  // red
    ws2812fx.setMode(FX_MODE_DUAL_SCAN);       // 2 running points
    fastLedLibInUse = false;
  }
}

void theatreTheme() {
   if(fastLedLibInUse){
    ws2812fx.setMode(FX_MODE_THEATER_CHASE_RAINBOW);       // theatre with rainbow colours
    fastLedLibInUse = false;
  }
}

void chaseTheme() {
  if(fastLedLibInUse){
    ws2812fx.setMode(FX_MODE_CHASE_RAINBOW);       // chase with rainbow colours
    fastLedLibInUse = false;
  }
}

void fireworkTheme() {
  if(fastLedLibInUse){
    ws2812fx.setMode(FX_MODE_FIREWORKS_RANDOM);       // firework with random colours 
    fastLedLibInUse = false;
  }
}

void wipeTheme() {
  if(fastLedLibInUse){
    ws2812fx.setMode(FX_MODE_COLOR_WIPE_RANDOM);       // wipe with random colours 
    fastLedLibInUse = false;
  }
}

void specialLightShowTheme() {
  if(millis() < TIME_STARTING_ANIMATION)
    return;

  if(fastLedLibInUse){
    startTime = millis();
    ws2812fx.setMode(FX_MODE_HALLOWEEN);
    fastLedLibInUse = false;
  }
  // Stop light theme after the setted time period
  if(millis() - startTime > TIME_SPECIAL_LIGHT_SHOW){
    specialLightShow = false;
    fastLedLibInUse = true;
  }
}
// -------------------------------------- ws2812fx libary methods end --------------------------------------------------

/*
// Main loop with state machine
*/
void loop() {
  // Wait for init
  if (millis() < 500)
    return;

  // Toogle debug LED
  digitalWrite(PIN_DEBUG_LED, millis() % 500 > 250);

  // Check for special light show  
  if(specialLightShow)
    specialLightShowTheme();
  else {

    // Update input data and button action
    updateInputData();
    missionCtrl();

    int timeInState = millis() - stateChangeTime;

    // State machine
    switch(state) {
      case STATE_START: // Start simulation
        startingTheme();

        if(timeInState > TIME_STARTING_ANIMATION)
          nextState();
        break;
      case STATE_CONSTANT_GREEN:
        loopConstantColour(green);
        break;
      case STATE_CONSTANT_RED:
        loopConstantColour(red);
        break;
      case STATE_CONSTANT_BLUE:
        loopConstantColour(blue);
        break;
      case STATE_CONSTANT_VIOLETT:
        loopConstantColour(violett);
        break;
      case STATE_CONSTANT_WHITE:
        loopConstantColour(white);
        break;
      case STATE_PULSE_RED:
        loopPulseColour(red);
        break;
      case STATE_PULSE_GREEN:
        loopPulseColour(green);
        break;
      case STATE_PULSE_BLUE:
        loopPulseColour(blue);
        break;
      case STATE_PULSE_VIOLETT:
        loopPulseColour(violett);
        break;  
      case STATE_COLOUR_SWITCHING:
        loopSwitchingColour();
        break;
      case STATE_COLOUR_FADE:
        loopFadeColours();
        break;
      case STATE_RAINBOW:
        loopRainbow();
        break;
      case STATE_FIRE:
        fire2012();
        break;   
      case STATE_CYCLON:
        loopCyclon();
        break;
      case STATE_PACIFIC:
        pacifica_loop();
        break;    
      case STATE_EPTILEPTIKER:
        eptileptiker();
        break;
      case STATE_TWINKLE:
        twinkle_loop();
        break;
      case STATE_THEATRE:
        theatreTheme();
        break;
      case STATE_CHASE:
        chaseTheme();
        break;
      case STATE_FIREWORK:
        fireworkTheme();
        break;
      case STATE_OFF:
        loopConstantColour(rgbOff);
        break;
      case STATE_COLOUR_WIPE:
        wipeTheme();
        break;
      default:
        // Do nothing
        break;
    }
  }

  if (!fastLedLibInUse)
    ws2812fx.service();
  else {
   FastLED.show();
   FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
}