#include <stm32f7xx.h>

#define USING_FSK

#ifndef USING_FSK
	#define USING_QPSK
#endif

extern "C"{
	#include "flash.h"
	#include "bl_utils.h"
}
#include "buttons.hh"
#include "gates.hh"
#include "leds.hh"

#ifdef USING_QPSK
	#include "stm_audio_bootloader/qpsk/packet_decoder.h"
	#include "stm_audio_bootloader/qpsk/demodulator.h"
#else
	#include "stm_audio_bootloader/fsk/packet_decoder.h"
	#include "stm_audio_bootloader/fsk/demodulator.h"
#endif

extern "C" {

using namespace stmlib;
using namespace stm_audio_bootloader;

const uint32_t SAMPLE_RATE = 48000;

#ifdef USING_QPSK
const float kModulationRate = 6000.0;
const float kBitRate = 12000.0;
const float kSampleRate = 48000.0;
#endif
uint32_t kStartExecutionAddress =	0x08004000;
uint32_t kStartReceiveAddress = 	0x08004000;
uint32_t EndOfMemory =				0x0800FFFC;

extern const uint32_t FLASH_SECTOR_ADDRESSES[];
const uint32_t kBlkSize = 16384;
const uint16_t kPacketsPerBlock = kBlkSize / kPacketSize;
uint8_t recv_buffer[kBlkSize];

PacketDecoder decoder;
Demodulator demodulator;

uint16_t packet_index;
uint16_t old_packet_index=0;
uint16_t discard_samples = 8000;
uint32_t current_address;

enum UiState {
	UI_STATE_WAITING,
	UI_STATE_RECEIVING,
	UI_STATE_ERROR,
	UI_STATE_WRITING,
	UI_STATE_DONE
};
volatile UiState ui_state;

enum Animations {
	ANI_WAITING, 
	ANI_WRITING,
	ANI_RECEIVING,
	ANI_DONE,
	ANI_SUCCESS,
	ANI_FAIL_ERR,
	ANI_FAIL_SYNC,
	ANI_FAIL_CRC,
	ANI_RESET
};

Leds leds;
Gates gates;
Buttons buttons;

void read_gate_input(void);
void animate(enum Animations animation_type);


void update_LEDs(void)
{
	static uint16_t dly=0;
	uint16_t fade_speed=800;

	if (ui_state == UI_STATE_RECEIVING)
	{
		if (packet_index > old_packet_index)
		{
			if (packet_index & 1)
				leds.freeze_.set(Colors::white);
			else
				leds.freeze_.set(Colors::blue);

			old_packet_index = packet_index;

		}
	}
	else if (ui_state == UI_STATE_WRITING)
	{
		if (dly++>200)
		{
			leds.freeze_.set(Colors::yellow);
			leds.learn_.set(Colors::black);
			dly=0;

		} else if (dly==100)
		{
			leds.freeze_.set(Colors::black);
			leds.learn_.set(Colors::yellow);
		}
	} 
	else if (ui_state == UI_STATE_WAITING)
	{
		animate(ANI_WAITING);
	} 
	else if (ui_state == UI_STATE_DONE)
	{
		//Flash button blue/green when done
		if (dly==(fade_speed>>1)){
			leds.freeze_.set(Colors::green);
			leds.learn_.set(Colors::green);
		}
		if (dly++>=fade_speed) {
			dly=0;
			leds.freeze_.set(Colors::blue);
			leds.learn_.set(Colors::blue);
		}
	}

}

void InitializeReception(void)
{
	#ifdef USING_QPSK
		//QPSK
		decoder.Init((uint16_t)20000);
		demodulator.Init(
		 kModulationRate / kSampleRate * 4294967296.0,
		 kSampleRate / kModulationRate,
		 2.0 * kSampleRate / kBitRate);
		demodulator.SyncCarrier(true);
		decoder.Reset();
	#else
		//FSK
		decoder.Init();
		decoder.Reset();
		demodulator.Init(16, 8, 4); //pause, one, zero. pause_thresh = 24. one_thresh = 6.
		demodulator.Sync();
	#endif
	
	current_address = kStartReceiveAddress;
	packet_index = 0;
	old_packet_index = 0;
	ui_state = UI_STATE_WAITING;
}

void HAL_SYSTICK_Callback(void)
{
	read_gate_input();
	update_LEDs();
}


int main(void)
{
	uint32_t symbols_processed=0;
	uint32_t dly=0, button_debounce=0;
	uint8_t do_bootloader;
	uint8_t symbol;
	PacketDecoderState state;
	bool rcv_err;
	uint32_t last_flash;
	uint8_t learn_last_detected_released=0, freeze_last_detected_released=0;
	uint8_t exit_updater=false;

	SetVectorTable(0x08000000);

	HAL_Init();
	SystemClock_Config();

	HAL_Delay(3000);


	leds.freeze_.set(Colors::black);
	leds.learn_.set(Colors::green);

	dly=32000;
	while(dly--){
		buttons.learn_.Debounce();
		if (buttons.learn_.pushed()) button_debounce++;
		else button_debounce=0;
	}
	do_bootloader = (button_debounce>15000) ? 1 : 0;

	if (do_bootloader)
	{
		#ifdef USING_FSK
			InitializeReception(); //FSK
		#endif

		while(buttons.learn_.pushed())
			buttons.learn_.Debounce();

		learn_last_detected_released = 0;
		freeze_last_detected_released = 0;

		buttons.learn_.Debounce();
		HAL_Delay(1000);

		while (!exit_updater)
		{
			rcv_err = false;

			while (demodulator.available() && !rcv_err && !exit_updater) {
				symbol = demodulator.NextSymbol();
				state = decoder.ProcessSymbol(symbol);
				symbols_processed++;

				switch (state) {
					case PACKET_DECODER_STATE_OK:
					{
						ui_state = UI_STATE_RECEIVING;
						memcpy(recv_buffer + (packet_index % kPacketsPerBlock) * kPacketSize, decoder.packet_data(), kPacketSize);
						++packet_index;
						if ((packet_index % kPacketsPerBlock) == 0) {
							ui_state = UI_STATE_WRITING;

							//Check for valid flash address before writing to flash
							if ((current_address + kBlkSize) < FLASH_SECTOR_ADDRESSES[NUM_FLASH_SECTORS])
							{
								write_flash_page(recv_buffer, current_address, kBlkSize);
								current_address += kBlkSize;
							}
							else {
								ui_state = UI_STATE_ERROR;
								leds.freeze_.set(Colors::red);
								leds.learn_.set(Colors::red);
								rcv_err = true;
							}

							decoder.Reset();

							#ifndef USING_QPSK
								demodulator.Sync(); //FSK
							#else
								demodulator.SyncCarrier(false);//QPSK
							#endif

						} else {
							#ifndef USING_QPSK
								decoder.Reset(); //FSK
							#else
								demodulator.SyncDecision();//QPSK
							#endif
						}
					}
					break;

					case PACKET_DECODER_STATE_ERROR_SYNC:
						leds.freeze_.set(Colors::magenta);
						leds.learn_.set(Colors::red);
						rcv_err = true;
						break;

					case PACKET_DECODER_STATE_ERROR_CRC:
						leds.freeze_.set(Colors::red);
						leds.learn_.set(Colors::magenta);
						rcv_err = true;
						break;

					case PACKET_DECODER_STATE_END_OF_TRANSMISSION:
						//Copy from Receive buffer to Execution memory
						//copy_flash_page(kStartReceiveAddress, kStartExecutionAddress, (current_address-kStartReceiveAddress));

						exit_updater = true;
						ui_state = UI_STATE_DONE;

						//Do a success animation
						animate(ANI_RESET);
						while (!buttons.learn_.pushed())
						{
							buttons.learn_.Debounce();
							animate(ANI_SUCCESS);
							HAL_Delay(1000);
						}
						while (buttons.learn_.pushed())
							buttons.learn_.Debounce();

						break;

					default:
						break;
				}
			}
			if (rcv_err) {
				ui_state = UI_STATE_ERROR;

				//flash button red/off until it's pressed
				last_flash = HAL_GetTick();

				while (!buttons.learn_.pushed()) {
					buttons.learn_.Debounce();
					if (HAL_GetTick() - last_flash >= 100){ 
						leds.learn_.set(Colors::black);
					}
					if (HAL_GetTick() - last_flash >= 200){ 
						last_flash = HAL_GetTick(); 
						leds.learn_.set(Colors::red);
					}
				}
				while (buttons.learn_.pushed())
					buttons.learn_.Debounce();

				leds.freeze_.set(Colors::black);
				leds.learn_.set(Colors::black);

				InitializeReception();

				learn_last_detected_released = 0;
				freeze_last_detected_released = 0;

				exit_updater=false;
			}

			buttons.learn_.Debounce();
			buttons.freeze_.Debounce();

			if (buttons.freeze_.just_pushed()) {
				if (packet_index==0)
					exit_updater=true;
			}

			if (buttons.learn_.just_pushed()) {
				if ((ui_state == UI_STATE_WAITING))
					exit_updater=true;
			}



		}
	}
	HAL_DeInit();
	HAL_RCC_DeInit();

	JumpTo(0x08004000);


	return 1;
}

void animate(enum Animations animation_type)
{
	uint32_t cur_tm = HAL_GetTick();
	static uint32_t last_tm = 0;
	static uint8_t ctr = 0;
	uint32_t step_time;

	switch (animation_type) {

		case ANI_RESET:
			last_tm = cur_tm;
			ctr = 0;
			break;

		case ANI_SUCCESS:
			step_time = 500*TICKS_PER_MS;

			if (ctr==0) {
				leds.freeze_.set(Colors::red);
				leds.learn_.set(Colors::red);
			}
			if (ctr==1) {
				leds.freeze_.set(Colors::yellow);
				leds.learn_.set(Colors::yellow);
			}
			if (ctr==2) {
				leds.freeze_.set(Colors::green);
				leds.learn_.set(Colors::green);
			}
			if (ctr==3) {
				leds.freeze_.set(Colors::cyan);
				leds.learn_.set(Colors::cyan);
			}
			if (ctr==4) {
				leds.freeze_.set(Colors::blue);
				leds.learn_.set(Colors::blue);
			}
			if (ctr==5) {
				leds.freeze_.set(Colors::magenta);
				leds.learn_.set(Colors::magenta);
			}
			if (ctr==6) {
				leds.freeze_.set(Colors::white);
				leds.learn_.set(Colors::white);
			}
			if (ctr==7)
				ctr = 0;
			break;

		case ANI_WAITING:
			//Flash button green/off when waiting
			step_time = 500*TICKS_PER_MS;

			if (ctr==0)
				leds.learn_.set(Colors::black);
			if (ctr==1)
				leds.learn_.set(Colors::green);
			if (ctr==2)
				ctr=0;


			break;
		default:
			break;
	}

	if ((cur_tm - last_tm) > step_time) {
		ctr++;
		last_tm = cur_tm;
	}


}

void read_gate_input(void)
{
	bool sample = !gates.learn_.get();

	if (!discard_samples) {
		#ifdef USING_FSK
		demodulator.PushSample(sample);
		#else
		demodulator.PushSample(sample ? 0x7FFF : 0);
		#endif
	} else {
		--discard_samples;
	}
}


} //extern "C"
