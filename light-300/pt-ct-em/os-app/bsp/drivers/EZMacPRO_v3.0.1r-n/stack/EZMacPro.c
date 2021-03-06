/*!\file EZMacPro.c
 * \brief EZMacPRO stack functions.
 *
 * \n EZMacPRO version: 3.0.1r
 *
 * \n This software must be used in accordance with the End User License
 * \n Agreement.
 *
 * \b COPYRIGHT
 * \n Copyright 2012 Silicon Laboratories, Inc.
 * \n http://www.silabs.com
 */

#include "stack.h"

#if 0
#define ezmacprof_debug(x) rt_kprintf x
#else
#define ezmacprof_debug(x)
#define ezmacprof_log(x)	rt_kprintf x

#endif

/* ======================================= *
 *     G L O B A L   V A R I A B L E S     *
 * ======================================= */

SEGMENT_VARIABLE(EZMacProReg, EZMacProUnion, REGISTER_MSPACE);
SEGMENT_VARIABLE(EZMacProCurrentChannel, U8, EZMAC_PRO_GLOBAL_MSPACE);

#ifdef EXTENDED_PACKET_FORMAT
SEGMENT_VARIABLE(TimeoutACK, U32, EZMAC_PRO_GLOBAL_MSPACE);
SEGMENT_VARIABLE(EZMacProSequenceNumber, U8, EZMAC_PRO_GLOBAL_MSPACE);
SEGMENT_VARIABLE(PreamRegValue, U8, EZMAC_PRO_GLOBAL_MSPACE);
#endif//EXTENDED_PACKET_FORMAT

#ifndef TRANSMITTER_ONLY_OPERATION
BIT fHeaderErrorOccurred;
SEGMENT_VARIABLE(RxBuffer[RECEIVED_BUFFER_SIZE], U8 , BUFFER_MSPACE);
#ifdef EXTENDED_PACKET_FORMAT
SEGMENT_VARIABLE(AckBufSize, U8 , EZMAC_PRO_GLOBAL_MSPACE);
SEGMENT_VARIABLE(AckBuffer[ACK_BUFFER_SIZE], U8 , BUFFER_MSPACE);
#endif//EXTENDED_PACKET_FORMAT
SEGMENT_VARIABLE(EZMacProReceiveStatus, U8, EZMAC_PRO_GLOBAL_MSPACE);
SEGMENT_VARIABLE(EZMacProRSSIvalue, U8, EZMAC_PRO_GLOBAL_MSPACE);
SEGMENT_VARIABLE(TimeoutSyncWord, U32, EZMAC_PRO_GLOBAL_MSPACE);
SEGMENT_VARIABLE(TimeoutRX_Packet, U32, EZMAC_PRO_GLOBAL_MSPACE);
#ifndef B1_ONLY
SEGMENT_VARIABLE(RX_Freq_dev, U8, EZMAC_PRO_GLOBAL_MSPACE);
#endif//B1_ONLY
SEGMENT_VARIABLE(TimeoutChannelSearch, U32, EZMAC_PRO_GLOBAL_MSPACE);
#endif//TRANSMITTER_ONLY_OPERATION

#ifndef RECEIVER_ONLY_OPERATION
SEGMENT_VARIABLE(TimeoutTX_Packet, U32, EZMAC_PRO_GLOBAL_MSPACE);
#ifndef B1_ONLY
SEGMENT_VARIABLE(TX_Freq_dev, U8, EZMAC_PRO_GLOBAL_MSPACE);
#endif//B1_ONLY
#endif//RECEIVER_ONLY_OPERATION

#ifdef TRANSCEIVER_OPERATION
SEGMENT_VARIABLE(TimeoutLBTI, U32, EZMAC_PRO_GLOBAL_MSPACE);

/* if RSSI interrupt is occured set the LBT(listen before talk) flag */
SEGMENT_VARIABLE(BusyLBT, U8, EZMAC_PRO_GLOBAL_MSPACE);
SEGMENT_VARIABLE(EZMacProRandomNumber, U8, EZMAC_PRO_GLOBAL_MSPACE);
SEGMENT_VARIABLE(EZMacProLBT_Retrys, U8, EZMAC_PRO_GLOBAL_MSPACE);
#endif//TRANSCEIVER_OPERATION

#ifdef PACKET_FORWARDING_SUPPORTED
SEGMENT_VARIABLE(ForwardedPacketTable[FORWARDED_PACKET_TABLE_SIZE], ForwardedPacketTableEntry, FORWARDED_PACKET_TABLE_MSPACE);
#endif//PACKET_FORWARDING_SUPPOERTED

#ifdef ANTENNA_DIVERSITY_ENABLED
SEGMENT_VARIABLE(Selected_Antenna, U8, EZMAC_PRO_GLOBAL_MSPACE);
#endif//ANTENNA_DIVERSITY_ENABLED

#ifdef MORE_CHANNEL_IS_USED
SEGMENT_VARIABLE(SelectedChannel, U8, EZMAC_PRO_GLOBAL_MSPACE);
SEGMENT_VARIABLE(maxChannelNumber, U8, EZMAC_PRO_GLOBAL_MSPACE);
#endif //MORE_CHANNEL_IS_USED


static void SetRfParameters(U8 mcr);
static void macSpecialRegisterSettings(U8 chiptype);
static void macUpdateDynamicTimeouts(U8 mcr, U8 mpl);

#ifdef PACKET_FORWARDING_SUPPORTED
static void initForwardedPacketTable (void);
#endif //PACKET_FORWARDING_SUPPORTED

static void macSetEnable2(U8 value);
static void macSetFunction1(U8 value);

#ifdef TRANSCEIVER_OPERATION
static void macUpdateLBTI (U8 lbti);
#endif//TRANSCEIVER_OPERATION

/* ======================================= *
 *     P U B L I C  F U N C T I O N S      *
 * ======================================= */

//------------------------------------------------------------------------------------------------
// Function Name:	EZMacPRO_Init()
//		Initializes the EZRadioPRO device. The function has to be called in the power-on routine.
// Return Values : MAC_OK: if the MAC recognized the used chip
//		CHIPTYPE_ERROR: if the MAC didn't recognize the used chip
//------------------------------------------------------------------------------------------------
MacParams EZMacPRO_Init(void)
{
	U8 temp8;

	DISABLE_MAC_INTERRUPTS();                     // disable MAC interrupts

	//Set the init value of the MAC registers
	for(temp8=0; temp8<EZ_LASTREG; temp8++)
		EZMacProReg.array[temp8] = 0x00;

	EZMacProReg.name.MCR    = 0x1C;
	EZMacProReg.name.SECR   = 0x50;
	EZMacProReg.name.TCR    = 0x38;
	EZMacProReg.name.RCR    = 0x04;
#ifdef FOUR_CHANNEL_IS_USED
	EZMacProReg.name.FR1    = 1;
	EZMacProReg.name.FR2    = 2;
	EZMacProReg.name.FR3    = 3;
#endif//FOUR_CHANNEL_IS_USED
#ifdef MORE_CHANNEL_IS_USED
	for(temp8=0; temp8<50; temp8++)
		EZMacProReg.array[4+temp8] = FrequencyTable[temp8];
#endif//MORE_CHANNEL_IS_USED
	EZMacProReg.name.PFCR   = 0x02;
	EZMacProReg.name.MPL    = 0x40;
	EZMacProReg.name.MSR    = 0x00;
	EZMacProReg.name.SCID   = 0xCD;
	EZMacProReg.name.SFID   = 0x01;
	EZMacProReg.name.PLEN   = 0x01;
#ifdef TRANSCEIVER_OPERATION
	EZMacProReg.name.LBTLR  = 0x78; // -60 dBm
	EZMacProReg.name.LBTIR  = 0x8A;
#endif//TRANSCEIVER_OPERATION
	EZMacProReg.name.LBDR   = 0x14;
	EZMacProReg.name.LFTMR2 = 0x40;

	// read Si443x interrupts to clear
	macSpiReadReg(SI4432_INTERRUPT_STATUS_1);
	macSpiReadReg(SI4432_INTERRUPT_STATUS_2);
	CLEAR_MAC_EXT_INTERRUPT();          // clear INT0 flag

	//enable only the chip ready interrupt
	macSpiWriteReg(SI4432_INTERRUPT_ENABLE_1, 0x00);
	macSpiWriteReg(SI4432_INTERRUPT_ENABLE_2, SI4432_ENCHIPRDY);


	//EZMac state Software Reset
	set_ezmacpro_msr(EZMAC_PRO_WAKE_UP);
	//Call the wake-up state entered callback.
	EZMacPRO_StateWakeUpEntered();
	//Send the Software Reset Command to the radio
	//use direct write since LBD and WUT initially disabled
	macSpiWriteReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_1, (SI4432_SWRES|SI4432_XTON));

	// set time out to XTAL start-up time
	macTimeout (TIMEOUT_XTAL_START);

	ENABLE_MAC_INTERRUPTS();         // enable INT0  & T0 interrupt

	//wait until the MAC goes to Idle State
	while (EZMacProReg.name.MSR != EZMAC_PRO_IDLE) {
		/* 唤醒超时后, EZMacProReg.name.MSR将会处于WAKE_UP_ERROR */
		if (WAKE_UP_ERROR==EZMacProReg.name.MSR || WAKE_UP_ERROR_NOT_TIMEOUT==EZMacProReg.name.MSR) {
			ezmacprof_log(("%s(), line:%d, wakeup error(0x%x)\n", __FUNCTION__, __LINE__, EZMacProReg.name.MSR));
			return WAKEUP_ERROR; /* mark by David */
		}
	}

	// clear Chip ready and POR interrupts
	macSpiWriteReg(SI4432_INTERRUPT_ENABLE_1, 0x00);
	macSpiWriteReg(SI4432_INTERRUPT_ENABLE_2, 0x00);

	/*Si443x configuration*/

#ifdef EXTENDED_PACKET_FORMAT
	if (( EZMacProReg.name.MCR & 0x80 ) == 0x80)//if CID is used
		//header length 4 byte(CTRL+CID+SID+DID)
		macSpiWriteReg(SI4432_HEADER_CONTROL_2, SI4432_HDLEN_4BYTE | (SYNC_WORD_LENGTH - 1) << 1);
	else
		//header length 3 byte(CTRL+SID+DID)
		macSpiWriteReg(SI4432_HEADER_CONTROL_2, SI4432_HDLEN_3BYTE | (SYNC_WORD_LENGTH - 1 ) << 1);
