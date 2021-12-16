 /***************************************************************************************
 *
 * [File Name]: HMI_mcu.c
 *
 * [Author]: Alaa Elsayed
 *
 * [Date Created]: 1 / 11 / 2021
 *
 * [Description]: Source file for Human Interface MCU
 * 				  This unit connected to ( Keypad / LCD )
 * 				  It is responsible of taking the actions and decisions from the user
 *
 ***************************************************************************************/

/*******************************************************************************
 *                                  Modules                                    *
 *******************************************************************************/

#include "micro_config.h"
#include <avr/io.h>
#include <util/delay.h>

#include "HMI_mcu.h"
#include "uart.h"
#include "lcd.h"
#include "keypad.h"
#include "timer.h"


/*******************************************************************************
 *                              Global Variables                               *
 *******************************************************************************/

/* Global array to store the password inputed from the user */
uint8 g_inputPassword[PASSWORD_LENGTH];

/* Global Variable to store the status of the Password after comparing */
uint8 g_matchStatus = PASS_MIS_MATCHED;

/* Global Variable to keep track of the seconds counted by the timer */
uint8 g_tick = 0;

/* Global Variable to keep track of how many times the user has inputed the password incorrectly */
uint8 g_passwordMistakes = 0;

/* Global Variable to keep track of the command sent from the CONTROL MCU through UART */
uint8 g_command;

/*******************************************************************************
 *                      Functions Definitions                                  *
 *******************************************************************************/

/* Function main begins program execution */
int main(void)
{
	/**************************** Initialization Code ****************************/

	/* Variable to store the pressed key */
	uint8 key_option;

	SREG  |= ( 1 << 7 ); /* Enable Global Interrupts */

	/* Initialize the UART with Configuration */
	UART_ConfigType UART_Config = {9600, EIGHT_BITS, ONE_STOP_BIT, DISABLED};
	UART_init(&UART_Config);

	/* Initialize LCD */
	LCD_init();

	/* Welcome message and Important note */
	LCD_moveCursor(0, 4);
	LCD_displayString("Welcome");
	LCD_moveCursor(1, 0);
	LCD_displayString("Use (=) as Enter");
	_delay_ms(STAND_PRESENTATION_TIME);
	LCD_clearScreen();

	/* Set the Password for the first time */
	HMI_newPassword();

	/* Super Loop */
	for(;;)
	{
		/**************************** Application Code ****************************/

		/* Display the main options to the screen to make the user decide */
		HMI_mainOptions();

		/* Store which key has been pressed for later use */
		key_option = KEYPAD_getPressedKey();

		/* Depending on the pressed key, Perform some operation */
		switch (key_option)
		{

		/************** OPEN DOOR CASE *************/
		case OPEN_DOOR:

			/* Ask the user to input a password */
			HMI_promptPassword();
			/* Ask CONTROL MCU to check the Password */
			HMI_sendCommand(SEND_CHECK_PASSWORD);
			/* Send the inputed password to the CONTROL MCU */
			HMI_sendPassword(g_inputPassword);
			/* Inform CONTROL MCU what the user has chosen */
			HMI_sendCommand(OPEN_DOOR);

			/* Receive the order command from CONTROL MCU */
			g_matchStatus = HMI_receiveCommand();

			/* In case the two passwords matches */
			if(g_matchStatus ==  OPENING_DOOR)
			{
				/* Begin unLocking and Locking the Door */
				HMI_openingDoor();
			}
			/* In case the two passwords did not match */
			else if(g_matchStatus == WRONG_PASSWORD)
			{
				/* Begin wrong operation protocol */
				HMI_wrongPassword();
			}

			break; /* End of open door case */

		/************* CHANGE PASSWORD CASE *************/
		case CHANGE_PASSWORD:

			/* Ask the user to input a password */
			HMI_promptPassword();
			/* Ask CONTROL MCU to check the Password */
			HMI_sendCommand(SEND_CHECK_PASSWORD);
			/* Send the inputed password to the CONTROL MCU */
			HMI_sendPassword(g_inputPassword);
			/* Inform CONTROL MCU what the user has chosen */
			HMI_sendCommand(CHANGE_PASSWORD);

			/* Receive the order command from CONTROL MCU */
			g_matchStatus = HMI_receiveCommand();

			/* In case the two passwords matches */
			if(g_matchStatus ==  CHANGING_PASSWORD)
			{
				/* Set new password for MCU */
				HMI_newPassword();
			}
			/* In case the two passwords did not match */
			else if(g_matchStatus == WRONG_PASSWORD)
			{
				/* Begin wrong operation protocol */
				HMI_wrongPassword();
			}

			break; /* End of change password case */

		} /* End switch case */
	} /* End Super Loop */
} /* End main Function */



