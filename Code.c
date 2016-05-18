# CPE329FinalProject

#include <msp430.h> 
#include <clockFrequencySet.h>

void motorDrive(int);
int buttonHit(void);
int lightCheck(void);
void allStop(void);
void setup(void);

#define STOP 0x00
#define FORWARD 0x06
#define BACKWARD 0x09
#define RTURN 0x04
#define LTURN 0x02
#define SLEFT 0x0A
#define SRIGHT 0x07

#define BR 0
#define FR 1
#define BL 2
#define FL 3
#define PR1 P2IN & BIT3 //back
#define PR2 P2IN & BIT4 //front
#define PR3 P2IN & BIT5 //right
#define PR4 P2IN & BIT6 //left
#define PRALL BIT3 | BIT4 | BIT5 | BIT6

int dark = 0;
int doubleHit = 0;

/*
 * main.c
 */
 int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    setup();

	P2OUT &= ~BIT7;
	allStop();
	while(!buttonHit()); //press button to start
	P2OUT |= BIT7;
	__delay_cycles(1600000);
	P2OUT &= ~BIT7;

	P2IES &= ~(PRALL);				// interrupt on low to high transition
	P2IFG &= ~(PRALL);				// Set flag off (button is not yet pressed)

	int button = 0;
    while(1){
    	if(dark){
        	P2IE  |= PRALL;				// Interrupt enable on button
        	_BIS_SR(GIE);
    		while(dark){
    			allStop();
    		}
    		P2IES &= ~(PRALL);				// interrupt on low to high transition
    		motorDrive(FORWARD);
    	}
    	else{
        	_BIS_SR(GIE);
    		motorDrive(FORWARD);
    	}

    	P2IE  |= PRALL;				// Interrupt enable on PR
    	_BIS_SR(GIE);
    	while(!(button = buttonHit()) && !dark);
    	_BIC_SR(GIE);

    	if(button & 2 && button & 1){ //emergency stop
    		allStop();
    		__delay_cycles(16000000);
        	while(!buttonHit());
    	}

    	if(!dark){
			allStop();
			motorDrive(BACKWARD);
			_BIS_SR(GIE);
			__delay_cycles(700000);
			_BIC_SR(GIE);

			if(button & 2){
				doubleHit & 2 ? motorDrive(SRIGHT) : motorDrive(RTURN);
				doubleHit |= 2;
				doubleHit &= ~1;
			}
			else if(button & 1){
				doubleHit & 1 ? motorDrive(SLEFT) : motorDrive(LTURN);
				doubleHit |= 1;
				doubleHit &= ~2;
			}
			_BIS_SR(GIE);
			__delay_cycles(1000000);
			_BIC_SR(GIE);
    	}
    }
}

/*
 * Interrupt service routine
 * parameter: void
 * return: void
 */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
	static int high = 0;
	if(high = !high){
		P2OUT |= BIT0;
		CCR0 += 0x0100;
	}
	else{
		P2OUT &= ~BIT0;
		CCR0 += 0x0200;
	}
}

#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void) {
	/*
	dark = dark & 0xF0 | P2IN >> 3;
	switch(dark){
		case 0x0F: //all dark
			allStop();
    		P2IES |= PRALL;				// interrupt on high to low transition
			break;
		case 0x00: //all light
			P2IES &= ~(PRALL);				// interrupt on low to high transition
			motorDrive(FORWARD);
			break;
		default:

			break;
	}*/
	if(!dark || ((P2IN & (PRALL)) == PRALL)){
		dark ^= 1;
		P2OUT ^= BIT7;
		P2IE  &= ~(PRALL);				// Interrupt disable on button
		P2IFG &= ~(PRALL);				// Set flag off (button is not yet pressed)
	}
	if(dark)
		allStop();
}

/*
 * Setup routine
 */
void setup(){
    P1DIR = 0xFF; //motor control output
    P2DIR = 0x81;

    //sets up P2.6 and P2.7 as I/O pin
    P2SEL &= ~(BIT7 + BIT6);
    P2SEL2 &= ~(BIT7 + BIT6);

    //interrupt pwm setup
    CCTL0 = CCIE;
    CCR0 = 0xFF;
    TACTL = TASSEL_2 + MC_2;
	_BIS_SR(GIE);
}
/*
 * return BIT1 == left or BIT2 == right depending on buttonpress
 * 2 if right // 1 if left
 */
int buttonHit(){
	int ret = 0;
	if(P2IN & BIT2)
		ret |= 2;
	if(P2IN & BIT1)
		ret |= 1;
	return ret;
}

/*
 * return 0 if in shade
 * return bit 1 if top left sensor
 * return bit 2 if top right sonar
 * retrurn bit 3
 */
int lightCheck(){
	//int ret = 0;
	if(P2IN & (PRALL))
		return 1;
	return 0;
	/*
	if(P2IN & BIT3)
		ret |= BIT0;
	if(P2IN & BIT4)
		ret |= BIT1;
	if(P2IN & BIT5)
		ret |= BIT2;
	return ret;*/
}

/*
 *
 */
void motorDrive(int control){
	P1OUT = P1OUT & 0xF0 | control;
	if(dark)
		allStop();
}

void allStop(){
	P1OUT &= 0xF0;
}