#else //STANDARD_PACKET_FORMAT
	if (( EZMacProReg.name.MCR & 0x80 ) == 0x80)//if CID is used
		//header length 3 byte(CID+SID+DID)
		macSpiWriteReg(SI4432_HEADER_CONTROL_2, SI4432_HDLEN_3BYTE | (SYNC_WORD_LENGTH - 1) << 1);
	else
		//header length 2 byte(SID+DID)
		macSpiWriteReg(SI4432_HEADER_CONTROL_2, SI4432_HDLEN_2BYTE | (SYNC_WORD_LENGTH - 1) << 1);
#endif//EXTENDED_PACKET_FORMAT
	//Clear the Header Control Register
	macSpiWriteReg(SI4432_HEADER_CONTROL_1, 0x00);
	//Set the SYNC WORD
	macSpiWriteReg(SI4432_SYNC_WORD_3, 0x2D);
	macSpiWriteReg(SI4432_SYNC_WORD_2, 0xD4);

	//Set the modem, frequency parameters and preamble length according to the init register value
	SetRfParameters(EZMacProReg.name.MCR);

	//set GPIO0, GPIO1 and GPIO2 to the init values
	//the init values are in the EZMacPro_defs.h
	macSpiWriteReg(SI4432_GPIO0_CONFIGURATION, GPIO0_FUNCTION);
	macSpiWriteReg(SI4432_GPIO1_CONFIGURATION, GPIO1_FUNCTION);
	macSpiWriteReg(SI4432_GPIO2_CONFIGURATION, GPIO2_FUNCTION);

	//enable packet handler & CRC16
	macSpiWriteReg(SI4432_DATA_ACCESS_CONTROL, SI4432_ENPACTX | SI4432_ENPACRX | SI4432_ENCRC | SI4432_CRC_16);

	//FIFO mode, GFSK modulation, TX Data Clk is available via the GPIO
	macSpiWriteReg(SI4432_MODULATION_MODE_CONTROL_2, SI4432_MODTYP_GFSK | SI4432_FIFO_MODE | SI4432_TX_DATA_CLK_GPIO);

	//reset digital testbus, disable scan test
	macSpiWriteReg(SI4432_DIGITAL_TEST_BUS, 0x00);

	//select nothing to the Analog Testbus
	macSpiWriteReg(SI4432_ANALOG_TEST_BUS, 0x0B);

#ifdef PACKET_FORWARDING_SUPPORTED
	initForwardedPacketTable ();
#endif
#ifndef B1_ONLY
	//read out the device type register
	temp8 = 	macSpiReadReg (SI4432_DEVICE_VERSION);
	if (temp8 == 0x02) EZMacProReg.name.DTR = 0x00;				//revision V2
	else if (temp8 == 0x04) EZMacProReg.name.DTR = 0x01;		//revision A0
	else if (temp8 == 0x06) EZMacProReg.name.DTR = 0x02;		//revision B1
	else {
		EZMacProReg.name.DTR = 0x02;				//default: rev B1
		temp8 = 1;
	}
#endif
#ifdef B1_ONLY
	EZMacProReg.name.DTR = 0x02;					//default: rev B1
#endif
	//set the non-default, special registers accoring to the device type
	macSpecialRegisterSettings(EZMacProReg.name.DTR);

#ifndef B1_ONLY
	if (EZMacProReg.name.DTR == 0x00)//if rev V2 chip is used
		//set crystal oscillator control test
		macSpiWriteReg (SI4432_CRYSTAL_OSCILLATOR_CONTROL_TEST, 0x24);
#endif//B1_ONLY

#ifdef ANTENNA_DIVERSITY_ENABLED
	Selected_Antenna = 1;
	//switch ON the internal algorithm
	temp8 = macSpiReadReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_2);
	macSpiWriteReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_2, temp8 | 0x80);
#endif//ANTENNA_DIVERSITY_ENABLED

	// stop Si443x xtal, clear TX, RX , PLL
	// use direct write since LBD and WUT initially disabled
	macSpiWriteReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_1, 0x00);

	//after EZMAC init the next state is SLEEP
	set_ezmacpro_msr(EZMAC_PRO_SLEEP);
	//Call the Sleep state entered callback.
	EZMacPRO_StateSleepEntered();
#ifndef B1_ONLY
	if(temp8 == 0)
		//if the MAC recognize the chip type
		return MAC_OK;
	else
		//if the MAC doesn't recognize the chip type
		return CHIPTYPE_ERROR;
#endif//B1_ONLY
#ifdef B1_ONLY
	return MAC_OK;
#endif//B1_ONLY
}


//-----------------------------------------------------------------------------------------------
// Function Name :	EZMAcPRO_Sleep()
//		Switch the MAC from IDLE to SLEEP mode. It turns off the crystal oscillator
//	on the radio. This function can be call from only Idle state.
// Return Values :	MAC_OK: the operation was successful.
//			STATE_ERROR: the operation is ignored because the MAC was not in IDLE mode
//-----------------------------------------------------------------------------------------------
MacParams EZMacPRO_Sleep(void)
{
	if (EZMacProReg.name.MSR == EZMAC_PRO_IDLE) {
		DISABLE_MAC_INTERRUPTS();         // disable MAC interrupts, just in case

		if (EZMacProReg.name.DTR == 0x00) //if rev V2 chip is used
			//this register setting is needed (only rev V2)
			macSpiWriteReg (SI4432_CRYSTAL_OSCILLATOR_CONTROL_TEST, 0x24);

		// disable Si443x interrupt sources
		macSpiWriteReg(SI4432_INTERRUPT_ENABLE_1, 0x00);
		/* Keep WUT and LBD enabled if these are enabled in LFTMR2 and LBDR. */
		macSetEnable2(0x00);

		// stop Si443x xtal, clear TX, RX , PLL
		macSetFunction1(0x00);
		// next state after this function is SLEEP
		set_ezmacpro_msr(EZMAC_PRO_SLEEP);
		//Call the Sleep state entered callback.
		EZMacPRO_StateSleepEntered();
		/* Enable External0 INT for WUT and LBD to work correctly. */
		ENABLE_MAC_EXT_INTERRUPT();
		return MAC_OK;
	} else
		return STATE_ERROR;
}


//------------------------------------------------------------------------------------------------
// Function Name :	EZMacPRO_Wake_Up
//		Switches the MAC from SLEEP mode into IDLE mode. Turns on the crystal oscillator
//		of the radio, so the current consumption increases.The MAC has to be in SLEEP mode
//		when calling the function.
// Return Values : 	MAC_OK: the operation was successful.
//		STATE_ERROR: the operation is ignored because the MAC was not in SLEEP mode.
//------------------------------------------------------------------------------------------------
MacParams EZMacPRO_Wake_Up(void)
{
	if (EZMacProReg.name.MSR == EZMAC_PRO_SLEEP) {
		DISABLE_MAC_INTERRUPTS();        // disable MAC interrupts, just in case

		// disable Si443x interrupt 2 sources except ENLBDI & SI4432_ENWUT
		macSpiWriteReg(SI4432_INTERRUPT_ENABLE_1, 0x00);
		// enable chip ready interrupt
		macSetEnable2(SI4432_ENCHIPRDY);

		// clear interrupts
		macSpiReadReg(SI4432_INTERRUPT_STATUS_1);
		macSpiReadReg(SI4432_INTERRUPT_STATUS_2);
		CLEAR_MAC_EXT_INTERRUPT();

		// next state is WAKE UP state
		set_ezmacpro_msr(EZMAC_PRO_WAKE_UP);

		// start Si443x XTAL
		macSetFunction1(SI4432_XTON);
		// start timer with XTAL startup timeout
		macTimeout (TIMEOUT_XTAL_START);

		//Call the wake-up state entered callback.
		EZMacPRO_StateWakeUpEntered();

		ENABLE_MAC_INTERRUPTS();

		//wait until the MAC goes to Idle State
		while (EZMacProReg.name.MSR != EZMAC_PRO_IDLE) {
			/* 唤醒超时后, EZMacProReg.name.MSR将会处于WAKE_UP_ERROR */
			if (WAKE_UP_ERROR==EZMacProReg.name.MSR || WAKE_UP_ERROR_NOT_TIMEOUT==EZMacProReg.name.MSR) {
				ezmacprof_log(("%s(), line:%d, wakeup error(0x%x)\n", __FUNCTION__, __LINE__, EZMacProReg.name.MSR));
				return WAKEUP_ERROR; /* mark by David */
			}
		}

		return MAC_OK;
	} else
		return STATE_ERROR;
}


//------------------------------------------------------------------------------------------------
// Function Name:	EZMacPRO_Idle()
//		This is the only function that aborts the ongoing reception and transmission.
//		It could be used to reset the state of the MAC. Calling this function,
//		EZMAC PRO goes into IDLE state.The function cannot be called in SLEEP mode.
//
// Return Values: 	MAC_OK: the MAC is set into IDLE mode, and no transmission or reception was aborted.
//		STATE_ERROR: the MAC is set into IDLE mode, and transmission or reception was aborted
//------------------------------------------------------------------------------------------------
MacParams EZMacPRO_Idle(void)
{
	//if the MAC in sleep state
	if (EZMacProReg.name.MSR == EZMAC_PRO_SLEEP) {
		return STATE_ERROR;
	}

	DISABLE_MAC_INTERRUPTS();        // clear EX0 & ET0

	// disable all Si443x interrupt sources
	macSpiWriteReg(SI4432_INTERRUPT_ENABLE_1, 0x00);
	macSetEnable2(0x00);

	// clear interrupts
	macSpiReadReg(SI4432_INTERRUPT_STATUS_1);
	macSpiReadReg(SI4432_INTERRUPT_STATUS_2);

	STOP_MAC_TIMER();                // stop Timer
	CLEAR_MAC_TIMER_INTERRUPT();     // clear flag
	CLEAR_MAC_EXT_INTERRUPT();
#ifndef B1_ONLY
	if (EZMacProReg.name.DTR == 0x00) //if the rev V2 chip is used
		// this register setting is need for good current consumption in Idle mode (only rev V2)
		macSpiWriteReg (SI4432_CRYSTAL_OSCILLATOR_CONTROL_TEST, SI4432_BUFOVR);
#endif//B1_ONLY
	// shut-down radio except for xtal
	macSetFunction1(SI4432_XTON);
	macSpiWriteReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_2, 0x00 );

	if (EZMacProReg.name.MSR & (TX_STATE_BIT|RX_STATE_BIT)) {
		//if the ongoing reception and transmission aborts
		// the next state is IDLE state
		set_ezmacpro_msr(EZMAC_PRO_IDLE);
		//Call the Idle state entered callback function.
		EZMacPRO_StateIdleEntered();
		return STATE_ERROR;
	} else {
		// the next state is IDLE state
		set_ezmacpro_msr(EZMAC_PRO_IDLE);
		//Call the Idle state entered callback function.
		EZMacPRO_StateIdleEntered();
		return MAC_OK;
	}
}