void HMI_TimerCallBackProcessing(void)
{
	g_tick++; /* Increment the counter */
} /* End HMI_TimerCallBackProcessing Function */



/*
 * Description :
 * Produce a delay timer using Polling Technique
 * TCNT0 = 0	,,	0 --> 7813	,,	Ticks = 7814
 * F)cpu = 8 Mhz // Prescaler = 1024
 * F)timer = F)cpu / Prescaler = ( 8 Mhz ) / 1024								----->	F)timer = 7812.5
 * Resolution = 1 / F)timer = 1 / 7812.5										----->	Resolution = 128 us
 * Maximum suitable number of counts per compare match = 7812.5 Counts			----->	T)ctc = 1000 us
 * Number of compare matches per One Second = 1 / T)ctc = 1 / ( 1000 us )		----->	Number of compare matches per One Second = 1
 */
void HMI_startTimer(void)
{
	/* Setup Timer Configuration */
	TIMER_ConfigType TIMER_Config = { TIMER1_ID, TIMER_CTC_Mode, 0, F_CPU_1024, 7813 };

	/* Initialize the Timer */
	Timer_init(&TIMER_Config);

	/* Set Call Back function for the timer */
	Timer_setCallBack(HMI_TimerCallBackProcessing, TIMER1_ID);
} /* End HMI_startTimer Function */



void HMI_sendCommand(uint8 g_command)
{
	/* Inform CONTROL MCU that you are to send */
	UART_sendByte(READY_TO_SEND);

	/* Wait until CONTROL MCU are ready to receive */
	while(UART_recieveByte() != READY_TO_RECEIVE);

	/* Send the required command to the CONTROL MCU */
	UART_sendByte(g_command);

	/* Wait until the CONTROL MCU receive the command */
	while(UART_recieveByte() != RECEIVE_DONE);
} /* End HMI_sendCommand Function */



uint8 HMI_receiveCommand(void)
{
	/* Wait until the CONTROL MCU is ready to send */
	while(UART_recieveByte() != READY_TO_SEND);

	/* Inform the CONTROL MCU that you are ready to receive */
	UART_sendByte(READY_TO_RECEIVE);

	/* Receive the command from the CONTROL MCU */
	g_command = UART_recieveByte();

	/* Inform the CONTROL MCU that the receive has been done successfully */
	UART_sendByte(RECEIVE_DONE);

	return g_command; /* Return the command value */
} /* End HMI_receiveCommand Function */



void HMI_newPassword(void)
{
	/* Set its status at first as mis-matched */
	g_matchStatus = PASS_MIS_MATCHED;

	/* Loop until the HMI MCU get the same password */
	while(g_matchStatus == PASS_MIS_MATCHED)
	{
		LCD_clearScreen(); /* Clear Screen */
		LCD_displayString("  New Password  "); /* Inform the user that he will input new password */
		_delay_ms(STAND_PRESENTATION_TIME); /* Hold for Presentation Time */

		LCD_clearScreen(); /* Clear Screen */
		LCD_displayString("Enter Password"); /* Prompt the user to input the password for the first time */
		LCD_moveCursor(1,0); /* Move Cursor to the second line */
		HMI_getPassword(g_inputPassword); /* Get the password from the user */

		HMI_sendCommand(SEND_FIRST_PASSWORD); /* Inform the CONTROL MCU that you will send the first password */
		HMI_sendPassword(g_inputPassword); /* Send the password to the CONTROL MCU */


		LCD_clearScreen(); /* Clear Screen */
		LCD_displayString("ReEnter Password"); /* Prompt the user to input the password for the second time */
		LCD_moveCursor(1,0); /* Move Cursor to the second line */
		HMI_getPassword(g_inputPassword); /* Get the password from the user */

		HMI_sendCommand(SEND_SECOND_PASSWORD); /* Inform the CONTROL MCU that you will send the second password */
		HMI_sendPassword(g_inputPassword); /* Send the password to the CONTROL MCU */

		/* Wait until the is able to send the confirmation of the second password */
		g_matchStatus = HMI_receiveCommand();

		/* In case the Two Passwords did not match */
		if (g_matchStatus == PASS_MIS_MATCHED)
		{
			LCD_clearScreen(); /* Clear Screen */
			LCD_displayString("MISMATCHED Pass"); /* Display an Error Message */
			_delay_ms(STAND_PRESENTATION_TIME); /* Hold for Presentation Time */
		} /* End if */
	} /* End while loop */
} /* End HMI_newPassword Function */



