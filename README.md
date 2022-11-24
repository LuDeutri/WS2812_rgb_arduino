# WS2812B_RGB_Animation
Arduino WS2812B RGB Animation Project



This is a simple Arduino project which includes various RGB animations for WS2812B LEDs strings inside a state machine. The project is based on two Arduino Libarys: 

1. FastLED (https://github.com/FastLED/FastLED)
2. WS2812FX (https://github.com/kitesurfer1404/WS2812FX)  

An Arduino strip with 1m length and 60 LEDs is used, but this can easily be scaled up or down in the code. With 4 mounted buttons the LED show is switched on / off, as well as switching between the different RGB animations.

BUTTON_ON: Switch on (if off), Special Animation for a few seconds (if already on)

BUTTON_OFF: Switch off

BUTTON_NEXT: Next animation

BUTTON_BACK: Previous animation 


The states are arranged in an array, which allows to arrange them differently or to disable different animations. At start a short start animation is executed.