//------------------------------------------------------------------------------------------------
// Function Name:	EZMacPRO_Transmit()
//		It starts to transmit a packet.All the parameters have to be set before calling
//		this function.	EZMAC PRO has to be in IDLE mode when calling this function.
//
// Return Values: MAC_OK: the transmission started correctly.
//		STATE_ERROR: the operation ignored (the transmission has not been started), because the
//		EZMAC PRO was not in IDLE mode.
//------------------------------------------------------------------------------------------------
#ifndef RECEIVER_ONLY_OPERATION
MacParams EZMacPRO_Transmit(void)
{
	U8 temp8;

	//if the MAC is not in Idle state
	if (EZMacProReg.name.MSR != EZMAC_PRO_IDLE)
		return STATE_ERROR;

	DISABLE_MAC_INTERRUPTS();        // clear EX0 & ET0
#ifndef B1_ONLY
	if(EZMacProReg.name.DTR == 0) { //if the rev V2 chip is used
		//this register setting is needed (only rev V2)
		macSpiWriteReg (SI4432_CRYSTAL_OSCILLATOR_CONTROL_TEST, 0x24);
		//set the TX deviation (only rev V2)
		macSpiWriteReg (SI4432_FREQUENCY_DEVIATION, TX_Freq_dev);
	}
#endif//B1_ONLY
#ifdef ANTENNA_DIVERSITY_ENABLED
#ifndef B1_ONLY
	//if revision V2 or A0 chip is used
	if ((EZMacProReg.name.DTR == 0x00) || (EZMacProReg.name.DTR == 0x01)) {

		//switch OFF the internal algorithm
		temp8 = macSpiReadReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_2);
		macSpiWriteReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_2, temp8 & 0x7F);

		//select the TX antenna
		if ( Selected_Antenna == 1 ) {
			//select antenna 1
			macSpiWriteReg(SI4432_GPIO1_CONFIGURATION, 0x1D);
			macSpiWriteReg(SI4432_GPIO2_CONFIGURATION, 0x1F);
		} else {
			//select antenna 2
			macSpiWriteReg(SI4432_GPIO1_CONFIGURATION, 0x1F);
			macSpiWriteReg(SI4432_GPIO2_CONFIGURATION, 0x1D);
		}
	}
#endif//B1_ONLY
#endif//ANTENNA_DIVERSITY_ENABLED

	//select the TX frequency
#ifdef FOUR_CHANNEL_IS_USED
	//if AFCH bit is cleared in the Transmit Control Register
	if( (EZMacProReg.name.TCR & 0x04) == 0x04) {
		//select the first frequency channel according to Frequency Register 0
		macSpiWriteReg (SI4432_FREQUENCY_HOPPING_CHANNEL_SELECT,EZMacProReg.name.FR0);
		EZMacProCurrentChannel = 0;
	} else {
		//select the proper frequency register according to FSR register
		temp8 = EZMacProReg.name.FSR;
		//in case of four channel is only for channel is allowed
		if(temp8>3) temp8 = 0;
		macSpiWriteReg (SI4432_FREQUENCY_HOPPING_CHANNEL_SELECT,EZMacProReg.array[FR0+temp8]);
		EZMacProCurrentChannel = temp8;
	}
#endif//FOUR_CHANNEL_IS_USED
#ifdef MORE_CHANNEL_IS_USED
	//select the proper frequency register according to FSR register
	temp8 = EZMacProReg.name.FSR;
	macSpiWriteReg (SI4432_FREQUENCY_HOPPING_CHANNEL_SELECT,EZMacProReg.array[FR0+temp8]);
	EZMacProCurrentChannel = temp8;
#endif//MORE_CHANNEL_IS_USED

#ifdef EXTENDED_PACKET_FORMAT
	//Assemble the CTRL byte
	//set radius field
	temp8 = (EZMacProReg.name.MCR >>3) & 0x03;
	if ( (EZMacProReg.name.TCR & 0x80) == 0x80 )
		temp8 |= 0x04;           //ACK request
	//set the Sequence number
	if ( EZMacProSequenceNumber < 15)
		EZMacProSequenceNumber++;
	else
		EZMacProSequenceNumber = 0;

	temp8 |= (EZMacProSequenceNumber<<4);

	// set the transmit headers
	if (( EZMacProReg.name.MCR & 0x80 ) == 0x80) { //if CID is used
		macSpiWriteReg(SI4432_TRANSMIT_HEADER_3, temp8); // CTRL
		macSpiWriteReg(SI4432_TRANSMIT_HEADER_2, EZMacProReg.name.SCID);
		macSpiWriteReg(SI4432_TRANSMIT_HEADER_1, EZMacProReg.name.SFID);
		macSpiWriteReg(SI4432_TRANSMIT_HEADER_0, EZMacProReg.name.DID);
	} else {
		//if CID is not used
		macSpiWriteReg(SI4432_TRANSMIT_HEADER_3, temp8); // CTRL
		macSpiWriteReg(SI4432_TRANSMIT_HEADER_2, EZMacProReg.name.SFID);
		macSpiWriteReg(SI4432_TRANSMIT_HEADER_1, EZMacProReg.name.DID);
	}
#endif   // EXTENDED_PACKET_FORMAT

#ifdef TRANSCEIVER_OPERATION
	if(((EZMacProReg.name.TCR & 0x08)==0x08) && ((EZMacProReg.name.TCR & 0x04)==0x00)) { // LBT enabled and the AFCH is not enabled
		//Set Listen Before Talk Limit to RSSI threshold register
		macSpiWriteReg(SI4432_RSSI_THRESHOLD, EZMacProReg.name.LBTLR);

		// clear interrupts
		macSpiReadReg(SI4432_INTERRUPT_STATUS_1);
		macSpiReadReg(SI4432_INTERRUPT_STATUS_2);


		// disable all Si443x interrupt enable 1 sources
		macSpiWriteReg(SI4432_INTERRUPT_ENABLE_1, 0x00);
		// enable RSSI interrupt
		macSetEnable2(SI4432_ENRSSI);
		//clear the LBT variable
		EZMacProLBT_Retrys = 0;
		BusyLBT = 0;
		// go to next state
		set_ezmacpro_msr(TX_STATE_BIT | TX_STATE_LBT_START_LISTEN);
		//Call the TX state entered callback.
		EZMacPRO_StateTxEntered();

		// enable RX
		macSetFunction1 (SI4432_RXON|SI4432_XTON);
		//first timeout is fix 0.5ms
		// start timer with LBT ETSI fix timeout
		macTimeout(TIMEOUT_LBTI_ETSI);

		ENABLE_MAC_INTERRUPTS();
		return MAC_OK;
	}
#endif //TRANSCEIVER_OPERATION
	// go straight to transmit without LBT
	// enable ENPKSENT bit
	macSpiWriteReg (SI4432_INTERRUPT_ENABLE_1, SI4432_ENPKSENT);
	// disable enable 2 using function
	macSetEnable2 (0x00);
	// clear interrupt status
	macSpiReadReg(SI4432_INTERRUPT_STATUS_1);
	macSpiReadReg(SI4432_INTERRUPT_STATUS_2);

	set_ezmacpro_msr(TX_STATE_BIT | TX_STATE_WAIT_FOR_TX); /* mark by David */
//	ezmacprof_debug(("----mac msr:0x%02x-->0x%02x, timeout:%d\n",
//			EZMacProReg.name.MSR, TX_STATE_BIT | TX_STATE_WAIT_FOR_TX, TimeoutTX_Packet));

	// enable TX
	macSetFunction1 (SI4432_TXON|SI4432_XTON);

	// start timer with packet transmit timeout
	macTimeout(TimeoutTX_Packet);

	//Call the TX state entered callback.
	EZMacPRO_StateTxEntered();
	ENABLE_MAC_INTERRUPTS();

	return MAC_OK;
}
#endif // RECEIVER_ONLY_OPERATION not defined


