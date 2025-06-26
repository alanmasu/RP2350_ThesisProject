#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include <BinaryMatMul.h>

#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"

#define PLL_40_MHZ (40 * 1000)

#define PROFILING

#if !defined(SIMULATION) && !defined(PROFILING)
    #define PRINTF_DBG(...) printf(__VA_ARGS__)
#else 
    #define PRINTF_DBG(...)
#endif

#if !defined(SIMULATION) && !defined(PROFILING)
    #define PROFILING_ENV_EXCLUSION(x) x
#else
    #define PROFILING_ENV_EXCLUSION(x)
#endif

#ifndef SIMULATION
    #define PRINTF_ERR(...) printf(__VA_ARGS__)
#else
    #define PRINTF_ERR(...)
#endif

#ifdef PROFILING
    #define PRINTF_LOG(...) printf(__VA_ARGS__)
#else
    #define PRINTF_LOG(...)
#endif

int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#endif
}

// Turn the led on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}

void set_sys_clock_40mhz() {
    pico_set_led(set_sys_clock_khz(PLL_40_MHZ, true));
}

#define ITERATIONS 6
#define TIMES_NUM 4

absolute_time_t times[TIMES_NUM * ITERATIONS] = {0};
uint32_t sizes[TIMES_NUM * ITERATIONS] = {0};
uint32_t timesIndex = 0;
uint32_t testN = 0;

char labels[TIMES_NUM][100] = {
    "Allocazione",
    "Popolazione",
    "ComputazioneSerialeFast",
    "freeMemory"
};

void printResults(){
    printf("Label,size(bit),timestamp(us),platform\n");
    for(int i = 0; i < timesIndex / TIMES_NUM; ++i){
        for (int label = 0; label < TIMES_NUM; ++label) {            
            if (label == 0){
                absolute_time_t start = times[i * TIMES_NUM];
                int64_t diff = i == 0 ? times[0] : absolute_time_diff_us(times[i * TIMES_NUM], times[(i - 1) * TIMES_NUM]);
                printf("%s,%d,,%llu,RP2350\n", labels[label], sizes[i * TIMES_NUM + label], diff);
            }else{  
                absolute_time_t start = times[i * TIMES_NUM];
                printf("%s,%d,,%llu,RP2350\n", labels[label], sizes[i * TIMES_NUM + label], 
                    absolute_time_diff_us(start, times[i * TIMES_NUM + label]));
            }
        }
    }
        
}

int main(){
    
    pico_led_init();
    
    set_sys_clock_40mhz();
    
    stdio_init_all();
    printf("Running at %u KHz\n", frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS));

    int bn = 0;
    const uint32_t signCmp = 48;

    for(int i = 0, n = 32; i < ITERATIONS; ++i, n *= 2){
        bn = n / 32;
        
        BinaryMatrix_t A = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));         //768 byte
        BinaryMatrix_t W = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));         // 1.536 kB
        BinaryMatrix_t OSerial = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));   // 1 kB

        sizes[timesIndex] = n;
        times[timesIndex++] = get_absolute_time();

        if(!A || !W || !OSerial){
            PRINTF_ERR("[ERROR]: Memory allocation failed -> N: %d\n", n);
            PRINTF_ERR("A: %p, W: %p, OSerial: %p\n", A, W, OSerial);
            printResults();
            PRINTF_ERR("Exiting...\n");
            while(1);
        }

        ++testN;

        PRINTF_DBG("Memory allocation successful!\n");

        PRINTF_DBG("\nInitializing matrices...\n");
        for(int i = 0; i < n * (n / 32); ++i){
            A[i] = i + 1;
        }
        for(int i = 0; i < n * (n / 32); ++i){
            W[i] = i;
        }
        memset(OSerial, 0, n * (n / 32) * sizeof(uint32_t));

        sizes[timesIndex] = n;
        times[timesIndex++] = get_absolute_time();

        PRINTF_DBG("\nMatrice A:\n");
        PROFILING_ENV_EXCLUSION(printIntBMatrixN(A, 2, 2, n, n);)

        PRINTF_DBG("\nMatrice W:\n");
        PROFILING_ENV_EXCLUSION(printIntBMatrixN(W, 2, 2, n, n);)

        PRINTF_DBG("\nMatrice OSerial (inizializzata a zero):\n");
        PROFILING_ENV_EXCLUSION(printIntBMatrixN(OSerial, 2, 2, n, n);)

        fastBinaryMatrixMul(A, W, OSerial, signCmp, n, n, n);

        sizes[timesIndex] = n;
        times[timesIndex++] = get_absolute_time();


        free(A);
        free(W);
        free(OSerial);

        sizes[timesIndex] = n;
        times[timesIndex++] = get_absolute_time();
    }
    PRINTF_LOG("\nAll tests completed!\n");
    printResults();

    while (true) {
        // printf("Hello, world!\n");
        sleep_ms(1000);
        pico_set_led(0);
        sleep_ms(1000);
        pico_set_led(1);
    }
}
