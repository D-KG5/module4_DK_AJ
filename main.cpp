#include "mbed.h"
#include "arm_math.h"
// FFT library Copyright (C) Tilen Majerle, 2015
#include "tm_stm32f4_fft.h"
#include <cstdio>

#define ADC_12BIT 4096.0
#define ADC_VREF 3.30
/* FFT settings */
#define SAMPLES                    512             /* 256 real party and 256 imaginary parts */
#define FFT_SIZE                SAMPLES / 2        /* FFT size is always the same size as we have samples, so 256 in our case */

float32_t Input[SAMPLES];
float32_t Output[FFT_SIZE];

float32_t fake_dac_buffer[SAMPLES];

DigitalOut LEDS(LED1);
uint16_t frequency = 0;

void fake_dac(void){
    static float32_t step = 0;
    float voltage = 0;

    for(int16_t i = 0; i < SAMPLES; i++){
        voltage = 2 + arm_sin_f32((float32_t)(2008.0 * PI * step)); // 1004 Hz sine wave
        fake_dac_buffer[i] = (float32_t) ((ADC_12BIT * voltage / ADC_VREF) - 1);
        step += 0.00002;
        //printf("%f\r", fake_dac_buffer[i]);
    }
}

// main() runs in its own thread in the OS
int main()
{
    TM_FFT_F32_t FFT;
    uint16_t i = 0;

    TM_FFT_Init_F32(&FFT, FFT_SIZE, 0);

    TM_FFT_SetBuffers_F32(&FFT, Input, Output);

    fake_dac();
    
    while (true) {
        //while loop to add samples at fixed rate
        while(!TM_FFT_AddToBuffer(&FFT, (fake_dac_buffer[i] - (float32_t)2048.0))){
            i++;
            wait_us(23); // sample rate faking
        }

        TM_FFT_Process_F32(&FFT);

        // print fundamental freq
        frequency = TM_FFT_GetMaxIndex(&FFT);
        printf("Fundamental frequency is %d Hz\r\n", frequency);

        // blink led proportional to fundamental freq
        uint32_t period_us = frequency * 1000000;
        printf("Period is %d us\n", period_us);
        while(1){
            // end program
            LEDS = !LEDS;
            wait_us(period_us);
        }

    }
}