//------------------------------------------------------------------------------------------------
// Function Name : EZMacPRO_Receive()
//	 It starts searching for a new packet on the defined frequencies. If the receiver finds
//	 RF activity on a channel, it tries to receive and process the packet.
//	 The search can be stopped by the EZMacPRO_Idle() function.
//	 All the parameters have to be set before calling this function.
//	 EZMAC PRO has to be in IDLE mode when calling this function.
//
// Return Values : MAC_OK: the search mechanism started correctly.
//	 STATE_ERROR: the operation is ignored (the search mechanism has not been started) because the
//	 EZMAC PRO was not in IDLE mode.
//-----------------------------------------------------------------------------------------------
#ifndef TRANSMITTER_ONLY_OPERATION
MacParams EZMacPRO_Receive(void)
{
	U8 temp8;

	if (EZMacProReg.name.MSR != EZMAC_PRO_IDLE)
		return STATE_ERROR;
	else {
		DISABLE_MAC_INTERRUPTS();        // just in case
#ifndef B1_ONLY
		if(EZMacProReg.name.DTR == 0) { //if the revV2 chip is used
			//this register setting is needed (only rev V2)
			macSpiWriteReg (SI4432_CRYSTAL_OSCILLATOR_CONTROL_TEST, 0x24);
			//set the RX deviation (only rev V2)
			macSpiWriteReg (SI4432_FREQUENCY_DEVIATION, RX_Freq_dev);
		}
#endif//B1_ONLY

#ifdef FOUR_CHANNEL_IS_USED
		//Frequency search mechanism is disabled
		if((EZMacProReg.name.RCR & 0x04)== 0x00) {
			//select the proper frequency register according to FSR register
			temp8 = EZMacProReg.name.FSR;
			if(temp8>3) temp8 = 0;
			macSpiWriteReg (SI4432_FREQUENCY_HOPPING_CHANNEL_SELECT,EZMacProReg.array[FR0+temp8]);
			EZMacProCurrentChannel = temp8;
		}
		//Frequency search mechanism is enabled
		else {
			//select the first not masked frequency
			if ((EZMacProReg.name.RCR & 0x08) != 0x08)
				macSpiWriteReg (SI4432_FREQUENCY_HOPPING_CHANNEL_SELECT, EZMacProReg.name.FR0);
			else if ((EZMacProReg.name.RCR & 0x10) != 0x10)
				macSpiWriteReg (SI4432_FREQUENCY_HOPPING_CHANNEL_SELECT, EZMacProReg.name.FR1);
			else if ((EZMacProReg.name.RCR & 0x20) != 0x20)
				macSpiWriteReg (SI4432_FREQUENCY_HOPPING_CHANNEL_SELECT, EZMacProReg.name.FR2);
			else if ((EZMacProReg.name.RCR & 0x40) != 0x40)
				macSpiWriteReg (SI4432_FREQUENCY_HOPPING_CHANNEL_SELECT, EZMacProReg.name.FR3);
		}
#endif //FOUR_CHANNEL_IS_USED

		//disable Interrupt Enable 1
		macSpiWriteReg (SI4432_INTERRUPT_ENABLE_1, 0x00);

		//enable preamble valid interrupt
		macSetEnable2(SI4432_ENPREAVAL);

#ifdef MORE_CHANNEL_IS_USED
		SelectedChannel = 0;
		macSpiWriteReg(SI4432_FREQUENCY_HOPPING_CHANNEL_SELECT,EZMacProReg.array[FR0+SelectedChannel]);
#endif //MORE_CHANNEL_IS_USED

		// clear interrupt status
		macSpiReadReg(SI4432_INTERRUPT_STATUS_1);
		macSpiReadReg(SI4432_INTERRUPT_STATUS_2);

		// clear RX FIFO
		temp8 = macSpiReadReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_2);
		temp8 |= SI4432_FFCLRRX;
		macSpiWriteReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_2, temp8);
		temp8 &= ~SI4432_FFCLRRX;
		macSpiWriteReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_2, temp8);

		// go to next state
		set_ezmacpro_msr(RX_STATE_BIT | RX_STATE_FREQUENCY_SEARCH);

		// enable RX
		macSetFunction1(SI4432_RXON|SI4432_XTON);
		// start timer with channel search timeout
		macTimeout(TimeoutChannelSearch);

		//Call the RX state entered callback.
		EZMacPRO_StateRxEntered();
		ENABLE_MAC_INTERRUPTS();

		return MAC_OK;

	}
}
#endif // TRANSMITTER_ONLY_OPERATION


