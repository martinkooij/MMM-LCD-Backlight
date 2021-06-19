/***********************************************************
 * backlighting.cpp                                          *
 * MIT licence                                             *
 * 2021 - Martin Kooij                                     *
 * https://github.com/martinkooij/MMM-LCD-Backlight        *
 ***********************************************************/


/*--------------------------------------  DEFINITIONS --------------------------------------*/
#include <stdlib.h>
#include <tusb.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "stdio.h"

#include "Adafruit_NeoPixel.hpp"
#include "jsmn.h"
#include "jsmn_traversal.hpp"
#include "rstp.h"

#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "hardware/adc.h"


#define TOP_OFF_POINT 24
#define NO_STRANDS 2
#define STEP_TIME 100
#define NO_PIXELS 30
#define STRAND_PIN_START 5
#define LDR_CHECK_TIME 15000

extern "C" int rstp_statemachine(int state);
extern "C" void rstp_set_message_handler(message_handlerfp handler);
extern "C" void rstp_set_kicker(kickerfp k);

absolute_time_t kick_timer0 = get_absolute_time() ;
absolute_time_t kick_timer1 = get_absolute_time() ;

struct Strandinfo {
   Adafruit_NeoPixel * ps;
   uint32_t * color_vals ;
   int time ;
   };   // info written by the producer
   
   
/* ------------------------- SHARED MEMORY------------------------------------------------------------ */
static uint8_t brightness = 124;
static uint8_t set_brightness = 255 ;
static uint lux_levels[] = {70,450,700} ; // if the first element = 0 the leds will be off

Strandinfo strings[NO_STRANDS] ;

auto_init_mutex(my_protect);

/* ------------------------- HELP FUNCTIONS -----------------------------------------------------------*/

uint8_t adjust (uint8_t value) {
	if (brightness == 0) return 0 ;
	if (brightness == 255) return value ;
	return (value * neopixels_gamma8(brightness)) >> 8 ;
}

struct HSV {
	uint16_t h;
	uint8_t s ;
	uint8_t v;
};

HSV rgb_to_hsv(uint32_t color)
{
	uint8_t r = (uint8_t)(color >> 16);
    uint8_t g = (uint8_t)(color >>  8);
    uint8_t b = (uint8_t)color;
    HSV hsv ;
    uint8_t rgbMin, rgbMax ;

    rgbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
    rgbMax = r > g ? (r > b ? r : b) : (g > b ? g : b);

    hsv.v = rgbMax;
    if (hsv.v == 0)
    {
        hsv.h = 0;
        hsv.s = 0;
        return hsv;
    }

    hsv.s = 255 * (uint32_t)(rgbMax - rgbMin) / hsv.v;
    if (hsv.s == 0)
    {
        hsv.h = 0;
        return hsv;
    }

    if (rgbMax == r)
        hsv.h = 0 + 10922 * (long)(g - b) / (rgbMax - rgbMin);
    else if (rgbMax == g)
        hsv.h = 21845 + 10922 * (long)(b - r) / (rgbMax - rgbMin);
    else
        hsv.h = 43690 + 10922 * (long)(r - g) / (rgbMax - rgbMin);

    return hsv;
}


void kick_timer(uint core) {
	if (core == 0) {
		kick_timer0 = get_absolute_time() ;
	} else {
		kick_timer1 = get_absolute_time();
	};
	uint64_t diff0 = absolute_time_diff_us (kick_timer0, get_absolute_time());
	uint64_t diff1 = absolute_time_diff_us (kick_timer1, get_absolute_time());
	if ( diff0 < 13000000 && diff1 < 13000000 ) {
		watchdog_update();  // only update the watchdog when *both* cores kick the timers
	};
};

/* ---------------------------   RUNNING ON CORE 1 (HANDLING THE LED STRANDS) ------------------------*/

void set_new_brightness(uint alarm) {
	if (alarm != 0 ) return;
	if (lux_levels[0] == 0) {
		set_brightness = 0 ;
		hardware_alarm_set_target (0,delayed_by_ms(get_absolute_time(),LDR_CHECK_TIME));
		return ;
	}
		
	const float conversion_factor = 3.3f / (1 << 12);
	uint16_t result = adc_read();
	float volt = result * conversion_factor ;
	float lux=500/(10*((3.35-volt)/volt));//use this equation if the LDR is in the upper part of the divider
	printf("Raw value: %d, voltage: %f V, lux = %d\n", result, result * conversion_factor, (int)lux);
	if (lux <= lux_levels[0]) {
		set_brightness = 64 ;
	} else if (lux <= lux_levels[1]) {
		set_brightness = 100 ;
	} else if (lux <= lux_levels[2]) {
		set_brightness = 154 ;
	} else {
		set_brightness = 192 ;
	};
	hardware_alarm_set_target (0,delayed_by_ms(get_absolute_time(),LDR_CHECK_TIME));
} ;
			
	
	

