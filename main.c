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

#define ITERATIONS 5
#define TIMES_NUM 9

absolute_time_t times[TIMES_NUM * ITERATIONS] = {0};
absolute_time_t startTime = 0;
uint32_t sizes[ITERATIONS] = {0};
uint32_t sizeIndex = 0;
uint32_t timesIndex = 0;
uint32_t testN = 0;
absolute_time_t transposeTimes[ITERATIONS] = {0};
uint32_t transposeTimesIndex = 0;
uint32_t computeTimes[ITERATIONS] = {0};
uint32_t computeTimesIndex = 0;

char labels[TIMES_NUM][25] = {
    "Allocazione",
    "Popolazione",
    "Caricamento",
    "Settaggio",
    "Inizializzazione",
    "ComputazioneBTPU",
    "LetturaRisultato",
    "ComputazioneSerialeFast",
    "freeMemory"
};

void printResults(){
    printf("size(bit),");
    for(int label = 0; label < TIMES_NUM - 1; ++label){
        printf("%s,", labels[label]);
    }
    printf("%s,transposeTime,computeTime,platform\n", labels[TIMES_NUM - 1]);
    

    absolute_time_t toPrint = 0;
    for(int size = 0; size < sizeIndex; ++size){
        printf("%d,", sizes[size]);
        for (int time = 0; time < TIMES_NUM - 1; ++time) {
            if(time == 0 && size == 0){
                toPrint = absolute_time_diff_us(startTime, times[0]);
            } else {
                toPrint = absolute_time_diff_us(times[size * TIMES_NUM + time - 1], times[size * TIMES_NUM + time]);
            }
            printf("%llu,", toPrint);
        }
        toPrint = absolute_time_diff_us(times[size * TIMES_NUM + TIMES_NUM - 2], times[size * TIMES_NUM + TIMES_NUM - 1]);
        #if  defined(MATH_COMPUTE) && defined(TRANSPOSE_COMPUTE)
            printf("%llu,%llu,%llu,RP2350\n", toPrint, transposeTimes[size], computeTimes[size]);
        #elif !defined(MATH_COMPUTE) && defined(TRANSPOSE_COMPUTE) 
            printf("%llu,%llu,%llu,RP2350-NOMATH\n", toPrint, transposeTimes[size], computeTimes[size]);
        #elif !defined(TRANSPOSE_COMPUTE) && defined(MATH_COMPUTE)
            printf("%llu,%llu,%llu,RP2350-NOTRANSPOSE\n", toPrint, transposeTimes[size], computeTimes[size]);
        #else
            printf("%llu,%llu,%llu,RP2350-ONLYMEMORY\n", toPrint, transposeTimes[size], computeTimes[size]);
        #endif
    }
}

float profileBinaryMul(uint32_t N){
    absolute_time_t acc = 0;
    uint32_t a = rand() % N;
    uint32_t b = rand() % N;
    volatile uint32_t* aPtr = &a;
    volatile uint32_t* bPtr = &b;

    absolute_time_t startTime = get_absolute_time();
    for(int i = 0; i < N; ++i){
        uint32_t result = binaryMul(*aPtr, *bPtr);
    }
    absolute_time_t endTime = get_absolute_time();
    acc += absolute_time_diff_us(startTime, endTime);
    return acc/(float)N;
}

absolute_time_t profileTransposeBlock(uint32_t N){
    absolute_time_t acc = 0;
    for(int i = 0; i < N; ++i){
        BinaryFragment_t a;
        absolute_time_t startTime = get_absolute_time();
        transposeBinaryFragment(a, a);
        absolute_time_t endTime = get_absolute_time();
        acc += absolute_time_diff_us(startTime, endTime);
    }
    return acc/N;
}