//------------------------------------------------------------------------------------------------
// Function Name:    EZMacPRO_Reg_Write
//	Writes the value into the register identified by name. MacRegs type is predefined;
//	the names of the available registers are listed in the Register Assignment section.
//	When required, this function also updates the radio register setting directly.
//	This function is also available to be called during SLEEP mode.
//	None of the registers can be set during packet transmission or reception!
//
// Return Values:	MAC_OK: The register is set correctly.
//	NAME_ERROR: The register name is unknown.
//	VALUE_ERROR: The value is out of the range: for this register.
//	STATE_ERROR: The requested operation is currently unavailable and ignored because either
//			transmission or reception is currently in progress.
//	INCONSISTENT_SETTING: If the data rate is changed and the current Frequency ID
//			is not supported by the new data rate, the value of the
//			inconsistent Frequency ID is automatically changed to a 0.
//			In this case, the function returns with 'INCONSISTENT_SETTING'.
//
// Parameters:  name: MAC register name
//             	value: MAC register value
//-----------------------------------------------------------------------------------------------
MacParams EZMacPRO_Reg_Write(MacRegs  name, U8 value)
{
	U8 temp8;
	U8 temp8_2;

	// register name check
	if (name>EZ_LASTREG)
		return NAME_ERROR;

	// state check
	if (EZMacProReg.name.MSR & (TX_STATE_BIT|RX_STATE_BIT))
		return STATE_ERROR;

	switch (name) {
		// order must match enumerations, mandatory elements listed first
	case    MCR:                          // Master Control Register
		//Set the header length according to CID control bit
#ifdef EXTENDED_PACKET_FORMAT
		if (( value & 0x80 ) == 0x80)//if CID is used
			//header length 4 byte(CTRL+CID+SID+DID)
			macSpiWriteReg(SI4432_HEADER_CONTROL_2, SI4432_HDLEN_4BYTE | (SYNC_WORD_LENGTH - 1) << 1);
		else
			//header length 3 byte(CTRL+SID+DID)
			macSpiWriteReg(SI4432_HEADER_CONTROL_2, SI4432_HDLEN_3BYTE | (SYNC_WORD_LENGTH - 1 ) << 1);
#else //STANDARD_PACKET_FORMAT
		if (( value & 0x80 ) == 0x80)//if CID is used
			//header length 3 byte(CID+SID+DID)
			macSpiWriteReg(SI4432_HEADER_CONTROL_2, SI4432_HDLEN_3BYTE | (SYNC_WORD_LENGTH - 1) << 1);
		else
			//header length 2 byte(SID+DID)
			macSpiWriteReg(SI4432_HEADER_CONTROL_2, SI4432_HDLEN_2BYTE | (SYNC_WORD_LENGTH - 1) << 1);
#endif//EXTENDED_PACKET_FORMAT

		//Master Control Register(set modem, frequency parameters and preamble length according to the data rate)
		SetRfParameters( value );
		macUpdateDynamicTimeouts(value, EZMacProReg.name.MPL);

#ifdef FOUR_CHANNEL_IS_USED
		//in case of four channel, check the frequency number is correct or not
		temp8 = (value & 0x60) >> 5;
		if ( EZMacProReg.name.FR0 >= Parameters[temp8][MAX_CHANNEL_NUMBER]) {
			EZMacProReg.name.FR0 = 0;
			return INCONSISTENT_SETTING;
		}
		if ( EZMacProReg.name.FR1 >= Parameters[temp8][MAX_CHANNEL_NUMBER]) {
			EZMacProReg.name.FR1 = 0;
			return INCONSISTENT_SETTING;
		}
		if ( EZMacProReg.name.FR2 >= Parameters[temp8][MAX_CHANNEL_NUMBER]) {
			EZMacProReg.name.FR2 = 0;
			return INCONSISTENT_SETTING;
		}

		if ( EZMacProReg.name.FR3 >= Parameters[temp8][MAX_CHANNEL_NUMBER]) {
			EZMacProReg.name.FR3 = 0;
			return INCONSISTENT_SETTING;
		}
#endif //FOUR_CHANNEL_IS_USED
#ifdef TRANSCEIVER_OPERATION
		macUpdateLBTI(EZMacProReg.name.LBTIR);
#endif //TRANSCEIVER_OPERATION
		//static packet length used
		if (( value & 0x04 ) == 0x00) {
			temp8 = macSpiReadReg(SI4432_HEADER_CONTROL_2);
			macSpiWriteReg(SI4432_HEADER_CONTROL_2, temp8|SI4432_FIXPKLEN);
		}

		break;

	case    SECR:                         // State & Error Counter Control Register
		break;

	case    TCR:                          // Transmit Control Register
		//Transmit Control Register(set output power of the radio
		temp8 = value >> 4;
#ifndef B1_ONLY
		if(EZMacProReg.name.DTR == 0) //check the device typ
			macSpiWriteReg(SI4432_TX_POWER,temp8); // revV2
		else if(EZMacProReg.name.DTR == 1)
			macSpiWriteReg(SI4432_TX_POWER,(temp8|0x08)); //revA0
		else if(EZMacProReg.name.DTR == 2)
			macSpiWriteReg(SI4432_TX_POWER,(temp8|0x18)); //revB1
#endif//B1_ONLY
#ifdef B1_ONLY
		/* SI4432_TX_POWER.bit3 LNA Switch Controller, SI4432_TX_POWER.bit[7:4] reserved bits, mark by David */
		macSpiWriteReg(SI4432_TX_POWER,(temp8|0x18)); //revB1
#endif//B1_ONLY
		break;

	case    RCR:                          // Receiver Control Register
		//Receive Control Register
		if ((value & 0x78) == 0x78)
			return VALUE_ERROR;
		break;

	case    FR0:                          // Frequency Register 0
		//break;
	case    FR1:                          // Frequency Register 1
		//break;
	case    FR2:                          // Frequency Register 2
		//break;
	case    FR3:                          // Frequency Register 3
		//break;
#ifdef FOUR_CHANNEL_IS_USED
		//Frequency registers(check the setting value is correct)
		// /get the setting data rate
		temp8 = (EZMacProReg.name.MCR>>5)&0x03;
		//check the enabled channel number
		if (value >=  Parameters[temp8][MAX_CHANNEL_NUMBER])
			return VALUE_ERROR;
		break;
#endif //FOUR_CHANNEL_IS_USED

#ifdef MORE_CHANNEL_IS_USED
	case    FR4:                          // Frequency Register 4
		//break;
	case    FR5:                          // Frequency Register 5
		//break;
	case    FR6:                          // Frequency Register 6
		//break;
	case    FR7:                          // Frequency Register 7
		//break;
	case    FR8:                          // Frequency Register 8
		//break;
	case    FR9:                          // Frequency Register 9
		//break;
	case    FR10:                         // Frequency Register 10
		//break;
	case    FR11:                         // Frequency Register 11
		//break;
	case    FR12:                         // Frequency Register 12
		//break;
	case    FR13:                          // Frequency Register 13
		//break;
	case    FR14:                          // Frequency Register 14
		//break;
	case    FR15:                          // Frequency Register 15
		//break;
	case    FR16:                          // Frequency Register 16
		//break;
	case    FR17:                          // Frequency Register 17
		//break;
	case    FR18:                          // Frequency Register 18
		//break;
	case    FR19:                          // Frequency Register 19
		//break;
	case    FR20:                          // Frequency Register 20
		//break;
	case    FR21:                          // Frequency Register 21
		//break;
	case    FR22:                          // Frequency Register 22
		//break;
	case    FR23:                          // Frequency Register 23
		//break;
	case    FR24:                          // Frequency Register 24
		//break;
	case    FR25:                          // Frequency Register 25
		//break;
	case    FR26:                          // Frequency Register 26
		//break;
	case    FR27:                          // Frequency Register 27
		//break;
	case    FR28:                          // Frequency Register 28
		//break;
	case    FR29:                          // Frequency Register 29
		//break;
	case    FR30:                          // Frequency Register 30
		//break;
	case    FR31:                          // Frequency Register 31
		//break;
	case    FR32:                          // Frequency Register 32
		//break;
	case    FR33:                          // Frequency Register 33
		//break;
	case    FR34:                          // Frequency Register 34
		//break;
	case    FR35:                          // Frequency Register 35
		//break;
	case    FR36:                          // Frequency Register 36
		//break;
	case    FR37:                          // Frequency Register 37
		//break;
	case    FR38:                          // Frequency Register 38
		//break;
	case    FR39:                          // Frequency Register 39
		//break;
	case    FR40:                          // Frequency Register 40
		//break;
	case    FR41:                          // Frequency Register 41
		//break;
	case    FR42:                          // Frequency Register 42
		//break;
	case    FR43:                          // Frequency Register 43
		//break;
	case    FR44:                          // Frequency Register 44
		//break;
	case    FR45:                          // Frequency Register 45
		//break;
	case    FR46:                          // Frequency Register 46
		//break;
	case    FR47:                          // Frequency Register 47
		//break;
	case	  FR48:									// Frequency Register 48
		//break;
	case	  FR49:									// Frequency Register 49
		break;
#endif //MORE_CHANNEL_IS_USED
	case 	  FSR:									// Frequency Select Register
#ifdef FOUR_CHANNEL_IS_USED
		if(value >= 4)
			return VALUE_ERROR;
#endif
#ifdef MORE_CHANNEL_IS_USED
		temp8 = (EZMacProReg.name.MCR >> 5) & 0x03;
		if(value >= Parameters[temp8] [5])
			return VALUE_ERROR;
#endif
		break;

#ifdef FOUR_CHANNEL_IS_USED
	case    EC0:                          // Error Counter of Frequency 0
		break;
	case    EC1:                          // Error Counter of Frequency 1
		break;
	case    EC2:                          // Error Counter of Frequency 2
		break;
	case    EC3:                          // Error Counter of Frequency 3
		break;
#endif //FOUR_CHANNEL_IS_USED

	case    PFCR:                         // Packet Filter Control Register
		break;

	case    SFLT:                         // Sender ID Filter
		break;

	case    SMSK:                         // Sender ID Filter Mask
		break;

	case    MCA_MCM:                      // Multicast Address / Multicast Mask
		break;

	case    MPL:                          // Maximum Packet Length
		if ( value > RECEIVED_BUFFER_SIZE )
			return VALUE_ERROR;
		else
			macUpdateDynamicTimeouts(EZMacProReg.name.MCR, value);          // max payload always updates timouts
		break;

	case    MSR:                          // MAC Status Register
		//MAC Status Register is read only
		return NAME_ERROR;
		break;

	case    RSR:                          // Receive Status Register
		//Receive Status Register is read only
		return NAME_ERROR;
		break;

	case	  RFSR:								// Received Frequency Status Register
		//Received Frequency Status Register is read only
		return NAME_ERROR;
		break;

	case    RSSI:                         // Received Signal Strength Indicator
		//Received Signal Strength Indicator is read  only
		return NAME_ERROR;
		break;

	case    SCID :                        // Self Customer ID
		if (( EZMacProReg.name.MCR & 0x80 ) == 0x80)//if CID is used
			//save the SCID to the Transmit Header3 register of the radio
			macSpiWriteReg(SI4432_TRANSMIT_HEADER_3, value);
		else
			return NAME_ERROR;
		break;

	case    SFID:                         // Self ID
		if (value == 255)
			return VALUE_ERROR;
		if (( EZMacProReg.name.MCR & 0x80 ) == 0x80)//if CID is used
			//save the SFID to the Transmit Header2 register of the radio
			macSpiWriteReg(SI4432_TRANSMIT_HEADER_2, value);
		else
			//save the SFID to the Transmit Header3 register of the radio
			macSpiWriteReg(SI4432_TRANSMIT_HEADER_3, value);
		break;

	case    RCTRL:                        // Received Control Byte
		//Received CTRL read only
		return NAME_ERROR;
		break;

	case    RCID:                         // Received Customer ID
		//Received Customer ID is read only
		return NAME_ERROR;
		break;

	case    RSID:                         // Received Sender ID
		//Received Sender ID is read only
		return NAME_ERROR;
		break;

	case    DID:                          // Destination ID
		if (( EZMacProReg.name.MCR & 0x80 ) == 0x80)//if CID is used
			//save the DID to the Transmit Header1 register of the radio
			macSpiWriteReg(SI4432_TRANSMIT_HEADER_1, value);
		else
			//save the DID to the Transmit Header2 register of the radio
			macSpiWriteReg(SI4432_TRANSMIT_HEADER_2, value);
		break;

	case    PLEN:                         // Payload Length
		//set the payload length
		if ( value > RECEIVED_BUFFER_SIZE )
			return VALUE_ERROR;
		else {
			macSpiWriteReg(SI4432_TRANSMIT_PACKET_LENGTH,value);   //save the PLEN to the transmit packet length register
		}
		break;

		// optional elemets listed last using compiler switches
#ifdef TRANSCEIVER_OPERATION
	case    LBTIR:                        // Listen Before Talk  Interval Register
		macUpdateLBTI(value);
		break;
	case    LBTLR:                        // Listen Before Talk Limit Register
		break;
#endif //TRANSCEIVER_OPERATION
	case    LFTMR0:                       // Low Frequency Timer Setting Register 0
		//Low Frequency Timer Setting Register0
		macSpiWriteReg(SI4432_WAKE_UP_TIMER_PERIOD_3, value); //Set Wake-Up-Timer Mantissa
		break;

	case    LFTMR1:                       // Low Frequency Timer Setting Register 1
		//Low Frequency Timer Setting Register1
		macSpiWriteReg(SI4432_WAKE_UP_TIMER_PERIOD_2, value); //Set Wake-Up_Timer Mantissa
		break;

	case LFTMR2: // Low Frequency Timer Setting Register 2
		//Set Wake-Up_timer Exponent
		macSpiWriteReg(SI4432_WAKE_UP_TIMER_PERIOD_1, value & 0x3F);
		temp8 = macSpiReadReg(SI4432_INTERRUPT_ENABLE_2);
		temp8_2 = temp8 = macSpiReadReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1);
		if(value & 0x80) {  //Wake-Up-Timer enabled
			temp8 |= SI4432_ENWUT;
			temp8_2 |= SI4432_ENWT;
		} else {            //Wake-Up-Timer disabled
			temp8 &= ~SI4432_ENWUT;
			temp8_2 &= ~SI4432_ENWT;
		}
		macSpiWriteReg(SI4432_INTERRUPT_ENABLE_2, temp8);
		macSpiWriteReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1, temp8_2);

		temp8 = macSpiReadReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1);
		if(value & 0x40) {  //Internal 32KHz RC Oscillator
			temp8 &= ~SI4432_X32KSEL;
		} else {            //External 32KHz Oscillator
			temp8 |= SI4432_X32KSEL;
		}
		macSpiWriteReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1, temp8);
		ENABLE_MAC_EXT_INTERRUPT();
		break;

	case    LBDR:                         // Low Battery Detect Register
		if ((value & 0x80) == 0x80) { //if enalbe the low battery detect
			temp8 = value & 0x1F;
			macSpiWriteReg(SI4432_LOW_BATTERY_DETECTOR_THRESHOLD, temp8);     //set battery voltage threshold

			temp8 = macSpiReadReg(SI4432_INTERRUPT_ENABLE_2);
			macSpiWriteReg(SI4432_INTERRUPT_ENABLE_2, temp8 | SI4432_ENLBDI);    //enable the low battery interrupt

			temp8 = macSpiReadReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1);
			macSpiWriteReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1, temp8|SI4432_ENLBD);
		} else {
			temp8 = macSpiReadReg(SI4432_INTERRUPT_ENABLE_2);
			macSpiWriteReg(SI4432_INTERRUPT_ENABLE_2, temp8 & ~SI4432_ENLBDI);    //disable the low battery interrupt

			temp8 = macSpiReadReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1);
			macSpiWriteReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1, temp8 & ~SI4432_ENLBD); //disable the low battery detect
		}
		break;

	case    ADCTSR:                       // ADC and Temperature Sensor Register
		//set ADC configuration register
		macSpiWriteReg(SI4432_ADC_CONFIGURATION, (value & 0xFC) | EZMACPRO_ADC_GAIN);

		//set ADC sensor Amplifier register(EZMACPRO_ADC_AMP_OFFSET is definition in EZMacPro_defs.h)
		macSpiWriteReg(SI4432_ADC_SENSOR_AMPLIFIER_OFFSET, EZMACPRO_ADC_AMP_OFFSET);
		//set the Temperature Sensor range
		macSpiWriteReg(SI4432_TEMPERATURE_SENSOR_CONTROL, ((value & 0x03)<<6)|0x20);
		break;

	case    ADCTSV:                       // ADC/Temperature Value Register
		return NAME_ERROR;
		break;

	case	  DTR:
#ifndef B1_ONLY
		if (value > 2)
			return VALUE_ERROR;
		macSpecialRegisterSettings(value);
#endif//B1_ONLY
		break;

	}

	//Register update
	EZMacProReg.array[name] = value;

	return MAC_OK;
}


