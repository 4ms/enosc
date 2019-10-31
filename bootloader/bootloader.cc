#include <stm32f7xx.h>

#define USING_FSK

#ifndef USING_FSK
    #define USING_QPSK
#endif

#include "bootloader.hh"
#include "bl_ui.hh"

#ifdef USING_QPSK
    #include "stm_audio_bootloader/qpsk/packet_decoder.h"
    #include "stm_audio_bootloader/qpsk/demodulator.h"
#else
    #include "stm_audio_bootloader/fsk/packet_decoder.h"
    #include "stm_audio_bootloader/fsk/demodulator.h"
#endif

using namespace stmlib;
using namespace stm_audio_bootloader;

extern "C" {
#include "periodic_func.h"
#include "flash.h"
#include "leds.h"
#include "buttons.h"
#include "hardware_test.h"
#include "bl_utils.h"
#include "lib/stm32f7xx_ll_rcc.h"
#include "lib/stm32f7xx_ll_pwr.h"
#include "lib/stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_bus.h"

#ifdef USING_QPSK
    const float kModulationRate = 6000.0;
    const float kBitRate = 12000.0;
    const float kSampleRate = 48000.0;
#else
    const uint32_t kSampleRate = 22050;
#endif
uint32_t kStartExecutionAddress =   0x08004000;
uint32_t kStartReceiveAddress =     0x08004000;

extern const uint32_t FLASH_SECTOR_ADDRESSES[];
const uint32_t kBlkSize = 16384;
const uint16_t kPacketsPerBlock = kBlkSize / kPacketSize; //kPacketSize=256
uint8_t recv_buffer[kBlkSize];

volatile uint32_t systmr=0;
PacketDecoder decoder;
Demodulator demodulator;

uint16_t packet_index;
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

void read_gate_input(void);
void init_gate_input(void);

void update_LEDs(void)
{
    if (ui_state == UI_STATE_RECEIVING)
        animate(ANI_RECEIVING);

    else if (ui_state == UI_STATE_WRITING)
        animate(ANI_WRITING);

    else if (ui_state == UI_STATE_WAITING)
        animate(ANI_WAITING);

    else //if (ui_state == UI_STATE_DONE)
    {
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
    ui_state = UI_STATE_WAITING;
}

void SysTick_Handler(void)
{
    systmr++;
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
    uint8_t exit_updater=false;

    SetVectorTable(0x08000000);

    // SCB_EnableICache();
    // SCB_EnableDCache();
    NVIC_SetPriority(MemoryManagement_IRQn, 0);
    NVIC_SetPriority(BusFault_IRQn, 0);
    NVIC_SetPriority(UsageFault_IRQn, 0);
    NVIC_SetPriority(SVCall_IRQn, 0);
    NVIC_SetPriority(DebugMonitor_IRQn, 0);
    NVIC_SetPriority(PendSV_IRQn, 0);
    NVIC_SetPriority(SysTick_IRQn, 0);

    SystemClock_Config();

    delay(3000);

    init_debug();
    init_leds();
    init_buttons();

    animate(ANI_RESET);

    dly=32000;
    while(dly--){
        if (button_pushed(BUTTON_LEARN) && button_pushed(BUTTON_FREEZE))
            button_debounce++;
        else
            button_debounce=0;
    }
    do_bootloader = (button_debounce>15000) ? 1 : 0;

    delay(100);

    if (do_bootloader)
    {
        #ifdef USING_FSK
            InitializeReception(); //FSK
        #endif

        uint32_t hwtest_entry=0;
        while (button_pushed(BUTTON_LEARN) || button_pushed(BUTTON_FREEZE)) {
            if (button_pushed(BUTTON_LEARN) && !button_pushed(BUTTON_FREEZE)) {
                hwtest_entry++;
                if (hwtest_entry>1000000) {
                    ui_state = UI_STATE_DONE;
                    do_hardware_test();
                }
            }
            else hwtest_entry=0;
        }

        init_gate_input();
        const uint32_t period = F_CPU / 2 / kSampleRate;
        init_periodic_function(period, 0, read_gate_input);
        start_periodic_func();

        delay(100);

        uint32_t learn_exit_armed=0, freeze_exit_armed=0;
        
        while (!exit_updater)
        {
            rcv_err = false;

            while (demodulator.available() && !rcv_err && !exit_updater) {
                symbol = demodulator.NextSymbol();
                state = decoder.ProcessSymbol(symbol);
                symbols_processed++;

                switch (state) {
                    case PACKET_DECODER_STATE_OK:
                        ui_state = UI_STATE_RECEIVING;
                        memcpy(recv_buffer + (packet_index % kPacketsPerBlock) * kPacketSize, decoder.packet_data(), kPacketSize);
                        ++packet_index;
                        if ((packet_index % kPacketsPerBlock) == 0) {
                            ui_state = UI_STATE_WRITING;

                            //Check for valid flash address before writing to flash
                            if ((current_address + kBlkSize) <= FLASH_SECTOR_ADDRESSES[NUM_FLASH_SECTORS])
                            {
                                write_flash_page(recv_buffer, current_address, kBlkSize);
                                current_address += kBlkSize;
                            }
                            else {
                                ui_state = UI_STATE_ERROR;
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
                        break;

                    case PACKET_DECODER_STATE_ERROR_SYNC:
                        rcv_err = true;
                        break;

                    case PACKET_DECODER_STATE_ERROR_CRC:
                        rcv_err = true;
                        break;

                    case PACKET_DECODER_STATE_END_OF_TRANSMISSION:
                        exit_updater = true;
                        ui_state = UI_STATE_DONE;
                        animate_until_button_pushed(ANI_SUCCESS, BUTTON_LEARN);
                        animate(ANI_RESET);
                        delay(100);
                        break;

                    default:
                        break;
                }
            }
            if (rcv_err) {
                ui_state = UI_STATE_ERROR;
                animate_until_button_pushed(ANI_FAIL_ERR, BUTTON_LEARN);
                animate(ANI_RESET);
                delay(100);
                InitializeReception();

                exit_updater=false;
            }

            if (button_pushed(BUTTON_FREEZE)) {
                if (freeze_exit_armed) {
                    if (packet_index==0)
                        exit_updater=true;
                }
                freeze_exit_armed = 0;
            }
            else
                freeze_exit_armed = 1;

            if (button_pushed(BUTTON_LEARN)) {
                if (learn_exit_armed) {
                    if ((ui_state == UI_STATE_WAITING))
                        exit_updater=true;
                }
                learn_exit_armed = 0;
            }
            else
                learn_exit_armed = 1;

        }
    }
    while (button_pushed(BUTTON_LEARN) || button_pushed(BUTTON_FREEZE)) {;}

    reset_buses();
    reset_RCC();
    JumpTo(kStartExecutionAddress);
    return 1;
}

void init_gate_input(void)
{
    LL_AHB1_GRP1_EnableClock(BOOTLOADER_INPUT_RCC);
    LL_GPIO_SetPinMode(BOOTLOADER_INPUT_GPIO, BOOTLOADER_INPUT_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(BOOTLOADER_INPUT_GPIO, BOOTLOADER_INPUT_PIN, LL_GPIO_PULL_DOWN);
}

void read_gate_input(void)
{
    DEBUG1_ON;
    bool sample = ((BOOTLOADER_INPUT_GPIO->IDR & BOOTLOADER_INPUT_PIN) != 0);

    // if (sample) DEBUG1_ON;
    // else DEBUG1_OFF;

    if (!discard_samples) {
        #ifdef USING_FSK
        demodulator.PushSample(sample ? 1 : 0);
        #else
        demodulator.PushSample(sample ? 0x7FFF : 0);
        #endif
    } else {
        --discard_samples;
    }
    DEBUG1_OFF;
}

} //extern "C"
