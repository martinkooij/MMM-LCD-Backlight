
#include <string.h>
#include <stdio.h>
#include "pico/stdio.h"
#include <tusb.h>
#include "rstp.h"

#define BUFLENGTH 2400

typedef  int (*statefunc)() ;

static message_handlerfp _rstp_handler  = NULL;
static kickerfp _rstp_kicker = NULL;

static uint8_t _rstp_buf[BUFLENGTH+1];

void rstp_set_message_handler(message_handlerfp handler) {
	_rstp_handler = handler;
};

void rstp_set_kicker(kickerfp k) {
	_rstp_kicker = k;
};

static int get_line(char firstchar, char * buf, int n) {
//     printf("PICO DEBUG: In getline now with firstchar = %d\n",firstchar);
	 uint count = 0 ;
     int c = firstchar ; 
	 while ( (c != '\r') && count < n) {
		buf[count++] = (uint8_t)c ;
		c = getchar_timeout_us((uint32_t)(1000000)*6) ;
//		printf("received a :%d:",buf[count -1]);
//		c = (int)getchar() ;
		if (c == PICO_ERROR_TIMEOUT) break ;
	 };
	 buf[count] = '\0' ;
//	 printf("PICO DEBUG: Out of getline now\n");
	 return (count == n ? -1 : count) ;
};


static int state0() {
	int c ;
	if (_rstp_kicker != NULL) _rstp_kicker(0) ;
	printf(">\n");
	while (c = getchar_timeout_us((uint32_t)(1000000)*6)) {
		if (c != 10) break; 
	};
	if (PICO_ERROR_TIMEOUT == c) return 0;
	int result = get_line((char)c,_rstp_buf,BUFLENGTH);
	return 1;
};

static int state1() {
	if (_rstp_handler == NULL) {
		printf("PICO OUTS>>>>");
		printf(_rstp_buf);
		printf("<<<<<\n");
	} else {
		_rstp_handler(_rstp_buf) ;
	};
	return 0;
};

void my_handler (uint8_t * buf) {
		printf("PICO SPECIAL>>>>");
		printf(_rstp_buf);
		printf("<<<<<\n");
};

int rstp_statemachine(int state) {
	statefunc states[2];
	states[0] = state0;
	states[1] = state1;

	while (state >=0 && state <99) {
//		printf("PICO DEBUG: starting statemachine %d\n", state);
		state = states[state]();
	};
	return state ;
};

	

	

	
	