//------------------------------------------------------------------------------------------------
// Function Name:    EZMacPRO_Reg_Read
// 		Gives back the value (over value pointer) of the register identified by name.
//		MacRegs type is predefined. This function may also be called in SLEEP mode.
// Return Value :    MAC_OK: The operation was succesfull
//             	NAME_ERROR: The register name is invalid
//
// Parameters   :    name: register name
//             	value: register value
//-----------------------------------------------------------------------------------------------
MacParams EZMacPRO_Reg_Read (MacRegs name, U8 *value)
{

	// register name check
	if (name>EZ_LASTREG)
		return NAME_ERROR;

	//this 3 registers are write only
	if ((name == LFTMR0) || (name == LFTMR1) ||(name == LFTMR2)) {
		return NAME_ERROR;
	}

	if (name == RSSI)
		// state check
		if ((EZMacProReg.name.MSR & RX_STATE_BIT)==RX_STATE_BIT)
			//if the MAC in these state then the RSSI will be read from the chip
			EZMacProReg.name.RSSI = macSpiReadReg(SI4432_RECEIVED_SIGNAL_STRENGTH_INDICATOR);


	if (name == ADCTSV) {
		EZMacProReg.name.ADCTSV = macSpiReadReg(SI4432_ADC_VALUE);
	}

	if (name == LBDR) {
		if ((EZMacProReg.name.LBDR & 0x80) == 0x80) {
			EZMacProReg.name.LBDR &= 0x80;
			EZMacProReg.name.LBDR |= macSpiReadReg(SI4432_BATTERY_VOLTAGE_LEVEL);

		} else {
			EZMacProReg.name.LBDR = 0x00;
		}
	}

	// gives back the register content
	*value = EZMacProReg.array[name];

	return MAC_OK;
}


//------------------------------------------------------------------------------------------------
// Function Name: EZMacPRO_TxBuf_Write
//	The function copies length number of payload bytes into the transmit FIFO of the radio chip.
//	There is no dedicated transmit buffer in the source code.Upon calling this function,
//	it clears the TX FIFO of the radio first.If variable packet length is used and
//	the length is not greater than the RECEIVED_BUFFER_SIZE definition
//	(it cannot be greater than 64), then EZMAC PRO copies length number of payload
//	bytes into the TX FIFO of the radio, sets the PLEN register of EZMAC PRO,
//	and also sets the packet length register of the radio. If fix packet length is used,
//	then the PLEN register has to set first, because the function copies only
//	Payload Length number of bytes into the FIFO even if the length is greater.
//	Also it fills the FIFO with extra 0x00 bytes if the length is smaller than
//	the value of the Payload Length register in fix packet length mode.
//	The function cannot be called during transmission and reception.
//
// Return Values:   MAC_OK: The operation performed correctly.
//                  STATE_ERROR: The operation is ignored because transmission and reception are in progress.
//
// Parameters:      length: payload length
//                  payload: payload content
//
//-----------------------------------------------------------------------------------------------
#ifndef RECEIVER_ONLY_OPERATION
MacParams EZMacPRO_TxBuf_Write(ubase_t length, VARIABLE_SEGMENT_POINTER(payload, U8, BUFFER_MSPACE))
{
	U8 temp8;

	// state check
	if (EZMacProReg.name.MSR & (TX_STATE_BIT|RX_STATE_BIT))
		return STATE_ERROR;

	// if the given packet is bigger then the RECEIVED_BUFFER_SIZE
	if ( length > RECEIVED_BUFFER_SIZE )
		return VALUE_ERROR;

	// clear TX FIFO
	temp8 = macSpiReadReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_2);
	temp8 |= SI4432_FFCLRTX;
	macSpiWriteReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_2, temp8);
	temp8 &= ~SI4432_FFCLRTX;
	macSpiWriteReg (SI4432_OPERATING_AND_FUNCTION_CONTROL_2, temp8);


	//if dynamic payload length mode is set
	if ((EZMacProReg.name.MCR & 0x04) == 0x04) {

		//set the transmit packet length
		macSpiWriteReg(SI4432_TRANSMIT_PACKET_LENGTH, length);
		EZMacProReg.name.PLEN = length;
		//set the payload content
		macSpiWriteFIFO(length,payload);
	}
	//if static payload length mode is set
	else {

		if (length < EZMacProReg.name.PLEN) { //if payload length smaller than the fix payload length
			//set the payload content
			macSpiWriteFIFO(length,payload);

			//fill the remain payload bytes with zero
			for(temp8=length; temp8 < EZMacProReg.name.PLEN; temp8++)
				macSpiWriteReg(SI4432_FIFO_ACCESS,0x00);

		} else { //if the payload length equal or bigger than the fix payload length
			macSpiWriteFIFO(EZMacProReg.name.PLEN,payload);
		}


	}
	return MAC_OK;
}
#endif //RECEIVER_ONLY_OPERATION not defined


//------------------------------------------------------------------------------------------------
// Function Name: EZMacPRO_RxBuf_Read
//      After a successful packet reception EZMAC PRO copies the received data bytes into
//      the receive data buffer. The receive data buffer is declared in the EZMacPro.c file as
//      SEGMENT_VARIABLE(RxBuffer[RECEIVED_BUFFER_SIZE], U8, BUFFER_MSPACE);
//      The length of the receive buffer is defined by the RECEIVED_BUFFER_SIZE definition in the
//      EZMacPro_defs.h. It can be adjusted for the application needs, but it can not be greater than 64bytes.
//      The receive buffer is declared to be placed into the XDATA memory, it also can be adjusted by
//      changing the BUFFER_MSPACE definition. Upon calling the EZMacPRO_RxBuf_Read() function, it
//      copies received data from the receive data buffer to payload. Also it gives back the number of
//      received bytes by length.
//      The function cannot be called during transmission and reception.
//
// Return Values:   MAC_OK: The operation was succesfull
//                  STATE_ERROR: The operation is ignored, because reception or transmission is ongoing.
//
// Parameters:      length: received payload length
//                  payload: received payload content
//
//-----------------------------------------------------------------------------------------------
#ifndef TRANSMITTER_ONLY_OPERATION
MacParams EZMacPRO_RxBuf_Read(VARIABLE_SEGMENT_POINTER(length, ubase_t, BUFFER_MSPACE), VARIABLE_SEGMENT_POINTER(payload, U8, BUFFER_MSPACE))
{
	U8 temp8 = 0;

	*length = EZMacProReg.name.PLEN;

	while(temp8 < EZMacProReg.name.PLEN) {
		*payload++ = RxBuffer[temp8];
		temp8++;
	}
	return MAC_OK;
}
#endif // TRANSMITTER_ONLY_OPERATION not defined


//------------------------------------------------------------------------------------------------
// Function Name:   EZMacPRO_Ack_Write()
//
//                  The function copies length number of bytes into the Acknowledgement buffer.
//                  Length should not be greater than ACK_BUFFER_SIZE otherwise STATE_ERROR is returned.
//                  In dynamic packet length mode the payload size is set to length.
//                  In fixed packet length mode, if length is smaller than PLEN, then the remaining bytes
//                  are filled with zeros. Otherwise only PLEN number of bytes are copied.
//                  The function should exclusively be used in the EZMacPRO_AckSending() callback to
//                  put custom data in the AckBuffer just before its content is pushed into the transmit
//                  FIFO.
//
// Return Values:   MAC_OK: The operation performed correctly.
//                  STATE_ERROR: The operation is ignored as the statemachine is not in receive state.
//
// Parameters:      length: payload length
//             	    payload: payload content
//
//-----------------------------------------------------------------------------------------------
#ifdef TRANSCEIVER_OPERATION
#ifdef EXTENDED_PACKET_FORMAT
MacParams EZMacPRO_Ack_Write(ubase_t length, VARIABLE_SEGMENT_POINTER(payload, U8, BUFFER_MSPACE))
{
	U8 temp8 = 0;

	// state check: if not in RX state -> error
	if (!(EZMacProReg.name.MSR & RX_STATE_BIT))
		return STATE_ERROR;

	//if length is bigger than the ACK_BUFFER_SIZE
	if (length > ACK_BUFFER_SIZE)
		return VALUE_ERROR;

	//if dynamic payload length mode is set
	if ((EZMacProReg.name.MCR & 0x04) == 0x04) {
		//set the transmit packet length
		macSpiWriteReg(SI4432_TRANSMIT_PACKET_LENGTH,length);

		//load AckBuffer
		for(temp8 = 0; temp8 < length; temp8++)
			AckBuffer[temp8] = payload[temp8];
	} else { //if static payload length mode is set
		//if fix packet length is greater than ack buffer size
		if(EZMacProReg.name.PLEN > ACK_BUFFER_SIZE) {
			//load AckBuffer
			for(temp8 = 0; temp8 < length; temp8++)
				AckBuffer[temp8] = payload[temp8];
			//fill the remaining bytes with zero
			for(temp8 = length; temp8 < ACK_BUFFER_SIZE; temp8++)
				AckBuffer[temp8] = 0x00;
		} else { //if fix packet length is smaller or equal than ack buffer size
			//if payload length smaller than the fix payload length
			if (length < EZMacProReg.name.PLEN) {
				//load AckBuffer
				for(temp8 = 0; temp8 < length; temp8++)
					AckBuffer[temp8] = payload[temp8];
				//fill the remaining bytes with zero
				for(temp8 = length; temp8 < EZMacProReg.name.PLEN; temp8++)
					AckBuffer[temp8] = 0x00;
			} else { //if the payload length equal or bigger than the fix payload length
				//load AckBuffer
				for(temp8 = 0; temp8 < EZMacProReg.name.PLEN; temp8++)
					AckBuffer[temp8] = payload[temp8];
			}
		}
	}
	return MAC_OK;
}
#endif //EXTENDED_PACKET_FORMAT not defined
#endif //TRANSCEIVER_OPERATION not defined


/* ======================================= *
 *      L O C A L   F U N C T I O N S      *
 * ======================================= */

