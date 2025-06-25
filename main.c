#include <stdio.h>
#include "pico/stdlib.h"
#include <BinaryMatMul.h>

#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"

#define PLL_SYS_KHZ (133 * 1000)

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
    // pico_set_led(set_sys_clock_khz(40 * 1000, true));
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    clock_configure(clk_sys,
        CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,  // usa clk_ref direttamente
        0,                                       // auxsrc non usato
        12 * MHZ,
        40 * MHZ
    );  

    // Configura clk_peri a partire da PLL_SYS
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

int main(){
    pico_led_init();

    set_sys_clock_40mhz();

    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
        pico_set_led(0);
        sleep_ms(1000);
        pico_set_led(1);
    }
}
