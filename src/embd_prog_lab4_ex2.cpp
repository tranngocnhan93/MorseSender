/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include "chip.h"
#include "board.h"
#include <cr_section_macros.h>
#include <atomic>
#include "digital_IO.h"
#include <cctype>
#include "itm_class.h"
#include <string>
#include <cstring>
#include <stdlib.h>


#define TICKRATE_HZ         (1000)
#define TIMEUNIT			(10)			//ms
static volatile int state;
static volatile std::atomic_int counter;

extern "C"{
void SysTick_Handler(void)
{
	static int ticks = 0;
	if(counter > 0) counter--;
	ticks++;
	if (ticks > TICKRATE_HZ) {
		ticks = 0;
		state = 1 - state;
	}
}
}
void Sleep(int ms)
{
	counter = ms;
	while(counter > 0) {
		__WFI();
	}
}

const int DOT = 1;
const int DASH = 3;
const char cmd_wpm[] = "wpm ";
const char cmd_send[] = "send ";
const char cmd_set[] = "set ";
struct MorseCode {
	char symbol;
	unsigned char code[7];
};
const MorseCode ITU_morse[] = {
		{ 'A', { DOT, DASH } }, // A
		{ 'B', { DASH, DOT, DOT, DOT } }, // B
		{ 'C', { DASH, DOT, DASH, DOT } }, // C
		{ 'D', { DASH, DOT, DOT } }, // D
		{ 'E', { DOT } }, // E
		{ 'F', { DOT, DOT, DASH, DOT } }, // F
		{ 'G', { DASH, DASH, DOT } }, // G
		{ 'H', { DOT, DOT, DOT, DOT } }, // H
		{ 'I', { DOT, DOT } }, // I
		{ 'J', { DOT, DASH, DASH, DASH } }, // J
		{ 'K', { DASH, DOT, DASH } }, // K
		{ 'L', { DOT, DASH, DOT, DOT } }, // L
		{ 'M', { DASH, DASH } }, // M
		{ 'N', { DASH, DOT } }, // N
		{ 'O', { DASH, DASH, DASH } }, // O
		{ 'P', { DOT, DASH, DASH, DOT } }, // P
		{ 'Q', { DASH, DASH, DOT, DASH } }, // Q
		{ 'R', { DOT, DASH, DOT } }, // R
		{ 'S', { DOT, DOT, DOT } }, // S
		{ 'T', { DASH } }, // T
		{ 'U', { DOT, DOT, DASH } }, // U
		{ 'V', { DOT, DOT, DOT, DASH } }, // V
		{ 'W', { DOT, DASH, DASH } }, // W
		{ 'X', { DASH, DOT, DOT, DASH } }, // X
		{ 'Y', { DASH, DOT, DASH, DASH } },  // Y
		{ 'Z', { DASH, DASH, DOT, DOT } }, // Z
		{ '0', { DASH, DASH, DASH, DASH, DASH } }, // 0
		{ '1', { DOT, DASH, DASH, DASH, DASH } }, // 1
		{ '2', { DOT, DOT, DASH, DASH, DASH } }, // 2
		{ '3', { DOT, DOT, DOT, DASH, DASH } }, // 3
		{ '4', { DOT, DOT, DOT, DOT, DASH } }, // 4
		{ '5', { DOT, DOT, DOT, DOT, DOT } }, // 5
		{ '6', { DASH, DOT, DOT, DOT, DOT } }, // 6
		{ '7', { DASH, DASH, DOT, DOT, DOT } }, // 7
		{ '8', { DASH, DASH, DASH, DOT, DOT } }, // 8
		{ '9', { DASH, DASH, DASH, DASH, DOT } }, // 9
		{ 0, { 0 } } // terminating entry - Do not remove!
};

