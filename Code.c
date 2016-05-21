/*
 * Matthew Ng and Eric Chen
 * CPE329 - Gerfen
 * Final Project
 */
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
#define RTURN 0x02
#define LTURN 0x04
#define SLEFT 0x05
#define SRIGHT 0x0A

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
int button = 0;

/*
 * main code
 * trapps the code in an infinite loop
 */
 int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    setup();

	allStop();
	while(!buttonHit()){ //press button to start; calibration
		if((P2IN & (PRALL)) == (PRALL))
			P1OUT |= BIT4 | BIT5;
		else if(PR1 && PR2){
			P1OUT |= BIT4;
			P1OUT &= ~BIT5;
		}
		else if(PR3 && PR4){
			P1OUT |= BIT5;
			P1OUT &= ~BIT4;
		}
		else
			P1OUT &= ~(BIT4 | BIT5);
	}
	__delay_cycles(1600000);

	P2IES &= ~(PRALL);				// interrupt on low to high transition
	P2IFG &= ~(PRALL);				// Set flag off (button is not yet pressed)

    while(1){
		P2IE  |= PRALL;				// Interrupt enable on PR
    	if(!dark){
    		motorDrive(FORWARD);
    	}

    	button = 0;
    	_BIS_SR(GIE);
    	while(!(button = buttonHit()) && !dark || ((dark & 0x0F) == 0x0F)){
    		if(lightCheck() == 0x0F){
    			dark = 0x0F;
    			allStop();
    		}
    	}
    	_BIC_SR(GIE);

    	if(button & 2 && button & 1){ //emergency stop
    		allStop();
    		button = 0;
    		__delay_cycles(3200000);
        	while(!buttonHit());
    	}

    	if(button){
    		P2IE  &= ~PRALL;				// Interrupt enable on PR
			motorDrive(BACKWARD);
			_BIS_SR(GIE);
			__delay_cycles(1000000);
			_BIC_SR(GIE);

			if(button & 2){
				(doubleHit & 2) ? motorDrive(SLEFT) : motorDrive(LTURN);
				doubleHit |= 2;
				doubleHit &= ~1;
			}
			else if(button & 1){
				(doubleHit & 1) ? motorDrive(SRIGHT) : motorDrive(RTURN);
				doubleHit |= 1;
				doubleHit &= ~2;
			}
			_BIS_SR(GIE);
			__delay_cycles(1000000);
			_BIC_SR(GIE);
			allStop();
			P2IE  |= PRALL;				// Interrupt enable on PR
    	}
    }
}


/*
 * Interrupt vector for button
 * pins on port 2
 */
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void) {
	dark = 0x0F & (P2IN >> 3);
	if(dark)
		P1OUT |= BIT4 | BIT5;
	else
		P1OUT &= ~(BIT4 | BIT5);

	switch(dark){
		case 0x0F: //all dark
		case 0x01:
			P2IE  |= PRALL;				// Interrupt enable on button
    		P2IES |= PRALL;				// interrupt on high to low transition
    		motorDrive(STOP);
    		break;
		case 0x00: //all light
			P2IE  |= PRALL;				// Interrupt enable on button
			P2IES &= ~(PRALL);				// interrupt on low to high transition
			if(!button)
				motorDrive(FORWARD);
			break;
		case 0x02: //just front
			P2IE  |= PRALL;				// Interrupt enable on button
			P2IE &= ~BIT4;
			motorDrive(FORWARD);
			break;
		case 0x04: //just right
			P2IE  |= PRALL;				// Interrupt enable on button
			P2IE &= ~BIT5;
		case 0x06: //front right
			P2IE  |= PRALL;				// Interrupt enable on button
			P2IE &= ~(BIT4 | BIT5);
			motorDrive(RTURN);
			break;
		case 0x08: //just left
			P2IE  |= PRALL;				// Interrupt enable on button
			P2IE &= ~BIT6;
		case 0x0A: //front left
			P2IE  |= PRALL;				// Interrupt enable on button
			P2IE &= ~(BIT4 | BIT6);
			motorDrive(LTURN);
			break;
		default:
			break;
	}
	P2IFG &= ~(PRALL);				// Set flag off (button is not yet pressed)
}

/*
 * Setup routine
 * sets up the output pins, LED's
 * and the interrupt timing vector
 */
void setup(){
    P1DIR |= 0x0F | BIT4 | BIT5 | BIT6; //motor control output //LED's // pwm
    P2DIR |= 0x81;

    //sets up P2.6 and P2.7 as I/O pin
    P2SEL &= ~(BIT7 + BIT6);
    P2SEL2 &= ~(BIT7 + BIT6);
    P1SEL |= BIT6;                            // P1.6 TA1/2 options

    //interrupt pwm setup
    TACCTL1 = OUTMOD_7;      // CCR1 reset/set
    TACCR0 = 0x0300;         // PWM Period
    TACCR1 = 0x0150;         // PWM duty cycle
    TACTL = TASSEL_2 | MC_1; // SMCLK + upmode
    _BIS_SR(GIE);
}

/*
 * Checks the buttonHit pins
 * Paramter: void
 * Return: int(0x03) for input pin
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
 * Checks the photoresistor values
 * Parameter: void
 * Return: int (0x0F) for the pins that are dark
 */
int lightCheck(){
	return ((P2IN >> 3) & 0x0F);
}

/*
 * Controls the motors with a defined input
 * Paramter: STOP, FORWARD, BACKWARD, RTURN, LTURN, SLEFT, SRIGHT 
 * Return: void
 */
void motorDrive(int control){
	P1OUT = P1OUT & 0xF0 | control;
}

/*
 * Automatic release of all the motors
 * Paramter: void
 * Return: void
 */
void allStop(){
	P1OUT &= 0xF0;
}