//------------------------------------------------------------------------------------------------
// Function Name: SetRfParameters
//
// Return Value : None
// Parameters   : mcr: Master Control Register value
//
// Note: Set the modem, frequency parameters and preamble according to data rate
//
//-----------------------------------------------------------------------------------------------
static void SetRfParameters(U8 mcr)
{
	U8 dataRate;
	U8 numFreq;
#ifdef MORE_CHANNEL_IS_USED
	U8 temp8;
#endif//MORE_CHANNEL_IS_USED
	dataRate = (mcr >> 5) & 0x03;
	numFreq = mcr & 0x03;
#ifndef B1_ONLY
	//set the modem parameters
	switch(EZMacProReg.name.DTR) {
	case 0://rev V2
		macSpiWriteReg(SI4432_IF_FILTER_BANDWIDTH, RfSettingsV2[dataRate][0] );
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OVERSAMPLING_RATIO, RfSettingsV2[dataRate][1]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_2, RfSettingsV2[dataRate][2]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_1, RfSettingsV2[dataRate][3]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_0, RfSettingsV2[dataRate][4]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_1, RfSettingsV2[dataRate][5]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_0, RfSettingsV2[dataRate][6]);
		macSpiWriteReg(SI4432_TX_DATA_RATE_1, RfSettingsV2[dataRate][7]);
		macSpiWriteReg(SI4432_TX_DATA_RATE_0, RfSettingsV2[dataRate][8]);
		macSpiWriteReg(SI4432_MODULATION_MODE_CONTROL_1,RfSettingsV2[dataRate][9]);
		macSpiWriteReg(SI4432_FREQUENCY_DEVIATION,RfSettingsV2[dataRate][10]);
		macSpiWriteReg(SI4432_AFC_LOOP_GEARSHIFT_OVERRIDE, RfSettingsV2[dataRate][12]);
		macSpiWriteReg(SI4432_CHARGEPUMP_CURRENT_TRIMMING_OVERRIDE, RfSettingsV2[dataRate][13]);
#ifndef RECEIVER_ONLY_OPERATION
		TX_Freq_dev = RfSettingsV2[dataRate][10];
#endif//RECEIVER_ONLY_OPERATION
#ifndef TRANSMITTER_ONLY_OPERATION
		RX_Freq_dev = RfSettingsV2[dataRate][11];
#endif//TRANSMITTER_ONLY_OPERATION
		macSpiWriteReg(SI4432_PREAMBLE_DETECTION_CONTROL, Parameters[dataRate][PREAMBLE_DETECTION_THRESHOLD]<<3);
		break;
	case 1://rev A0
		macSpiWriteReg(SI4432_IF_FILTER_BANDWIDTH, RfSettingsA0[dataRate][0] );
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OVERSAMPLING_RATIO, RfSettingsA0[dataRate][1]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_2, RfSettingsA0[dataRate][2]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_1, RfSettingsA0[dataRate][3]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_0, RfSettingsA0[dataRate][4]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_1, RfSettingsA0[dataRate][5]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_0, RfSettingsA0[dataRate][6]);
		macSpiWriteReg(SI4432_TX_DATA_RATE_1, RfSettingsA0[dataRate][7]);
		macSpiWriteReg(SI4432_TX_DATA_RATE_0, RfSettingsA0[dataRate][8]);
		macSpiWriteReg(SI4432_MODULATION_MODE_CONTROL_1,RfSettingsA0[dataRate][9]);
		macSpiWriteReg(SI4432_FREQUENCY_DEVIATION,RfSettingsA0[dataRate][10]);
		macSpiWriteReg(SI4432_AFC_LOOP_GEARSHIFT_OVERRIDE, RfSettingsA0[dataRate][12]);
		macSpiWriteReg(SI4431_AFC_LIMIT, RfSettingsA0[dataRate][13]);
		macSpiWriteReg(SI4432_PREAMBLE_DETECTION_CONTROL, (Parameters[dataRate][PREAMBLE_DETECTION_THRESHOLD]<<3)|0x02);
		break;
	case 2: //rev B1
	default:
		macSpiWriteReg(SI4432_IF_FILTER_BANDWIDTH, RfSettingsB1[dataRate][0] );
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OVERSAMPLING_RATIO, RfSettingsB1[dataRate][1]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_2, RfSettingsB1[dataRate][2]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_1, RfSettingsB1[dataRate][3]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_0, RfSettingsB1[dataRate][4]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_1, RfSettingsB1[dataRate][5]);
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_0, RfSettingsB1[dataRate][6]);
		macSpiWriteReg(SI4432_TX_DATA_RATE_1, RfSettingsB1[dataRate][7]);
		macSpiWriteReg(SI4432_TX_DATA_RATE_0, RfSettingsB1[dataRate][8]);
		macSpiWriteReg(SI4432_MODULATION_MODE_CONTROL_1,RfSettingsB1[dataRate][9]);
		macSpiWriteReg(SI4432_FREQUENCY_DEVIATION,RfSettingsB1[dataRate][10]);
		macSpiWriteReg(SI4432_AFC_LOOP_GEARSHIFT_OVERRIDE, RfSettingsB1[dataRate][12]);
		macSpiWriteReg(SI4431_AFC_LIMIT, RfSettingsB1[dataRate][13]);
		macSpiWriteReg(SI4432_AFC_TIMING_CONTROL, RfSettingsB1[dataRate][14]);
		macSpiWriteReg(SI4432_PREAMBLE_DETECTION_CONTROL, (Parameters[dataRate][PREAMBLE_DETECTION_THRESHOLD]<<3)|0x02);
		break;
	}
#endif//B1_ONLY
#ifdef B1_ONLY
	macSpiWriteReg(SI4432_IF_FILTER_BANDWIDTH, RfSettingsB1[dataRate][0] );
	macSpiWriteReg(SI4432_CLOCK_RECOVERY_OVERSAMPLING_RATIO, RfSettingsB1[dataRate][1]);
	macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_2, RfSettingsB1[dataRate][2]);
	macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_1, RfSettingsB1[dataRate][3]);
	macSpiWriteReg(SI4432_CLOCK_RECOVERY_OFFSET_0, RfSettingsB1[dataRate][4]);
	macSpiWriteReg(SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_1, RfSettingsB1[dataRate][5]);
	macSpiWriteReg(SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_0, RfSettingsB1[dataRate][6]);
	macSpiWriteReg(SI4432_TX_DATA_RATE_1, RfSettingsB1[dataRate][7]);
	macSpiWriteReg(SI4432_TX_DATA_RATE_0, RfSettingsB1[dataRate][8]);
	macSpiWriteReg(SI4432_MODULATION_MODE_CONTROL_1,RfSettingsB1[dataRate][9]);
	macSpiWriteReg(SI4432_FREQUENCY_DEVIATION,RfSettingsB1[dataRate][10]);
	macSpiWriteReg(SI4432_AFC_LOOP_GEARSHIFT_OVERRIDE, RfSettingsB1[dataRate][12]);
	macSpiWriteReg(SI4431_AFC_LIMIT, RfSettingsB1[dataRate][13]);
	macSpiWriteReg(SI4432_AFC_TIMING_CONTROL, RfSettingsB1[dataRate][14]);
	macSpiWriteReg(SI4432_PREAMBLE_DETECTION_CONTROL, (Parameters[dataRate][PREAMBLE_DETECTION_THRESHOLD]<<3)|0x02);
#endif//B1_ONLY
	//set frequency parameters (center frequency and frequency hopping step size)
	macSpiWriteReg(SI4432_FREQUENCY_BAND_SELECT, Parameters[dataRate][START_FREQUENCY_1]);
	macSpiWriteReg(SI4432_NOMINAL_CARRIER_FREQUENCY_1, Parameters[dataRate][START_FREQUENCY_2]);
	macSpiWriteReg(SI4432_NOMINAL_CARRIER_FREQUENCY_0, Parameters[dataRate][START_FREQUENCY_3]);
	macSpiWriteReg(SI4432_FREQUENCY_HOPPING_STEP_SIZE, Parameters[dataRate][STEP_FREQUENCY]);

#ifdef MORE_CHANNEL_IS_USED
	macSpiWriteReg(SI4432_PLL_TUNE_TIME, Parameters[dataRate][PLL_TUNE_TIME_REG_VALUE]);
#endif

	//set preamble length according to number of used channel
#ifdef FOUR_CHANNEL_IS_USED
	if (numFreq == 0) {
		//if one channel is used
		macSpiWriteReg(SI4432_PREAMBLE_LENGTH, (Parameters[dataRate][PREAMBLE_IF_ONE_CHANNEL])<<1);
	} else if (numFreq == 1) {
		//if two channel is used
		macSpiWriteReg(SI4432_PREAMBLE_LENGTH, (Parameters[dataRate][PREAMBLE_IF_TWO_CHANNEL])<<1);
	} else if (numFreq == 2) {
		//if three channel is used
		macSpiWriteReg(SI4432_PREAMBLE_LENGTH, (Parameters[dataRate][PREAMBLE_IF_THREE_CHANNEL])<<1);
	} else { // default four channels
		//if four channel is used
		macSpiWriteReg(SI4432_PREAMBLE_LENGTH, (Parameters[dataRate][PREAMBLE_IF_FOUR_CHANNEL])<<1);
	}
#endif//FOUR_CHANNEL_IS_USED
#ifdef MORE_CHANNEL_IS_USED
	maxChannelNumber = Parameters[dataRate][CHANNEL_NUMBERS];
	temp8 = macSpiReadReg(SI4432_HEADER_CONTROL_2);
	macSpiWriteReg(SI4432_HEADER_CONTROL_2, (temp8 | Parameters[dataRate][PREAMBLE_LENGTH_REG_VALUE1]));
	macSpiWriteReg(SI4432_PREAMBLE_LENGTH, Parameters[dataRate][PREAMBLE_LENGTH_REG_VALUE2]);
#endif//MORE_CHANNEL_IS_USED

}


//------------------------------------------------------------------------------------------------
// Function Name: macSpecialRegisterSettings
//
// Return Value : None
// Parameters   : chiptype - Device Type Register value
//
//
//-----------------------------------------------------------------------------------------------
static void macSpecialRegisterSettings(U8 chiptype)
{

	switch(chiptype) {
#ifndef B1_ONLY
	case 0://rev V2
		//these settings need for good RF link(V2 specific settings)
		macSpiWriteReg(SI4432_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE, 0x03);
		//set VCO
		macSpiWriteReg(SI4432_VCO_CURRENT_TRIMMING, 0x7F);
		macSpiWriteReg(SI4432_DIVIDER_CURRENT_TRIMMING, 0x40);
		//set the AGC
		macSpiWriteReg(SI4432_AGC_OVERRIDE_2, 0x0B);
		//set ADC reference voltage to 0.9V
		macSpiWriteReg(SI4432_DELTASIGMA_ADC_TUNING_2, 0x04);
		//set the crystal capacitance bank
		macSpiWriteReg(SI4432_CRYSTAL_OSCILLATOR_LOAD_CAPACITANCE, 0xDD);
		break;
	case 1: //rev A0
		//set VCO
		macSpiWriteReg(SI4432_VCO_CURRENT_TRIMMING, 0x01);
		macSpiWriteReg(SI4432_DIVIDER_CURRENT_TRIMMING, 0x00);

		macSpiWriteReg(SI4432_CHARGEPUMP_TEST, 0x01);
		//set the Modem test register
		macSpiWriteReg(SI4432_MODEM_TEST, 0xC1);
		//set the crystal capacitance bank
		macSpiWriteReg(SI4432_CRYSTAL_OSCILLATOR_LOAD_CAPACITANCE, 0xDD);
		break;
#endif //B1_ONLY
	case 2:
	default:	//rev B1
		// Set AGC Override1 Register
		macSpiWriteReg(SI4432_AGC_OVERRIDE_1, 0x60);
		//set the crystal capacitance bank
		macSpiWriteReg(SI4432_CRYSTAL_OSCILLATOR_LOAD_CAPACITANCE, 0x6D);
		break;
	}
}