class MorseSender {
public:
	MorseSender(DigitalIO *ld, DigitalIO *dcr) {
		led = ld;
		decoder = dcr;
		dottime = 10;	//ms
	}
	void Send(char message[]) {
//		SWOITMclass itm;
		for(int i = 0; message[i] != '\0'; i++) {			//scan every letter
			if((message[i] >= 'a' && message[i] <= 'z') || (message[i] >= 'A' && message[i] <= 'Z')) {
				message[i] = toupper(message[i]);
//				itm.print(ITU_morse[message[i] - 'A'].symbol);
//				itm.print("\n");
				for(int j = 0; ITU_morse[message[i] - 'A'].code[j] != 0; j++) {		//scan every code in letter
					for(int k = 0; k < ITU_morse[message[i] - 'A'].code[j]; k++) {
						led->Write(true);
						decoder->Write(true);
						Sleep(TIMEUNIT);
					}
					led->Write(false);						//inter-element gap between codes
					decoder->Write(false);
					Sleep(TIMEUNIT);
				}
			}
			else if(message[i] >= '0' && message[i] <= '9') {
//				itm.print(ITU_morse[message[i] - '0' + 26].symbol);
//				itm.print("\n");
				for(int j = 0; ITU_morse[message[i] - '0' + 26].code[j] != 0; j++) {
					for(int k = 0; k < ITU_morse[message[i] - '0' + 26].code[j]; k++) {
						led->Write(true);
						decoder->Write(true);
						Sleep(TIMEUNIT);
					}
					led->Write(false);
					decoder->Write(false);
					Sleep(TIMEUNIT);
				}
			}
			else if(message[i] != ' ') {
//				itm.print(ITU_morse['X' - 'A'].symbol);
//				itm.print("\n");
				for(int j = 0; ITU_morse['X' - 'A'].code[j] != 0; j++) {		//send X
					for(int k = 0; k < ITU_morse['X' - 'A'].code[j]; k++) {
						led->Write(true);
						decoder->Write(true);
						Sleep(TIMEUNIT);
					}
					led->Write(false);						//inter-element gap between codes
					decoder->Write(false);
					Sleep(TIMEUNIT);
				}
			}
			if(message[i] != ' ') {
				if(message[i+1] == ' ')
					Sleep(7*TIMEUNIT);
				else if(message[i+1] != '\0') Sleep(3*TIMEUNIT);
			}
		}
	}
	void Send(std::string message) {
		char cmess[81];
		std::strncpy(cmess, message.c_str(), 81);
		this->Send(cmess);
	}
	int ScanCommand(char message[]) {
		SWOITMclass itm;
		char *p, *q;
		int temp = 0;
		char c[2] = {'a', '\0'};
		p = strstr(message, cmd_wpm);
		if(p != NULL) {
			temp = atoi(p+4);			//4 characters in "wpm "
			if(temp != 0)
				this->dottime = temp;
		}
		p = strstr(message, cmd_send);
		if(p != NULL) {
			p += 5;							//5 characters in "send "
			q = strstr(p, cmd_wpm);		//find terminating point of string to be sent
			if(q == NULL) {
				q = strstr(p, cmd_send);
				if(q == NULL) {
					q = strstr(p, cmd_set);
					if(q == NULL)
						q = strchr(p, '\0');
				}
			}
			for(p; p < q; p++) {
				c[0] = *p;
				this->Send(c);
				itm.print(c);
				itm.print("\n");
			}
		}
		p = strstr(message, cmd_set);
		if(p != NULL)
			return this->dottime;
		else return -1;
	}
	~MorseSender() {
		delete led;
		delete decoder;
	}
private:
	DigitalIO *led;
	DigitalIO *decoder;
	unsigned int dottime;
};


int main(void) {
	uint32_t sysTickRate;
	SystemCoreClockUpdate();
	Chip_Clock_SetSysTickClockDiv(1);
	sysTickRate = Chip_Clock_GetSysTickClockRate();
	SysTick_Config(sysTickRate / TICKRATE_HZ);
	Board_Init();

	DigitalIO led(1, 1, false, false, true);
	DigitalIO decoder(0, 8, false, false, false);
	MorseSender sender(&led, &decoder);
	char message[81] = "test string wpm 150 send 123con cu set con cu";
//	int c;
//	int i = 0;
	int dottime = 0, wpm = 0, temp = 0;
	temp = atoi(message);
	dottime = sender.ScanCommand(message);
	if(dottime > 0) {
		wpm = 1000/dottime;
		Board_UARTPutSTR("wpm: ");


		Board_UARTPutSTR("dot time %d\n", dottime);
	}
    while(1) {

    	Sleep(5000);
    }
    return 0 ;
}

/*
 *     	c = Board_UARTGetChar();
    	while(i <= 80 && c != '\r') {
    		if(c != EOF) {
    			Board_UARTPutChar(c);
    			message[i] = c;
    			i++;
    		}
    		c = Board_UARTGetChar();
    	}
    	if(c == '\r')	Board_UARTPutChar('\n'); // send line feed after carriage return
    	message[i] = '\0';
    	Board_UARTPutSTR(message);
    	sender.Send(message);
    	i = 0;
 *
 */
