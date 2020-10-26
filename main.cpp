#include "mbed.h"
#include "arm_math.h"
// FFT library is Copyright (C) Tilen Majerle, 2015
#include "tm_stm32f4_fft.h"
#include <cstdint>
#include <cstdio>

// Fake ADC/DAC scaling vars
#define ADC_12BIT 4096.0
#define ADC_VREF 3.30
// FFT vars
#define SAMPLES 1024  // 512 real parts and 512 imaginary parts
#define FFT_SIZE SAMPLES / 2    // FFT size is always the same size as we have samples
// Zero-cross algorithm vars
#define FREQ_SR 50000   // Sample rate of sine wave (50kHz)

int32_t prevsamp[2] = {0}; // buffer to check for zero crossing
int sampcount = 0;  // used to calculate frequency

// set up input and output buffers for FFT library
float32_t Input[SAMPLES];
float32_t Output[FFT_SIZE];
// set up fake dac buffer
int32_t fake_dac_buffer[SAMPLES];
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
        voltage = arm_sin_f32((float32_t)(2008.0 * PI * step)); // 1004 Hz sine wave [f(x) = sin(2008*pi*x)]
        fake_dac_buffer[i] = (int32_t) ((ADC_12BIT * voltage / ADC_VREF) - 1);    // scale raw sine samples to be what a DAC would output
        step += 0.00001;    // increment step size to ensure sine wave is detailed enough
    }
}

/**
* This function is used to calculate the frequency from the fake DAC.
* @author Dhruva Koley & Alex Fritz
* @param sample fake DAC sample
* @param prevresult previous result
* @return uint16_t frequency from fake DAC samples
*/
uint32_t freq_calc(int32_t sample, uint16_t prevresult) {
    float32_t result;
    // shift the buffer contents
    prevsamp[1] = prevsamp[0];
    prevsamp[0] = sample;
    // check for zero crossings
    if(prevsamp[0] > 0) {
        if(prevsamp[1] <= 0) {
            result = 1/ ((float32_t)sampcount / FREQ_SR) * 2;
            sampcount = 0;
            return result;
        }
    } else {
        sampcount++;
        return prevresult;
    }
    if(prevsamp[0] < 0) {
        if(prevsamp[1] >= 0) {
            result = 1 / (sampcount / FREQ_SR) * 2;
            sampcount = 0;
            return result;
        }
    } else {
        sampcount++;
        return prevresult;
    }
    return 0;
}

/**
* This function is used to run the fake DAC, the FFT analysis, and
* zero-crossing backup algorithm for module 4.
* @author Dhruva Koley
* @date 10/26/2020
*/
int main() {
    TM_FFT_F32_t FFT;   // set up FFT struct for FFT analysis
    uint16_t i = 0, period;
    uint32_t frequency, frequency_backup;

    fake_dac(); // run fake DAC to fill DAC buffer with sine wave samples

    TM_FFT_Init_F32(&FFT, FFT_SIZE, 0); // initialize FFT struct with no malloc

    TM_FFT_SetBuffers_F32(&FFT, Input, Output); // set up FFT struct buffers since malloc isn't used


    while (true) {
        // while loop to add samples at fixed rate from the buffer
        while(!TM_FFT_AddToBuffer(&FFT, fake_dac_buffer[i])) {
            i++;
            wait_us(20); // fake sample rate of 50 kHz
        }

        TM_FFT_Process_F32(&FFT);   // process buffer and calculate FFT, max value, max index
        frequency = TM_FFT_GetMaxIndex(&FFT);

        // backup zero-crossing algorithm to measure frequency
        i = 0;
        while(i < SAMPLES) {
            frequency_backup = freq_calc(fake_dac_buffer[i], frequency_backup);   // calculate frequency from samples
            i++;
            wait_us(20); // fake sample rate of 50 kHz
        }

        // check if FFT frequency and zero-crossing algo frequency match
        // if they don't then pick the backup frequency value
        if(frequency != frequency_backup){
            frequency = frequency_backup;
        }
        period = (1 / ((float32_t)frequency)) * 1000000;    // get period of frequency in uS
        printf("Fundamental frequency is %d Hz\r\n", frequency);
        printf("Period is %d uS\n", period);


        while(1) {
            // blink led proportional to fundamental frequency
            LEDS = !LEDS;
            wait_us(period);
        }

    }
    return 0;
}