int main(){
    
    pico_led_init();
    
    set_sys_clock_40mhz();
    
    stdio_init_all();
    printf("Compiled at: %s %s\n", __DATE__, __TIME__);
    printf("Running at %u KHz\n", frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS));

    printf("Binary Matrix wall time: %f us\n", profileBinaryMul(10000000));
    printf("Transpose block wall time: %llu us\n", profileTransposeBlock(1000));

    int bn = 0;
    const uint32_t signCmp = 48;

    BTPU0RegFile = (BTPURegFile_t*)malloc(sizeof(BTPURegFile_t));
    BTPU0_W_MEMORY = (BinaryFragment_t*)malloc(1024 * sizeof(BinaryFragment_t));
    BTPU0_IO0_MEMORY = (BinaryFragment_t*)malloc(1024 * sizeof(BinaryFragment_t));
    BTPU0_IO1_MEMORY = (BinaryFragment_t*)malloc(1024 * sizeof(BinaryFragment_t));

    if(!BTPU0RegFile || !BTPU0_W_MEMORY || !BTPU0_IO0_MEMORY || !BTPU0_IO1_MEMORY){
        PRINTF_ERR("[ERROR]: Memory allocation failed for BTPU structures!\n");
        // PRINTF_ERR("BTPU0RegFile: %p, BTPU0_W_MEMORY: %p, BTPU0_IO0_MEMORY: %p, BTPU0_IO1_MEMORY: %p\n", 
        //            BTPU0RegFile, BTPU0_W_MEMORY, BTPU0_IO0_MEMORY, BTPU0_IO1_MEMORY);
        return -1;
    }

    startTime = get_absolute_time();

    for(int i = 0, n = 32; i < ITERATIONS; ++i, n *= 2){
        bn = n / 32;
        
        BinaryMatrix_t A = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));         //768 byte
        BinaryMatrix_t W = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));         // 1.536 kB
        BinaryMatrix_t OSerial = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));   // 1 kB

        times[timesIndex++] = get_absolute_time();

        if(!A || !W || !OSerial){
            PRINTF_ERR("[ERROR]: Memory allocation failed -> N: %d\n", n);
            // PRINTF_ERR("A: %p, W: %p, OSerial: %p\n", A, W, OSerial);
            printResults();
            PRINTF_ERR("Exiting...\n");
            while(1);
        }

        sizes[sizeIndex++] = n;
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

        times[timesIndex++] = get_absolute_time(); // Popolazione

        loadBinaryMatrixToFragments(A, BTPU0_IO0_MEMORY, n, n);
        loadBinaryMatrixToFragments(W, BTPU0_W_MEMORY, n, n);
        times[timesIndex++] = get_absolute_time(); // Caricamento

        btpuSetBlocks(BTPU0RegFile, bn, bn, bn);
        btpuSetAddrs(BTPU0RegFile, 0, 0, bn * bn); 
        times[timesIndex++] = get_absolute_time(); // Settaggio

        btpuStartBinaryMatrixMul(BTPU0RegFile, signCmp, true, true, BTPU_USE_MEMORY_0_CONFIG);
        times[timesIndex++] = get_absolute_time(); // Inizializzazione

        times[timesIndex++] = get_absolute_time(); // ComputazioneBTPU

        storeFramentsToBinaryMatrix(BTPU0_IO1_MEMORY + (bn * bn), OSerial, n, n);
        times[timesIndex++] = get_absolute_time(); // LetturaRisultato

        PRINTF_DBG("\nMatrice A:\n");
        PROFILING_ENV_EXCLUSION(printIntBMatrixN(A, 2, 2, n, n);)

        PRINTF_DBG("\nMatrice W:\n");
        PROFILING_ENV_EXCLUSION(printIntBMatrixN(W, 2, 2, n, n);)

        PRINTF_DBG("\nMatrice OSerial (inizializzata a zero):\n");
        PROFILING_ENV_EXCLUSION(printIntBMatrixN(OSerial, 2, 2, n, n);)

        fastBinaryMatrixMul(A, W, OSerial, signCmp, n, n, n);

        times[timesIndex++] = get_absolute_time();


        free(A);
        free(W);
        free(OSerial);

        A = NULL;
        W = NULL;
        OSerial = NULL;
        
        times[timesIndex++] = get_absolute_time();
    }

    free(BTPU0RegFile);
    free(BTPU0_W_MEMORY);
    free(BTPU0_IO0_MEMORY);
    free(BTPU0_IO1_MEMORY);
    BTPU0RegFile = NULL;
    BTPU0_W_MEMORY = NULL;
    BTPU0_IO0_MEMORY = NULL;
    BTPU0_IO1_MEMORY = NULL;



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
