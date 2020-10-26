#include "mbed.h"
#include "arm_math.h"
// FFT library is Copyright (C) Tilen Majerle, 2015
#include "tm_stm32f4_fft.h"
#include <cstdio>

// Fake ADC/DAC scaling vars
#define ADC_12BIT 4096.0
#define ADC_VREF 3.30
// FFT vars
#define SAMPLES 1024  // 512 real parts and 512 imaginary parts
#define FFT_SIZE SAMPLES / 2    // FFT size is always the same size as we have samples

// set up input and output buffers for FFT library
float32_t Input[SAMPLES];
float32_t Output[FFT_SIZE];
// set up fake dac buffer
float32_t fake_dac_buffer[SAMPLES];
// set up LED
DigitalOut LEDS(LED1);

/**
* This function is used to create sine wave samples from a fake DAC.
* @author Dhruva Koley
* @date 10/26/2020
*/
void fake_dac(void) {
    static float32_t step = 0;
    float voltage = 0;
    // output a sine wave with a frequency of 10004 Hz
    for(int16_t i = 0; i < SAMPLES; i++){
        voltage = 2 + arm_sin_f32((float32_t)(2008.0 * PI * step)); // 1004 Hz sine wave [f(x) = 2 + sin(2008*pi*x) ] with an offset of 2 because negative numbers are considered evil
        fake_dac_buffer[i] = (float32_t) ((ADC_12BIT * voltage / ADC_VREF) - 1);    // scale raw sine samples to be what a DAC would output
        step += 0.00002;    // increment step size to ensure sine wave is detailed enough
    }
}

/**
* This function is used to run the fake DAC and the FFT analysis
* for module 4.
* @author Dhruva Koley
* @date 10/26/2020
*/
int main() {
    TM_FFT_F32_t FFT;   // set up FFT struct for FFT analysis
    uint16_t i = 0;
    uint16_t frequency = 0;

    TM_FFT_Init_F32(&FFT, FFT_SIZE, 0); // initialize FFT struct with no malloc

    TM_FFT_SetBuffers_F32(&FFT, Input, Output); // set up FFT struct buffers since malloc isn't used

    fake_dac(); // run fake DAC to fill DAC buffer with sine wave samples
    
    while (true) {
        // while loop to add samples at fixed rate from the buffer
        while(!TM_FFT_AddToBuffer(&FFT, (fake_dac_buffer[i] - (float32_t)2048.0))) {
            i++;
            wait_us(20); // fake sample rate of 50 kHz
        }

        TM_FFT_Process_F32(&FFT);   // process buffer and calculate FFT, max value, max index

        // print fundamental freq
        frequency = TM_FFT_GetMaxIndex(&FFT);
        printf("Fundamental frequency is %d Hz\r\n", frequency);

        // blink led proportional to fundamental freq
        uint32_t freq_us = frequency * 1000000;
        while(1) {
            LEDS = !LEDS;
            wait_us(freq_us);
        }

    }
    return 0;
}