void printf_hsv(HSV hsv) {
	printf("<%d,%d,%d = %X>",hsv.h, hsv.s, hsv.v,strings[0].ps->ColorHSV(hsv.h, hsv.s, hsv.v)) ;
};
	

void continous_adapt_strings() {
	
  printf("Core 1 started\n");
  
  mutex_enter_blocking(&my_protect);
  for (int s = 0 ; s < NO_STRANDS ; s++ ) {
	  strings[s].ps->begin() ;
	  strings[s].ps->clear() ;
	  strings[s].ps->setBrightnessFunctions(adjust,adjust,adjust,adjust);
	  strings[s].ps->show() ;
  };
  mutex_exit(&my_protect);
  
  while (1) {
	bool something_changed = false ;
	
//	printf("Enter a mutex_cyle\n") ;
	mutex_enter_blocking(&my_protect);
	if (brightness != set_brightness) {
		printf ("brightness = %d, set_brightness = %d\n", brightness, set_brightness) ;
		brightness = set_brightness ;
		
		for (int s = 0 ; s < NO_STRANDS ; s++ ) {
			strings[s].ps->setBrightnessFunctions(adjust,adjust,adjust,adjust); // to force pixel recalculations
		};
		something_changed = true ;
	};
	for (int s = 0 ; s < NO_STRANDS ; s++) {
		if (strings[s].time == -1) { 
//			printf("Immediate skip strand if ready \n"); 
			continue ; 
		} // strand already upto date
		uint steps_to_go = strings[s].time / STEP_TIME ;
//		printf("DEBUG:PICO: steps_to_go on string %d= %d. Time = %d\n", s, steps_to_go, strings[s].time);
		for (int i = 0 ; i < strings[s].ps->numPixels() ; i++) {
			uint32_t pixel ;
			pixel = strings[s].ps->getPixelColor(i) ;
			if (pixel != strings[s].color_vals[i]) {
				if (steps_to_go <= 1) {
					strings[s].ps->setPixelColor(i,strings[s].color_vals[i]) ;
					something_changed = true ;
				} else {
					HSV hsv_old = rgb_to_hsv(pixel) ;
					HSV hsv_new = rgb_to_hsv(strings[s].color_vals[i]) ;
					HSV hsv_set;
					uint16_t h = (hsv_new.h - hsv_old.h)+32768 ;
					int diff = (int)h - (int)32768 ;
					int diff2 ;
					uint8_t s_to = (uint32_t)(hsv_new.s <= TOP_OFF_POINT ? 0 : (hsv_new.s - TOP_OFF_POINT))*256/(256-TOP_OFF_POINT) ;
					uint8_t s_from = (uint32_t)(hsv_old.s <= TOP_OFF_POINT ? 0 : (hsv_old.s - TOP_OFF_POINT))*256/(256-TOP_OFF_POINT) ;
					if (s_to < (int)s_from * (int)steps_to_go) {
						diff2 = (diff * (int)s_to) / ((int)steps_to_go * (int)s_from) ; // hue-step = max (diff , (diff/n)*(sat_new/sat_old)
					} else {
						diff2 = diff;
					};
					hsv_set.h = hsv_old.h + diff2 ;
					hsv_set.s = hsv_old.s + ((int)hsv_new.s - (int)hsv_old.s) / (int)steps_to_go ;
					hsv_set.v = hsv_old.v + ((int)hsv_new.v - (int)hsv_old.v) / (int)steps_to_go ;
					strings[s].ps->setPixelColor(i,strings[s].ps->ColorHSV(hsv_set.h, hsv_set.s, hsv_set.v));
					something_changed = true ;
				};
			};
		};
		strings[s].time = steps_to_go <= 1 ?  -1 : strings[s].time - STEP_TIME ;
	};
	mutex_exit(&my_protect);
	
//	printf("exit a mutex cyle with something changed = %d\n",something_changed);
	// Has something changed in the strand setting in the mutex protected code? 
	if (something_changed) {
		for (int s = 0 ; s < NO_STRANDS ; s++) {
//			printf("Force a show\n");
			strings[s].ps->show();
		};
	};
	
	sleep_ms(STEP_TIME);
	kick_timer(1) ;
	
  };
};