//------------------------------------------------------------------------------------------------
// Function Name: macUpdateDynamicTimeouts
//		This function calculate all of the timeouts for the EZMac stack.
//
// Return Value : None
// Parameters   : mcr - Master Control Register value
//		mpl - Maximum Payload size
//------------------------------------------------------------------------------------------------
static void macUpdateDynamicTimeouts(U8 mcr, U8 mpl)
{
	U16 n;
	U8 preamble;
	S8 header;
	U16 byteTime;

	ezmacprof_debug(("func:%s(), line:%d, mcr:0x%x, mpl:%d\n", __FUNCTION__, __LINE__, mcr, mpl));

	// look up byte time
	n = ((mcr >> 5) & 0x03);
	byteTime = EZMacProByteTime[n];

	//determine the preamble length
#ifdef FOUR_CHANNEL_IS_USED
	preamble = Parameters[n][PREAMBLE_IF_ONE_CHANNEL + (mcr & 0x03)];
#endif //FOUR_CHANNEL_IS_USED
#ifdef MORE_CHANNEL_IS_USED
	preamble = Parameters[n][PREAMBLE_LENGTH];
#endif //MORE_CHANNEL_IS_USED

	//determine the header length
#ifdef STANDARD_PACKET_FORMAT
	if(mcr & 0x80) {    //if CID is used
		header = 3;     //CID+SID+DID
	} else {
		header = 2;     //SID+DID
	}
#endif //STANDARD_PACKET_FORMAT
#ifdef EXTENDED_PACKET_FORMAT
	if(mcr & 0x80) {    //if CID is used
		header = 4;     //CTRL+CID+SID+DID
	} else {
		header = 3;     //CTRL+SID+DID
	}
#endif //EXTENDED_PACKET_FORMAT

	//if DNPL
	if (mcr & 0x04) {
		header++;       //add one for length
	}

#ifndef TRANSMITTER_ONLY_OPERATION
	//update the sync word timeout
	TimeoutSyncWord = (preamble + 2) * (U32)byteTime;
	//update the channel search timeout
#ifdef FOUR_CHANNEL_IS_USED
	TimeoutChannelSearch = (byteTime) * Parameters[n][SEARCH_TIME];
#endif//FOUR_CHANNEL_IS_USED
#ifdef MORE_CHANNEL_IS_USED
	TimeoutChannelSearch = (byteTime/8) * Parameters[n][SEARCH_TIME];
#endif//MORE_CHANNEL_IS_USED

	// calculate TimeoutRX_Packet using mpl
	n = (U8)header + mpl + 2 + 3;
	TimeoutRX_Packet = n * (U32)(byteTime);
#endif //TRANSMITTER_ONLY_OPERATION
#ifndef RECEIVER_ONLY_OPERATION
	// calculate TimeoutTX_Packet using MPL
	n = preamble + SYNC_WORD_LENGTH + (U8)header + mpl + 2 + 8;
	TimeoutTX_Packet = n * (U32)(byteTime);
#endif

	ezmacprof_debug(("func:%s(), line:%d, byteTime:%d, preamble:%d, header:%d, TimeoutSyncWord:%d, "
			"TimeoutRX_Packet:%d, TimeoutTX_Packet:%d\n", __FUNCTION__, __LINE__,
			byteTime, preamble, header, TimeoutSyncWord, TimeoutRX_Packet, TimeoutTX_Packet));

#ifdef EXTENDED_PACKET_FORMAT
	// if DNPL
	if ((mcr & 0x04) == 0x04) {
		// use fixed one byte payload for ACK
		//mpl = 1;
		// use ACK_BUFFER_SIZE for calculation of TimeoutACK
		mpl = ACK_BUFFER_SIZE;
	}

	n = preamble + SYNC_WORD_LENGTH + (U8)header + mpl + 2 + 3;
	//ACK time out: the SW make up the ACK packet(in 4 Mhz ~1.6ms) + 200 us(PLL settling time) + n * byte time
	TimeoutACK = (n * (U32)(byteTime));
	TimeoutACK += (U32)(MAKE_UP_THE_ACK_PACKET + PLL_SETTLING_TIME);

#ifdef PACKET_FORWARDING_SUPPORTED
	// use n for radius
	n = (mcr & 0x18) >> 3;

	if (n) {
		TimeoutACK = ((n + 1) * TimeoutACK) + (n * TimeoutTX_Packet) + (n * MAX_LBT_WAITING_TIME * MAX_LBT_RETRIES);
	}
#endif//PACKET_FORWARDING_SUPPORTED
#endif//EXTENDED_PACKET_FORMAT

	ezmacprof_debug(("func:%s(), line:%d, mpl:%d, TimeoutACK:%d\n", __FUNCTION__, __LINE__,  mpl, TimeoutACK));
}


//------------------------------------------------------------------------------------------------
// Function Name: initForwardedPacketTable
//						This function resets the forwarding table.
// Return Value : None
// Parameters   : None
//------------------------------------------------------------------------------------------------
#ifdef PACKET_FORWARDING_SUPPORTED
static void initForwardedPacketTable (void)
{
	U8 i;
	for (i = 0; i < FORWARDED_PACKET_TABLE_SIZE; i++) {
		ForwardedPacketTable[i].sid = 0xFF;
		ForwardedPacketTable[i].seq = 0xFF;
		ForwardedPacketTable[i].chan = 0xFF;
	}
}
#endif


//------------------------------------------------------------------------------------------------
// Function Name: macSetEnable2
//						This function can be use to set the Interrupt Enable2 register of the radio
// Return Value : None
// Parameters   : value - which bit should be set
//------------------------------------------------------------------------------------------------
static void macSetEnable2(U8 value)
{

	if((EZMacProReg.name.LFTMR2 & 0x80)==0x80)
		value |= SI4432_ENWUT;

	if((EZMacProReg.name.LBDR & 0x80)==0x80)
		value |= SI4432_ENLBDI;

	macSpiWriteReg(SI4432_INTERRUPT_ENABLE_2, value);
}


//------------------------------------------------------------------------------------------------
// Function Name: macSetFunction1
//						This function can be use to set the Operation Function Control1 register of the radio
// Return Value : None
// Parameters   : value -which bit should be set
//
//------------------------------------------------------------------------------------------------
static void macSetFunction1(U8 value)
{

	if((EZMacProReg.name.LFTMR2 & 0x80)==0x80)
		value |= SI4432_ENWT;

	if((EZMacProReg.name.LFTMR2 & 0x40)==0x00)
		value |= SI4432_X32KSEL;

	if((EZMacProReg.name.LBDR & 0x80)==0x80)
		value |= SI4432_ENLBD;

	ezmacprof_debug(("func:%s(), line:%d, SI4432_OPERATING_AND_FUNCTION_CONTROL_1:0x%2x --> 0x%2x\n"
			,__FUNCTION__, __LINE__, macSpiReadReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1)
			,value));

	macSpiWriteReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1, value);
}


//------------------------------------------------------------------------------------------------
// Function Name: macUpdateLBTI
//		This function update the Listen Before Talk timeout.
//
// Return Value : None
// Parameters   : lbti - Listen Before Talk Interval Register value
//
//------------------------------------------------------------------------------------------------

#ifdef   TRANSCEIVER_OPERATION
static void macUpdateLBTI (U8 lbti)
{
	U8 rate;

	// check fixed or byte time LBT
	if((lbti & 0x80) == 0x80) {
		// set timeout to 100 us
		TimeoutLBTI = TIMEOUT_US(100);
	} else {
		// get data rate from MCR
		rate = ((EZMacProReg.name.MCR >> 5)&0x03);
		//look-up byte time
		TimeoutLBTI = EZMacProByteTime[rate];
	}

	// multiply by LBTI register
	TimeoutLBTI *= (U32)(lbti& 0x7F);
}
#endif //TRANSCEIVER_OPERATION

#if !SET_EZMACPRO_MSR_USE_MACR
base_t set_ezmacpro_msr(U8 state)
{
#if 0
	ubase_t old_state;

	old_state =  EZMacProReg.name.MSR;
	EZMacProReg.name.MSR = state;

#if 1
	printf_syn("mac msr:0x%02x-->0x%02x\n", old_state, state);
#else
	rt_kprintf("mac msr:0x%02x-->0x%02x\n", old_state, state);
#endif

#else
	EZMacProReg.name.MSR = state;
#endif

	return 0;
}
#endif

U8     get_ezmacpro_msr(void)
{
	return EZMacProReg.name.MSR;
}


U8     get_ezmacpro_rssi(void)
{
	return EZMacProReg.name.RSSI;
}


void print_si4432_state(void)
{
	printf_syn("mac-sr:0x%x, dev-state:0x%x, int-state1:0x%x, int-state2:0x%x, int-enable1:0x%x, int-enable2:0x%x, "
			"oper-ctrl1:0x%x, oper-ctrl2:0x%x\n",
			EZMacProReg.name.MSR, macSpiReadReg(SI4432_DEVICE_STATUS),
			macSpiReadReg(SI4432_INTERRUPT_STATUS_1), macSpiReadReg(SI4432_INTERRUPT_STATUS_2),
			macSpiReadReg(SI4432_INTERRUPT_ENABLE_1), macSpiReadReg(SI4432_INTERRUPT_ENABLE_2),
			macSpiReadReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1),
			macSpiReadReg(SI4432_OPERATING_AND_FUNCTION_CONTROL_2) );

	return;
}

//================================================================================================
// end EZMacPro.c
//================================================================================================