void HMI_sendPassword(uint8 a_inputPassword[])
{
	uint8 counter; /* Variable to work as a counter */

	/* Loop on the passwords elements */
	for( counter = 0; counter < PASSWORD_LENGTH; counter++)
	{
		UART_sendByte(a_inputPassword[counter]); /* Send Password element by element to the CONTROL MCU */
		_delay_ms(SEND_RECEIVE_TIME);      /* Delay for the time gap for sending receiving time between the MCUs */
	} /* End for */
} /* End HMI_sendPassword Function */



void HMI_getPassword(uint8 a_inputPassword[])
{
	LCD_moveCursor(1, 0);

	uint8 counter = 0; /* Variable to be used as a counter */
	uint8 password_key = 0; /* Variable to store the pressed key */

	/* Stop getting number after you get 5 characters */
	while( counter != PASSWORD_LENGTH )
	{
		password_key = KEYPAD_getPressedKey(); /* Get the get the key pressed and store it in the password array */

		if ( (password_key >= 0) && (password_key <= 9) )
		{
			LCD_displayCharacter('*'); /* Display asterisk for privacy */
			a_inputPassword[counter] = password_key;
			counter++;
		}
		_delay_ms(KEYPAD_CLICK_TIME); /* Delay time for keypad press */
	} /* End while loop */

	/* Don't leave until the user press (=) symbol */
	while ( KEYPAD_getPressedKey() != '=' );
} /* End HMI_getPassword Function */



void HMI_mainOptions(void)
{
	LCD_clearScreen(); /* Clear Screen */
	LCD_displayString("(+): Open Door"); /* Display the first option */
	LCD_moveCursor(1,0); /* Move to the next line */
	LCD_displayString("(-): Change Pass"); /* Display the second option */
} /* End HMI_mainOptions Function */



void HMI_promptPassword(void)
{
	LCD_clearScreen(); /* Clear Screen */
	LCD_displayString("Enter Password :"); /* Prompt the user to write the password */
	HMI_getPassword(g_inputPassword); /* Takes the password and store it in an array */
} /* End HMI_promptPassword Function */



void HMI_openingDoor(void)
{
	HMI_startTimer(); /* Start the timer to measure time period */

	/* Open the door for ( 15 sec ) */
	LCD_clearScreen(); /* Clear Screen */
	LCD_displayString("Door is Opening"); /* Display explanation message on LCD */
	while(g_tick < OPEN_DOOR_TIME); /* Count up to 15 */
    g_tick = 0; /* Reset counter to reuse it */

	/* Hold the door for ( 3 sec ) */
	LCD_clearScreen(); /* Clear Screen */
	LCD_displayString("Door is on Hold"); /* Display explanation message on LCD */
	while(g_tick < HOLD_DOOR_TIME); /* Count up to 3 */
    g_tick = 0; /* Reset counter to reuse it */

	/* Open the door for ( 15 sec ) */
	LCD_clearScreen(); /* Clear Screen */
	LCD_displayString("Door is Closing"); /* Display explanation message on LCD */
	while(g_tick < CLOSE_DOOR_TIME); /* Count up to 15 */
    g_tick = 0; /* Reset counter to reuse it */

    Timer_DeInit(TIMER1_ID); /* Stop the timer */
    LCD_clearScreen(); /* Clear Screen */
} /* End HMI_openingDoor Function */



void HMI_wrongPassword(void)
{
	g_passwordMistakes++; /* Increment the wrong counter */

	LCD_clearScreen(); /* Clear Screen */
	LCD_displayString(" Wrong Password "); /* Display explanation message on LCD */
	_delay_ms(STAND_PRESENTATION_TIME); /* Hold for Presentation Time */

	/* If the user entered the password 3 times wrong */
	if(g_passwordMistakes == MAX_NUM_OF_MISTAKES)
	{
		HMI_startTimer(); /* Start the timer to measure time period */

		LCD_clearScreen(); /* Clear Screen */
		LCD_displayString("!!! WARNING !!!"); /* Display warning message on LCD */

		while(g_tick != WARNING_TIME); /* Display the message for one minute */

		/* Reset the counters */
		g_passwordMistakes = 0;
		g_tick = 0;

	    Timer_DeInit(TIMER1_ID); /* Stop the timer */
	} /* End if */

    LCD_clearScreen(); /* Clear Screen */
} /* End HMI_wrongPassword Function */