/* ------------------------------------   RUNNING ON CORE 0 (HANDLING THE COMMUNICATION) ---------------------*/

void json_handler (uint8_t * buf) { 
	printf("HANDLING JSON:");
	printf((const char *)buf) ;
	printf("\n");
	
	int i;
	int r;
	int extra;
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
  
	jsmn_init(&p);
	r = jsmn_parse(&p, (char *) buf, strlen( (char *) buf), t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return ;
	}

	JSMNobject myJSON((char *) buf, &t[0]) ;
  
  /* Assume the top-level element is an object */
	if (r < 1 || myJSON.isOf() != JSMN_OBJECT) {
		printf("Object expected\n");
		return ;
	} ;
	
	int command = myJSON.select("command").asInt() ;
	
	switch (command) {
	case 0: 
	{int strand_no = myJSON.select("strand").asInt();
	JSMNobject pixelArray = myJSON.select("pixels");
	int n  = pixelArray.sizeIter() ;
	
	mutex_enter_blocking(&my_protect);
	for (int p = 0 ; p < n ; p++) {
		uint32_t pixel = pixelArray.iterElem().asInt() ;
//		printf("Set pixel %d as %X\n",p,pixel);
		strings[strand_no - 1 ].color_vals[p] = pixel ;
		pixelArray.iterate() ;
	};
	int timing = myJSON.select("timer").asInt() ;
	printf("received strand %d, with last pixel %X, with timing %d ms\n", strand_no, strings[strand_no - 1].color_vals[0], timing);
	strings[strand_no - 1].time = timing ;
	mutex_exit(&my_protect);
	};
	break ;
	
	case 1:
	lux_levels[0] = (uint)myJSON.select("levels").element(0).asInt() ;
	lux_levels[1] = (uint)myJSON.select("levels").element(1).asInt() ;
	lux_levels[2] = (uint)myJSON.select("levels").element(2).asInt() ;
	printf("Received Lux-levels [%d,%d,%d]\n", lux_levels[0], lux_levels[1], lux_levels[2]);
	break;
	
	default:
	;
	};
};


/* ------------     MAIN FUNCTION (STARTING COMMUNICATION & LED STRINGS HANDLING) --------------*/

int main() {
    stdio_init_all();
	sleep_ms(10000) ;
	printf("Started \n");
	
	Adafruit_NeoPixel mystrand1 = Adafruit_NeoPixel(NO_PIXELS,STRAND_PIN_START,NEO_GRB + NEO_KHZ800);
	Adafruit_NeoPixel mystrand2 = Adafruit_NeoPixel(NO_PIXELS,STRAND_PIN_START + 1,NEO_GRB + NEO_KHZ800);
	
	hardware_alarm_set_callback (0,set_new_brightness);
	hardware_alarm_set_target   (0,delayed_by_ms(get_absolute_time(),LDR_CHECK_TIME));
	
	adc_init();
	adc_gpio_init(26); // Select ADC input 0 (GPIO26)
	adc_select_input(0);

	uint32_t colors1[NO_PIXELS] ;
	uint32_t colors2[NO_PIXELS] ;
	mystrand1.clear() ;
	mystrand2.clear() ;
  
    for (int i = 0 ; i < 30 ; i++) {
		colors1[i] = mystrand1.Color(0, 0, 0);
		colors2[i] = colors1[i] ;
	};
	
  	strings[0].ps = &mystrand1 ;
	strings[0].color_vals = colors1 ;
	strings[0].time = -1 ;
	
	strings[1].ps = &mystrand2 ;
	strings[1].color_vals = colors2 ;
	strings[1].time = -1 ;
	
/* ----------------------- START CORE 1 LOOP --------------------------*/
	multicore_launch_core1(continous_adapt_strings);
  
/* ----------------------- START CORE 0 LOOP --------------------------*/

    printf("Starting statemachine\n") ;
	watchdog_enable(0x7fffffff, 1);
	rstp_set_kicker(kick_timer);
    rstp_set_message_handler(json_handler) ;
	int result = rstp_statemachine(0);
 
 /* ----------------------- INFINITY LOOPS ENDING ---------------------*/
	
  printf("I should never be here! ENDING\n");
  return 0 ;
};
		
		
		
			
			