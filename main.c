#include <stdio.h>
#include "pico/stdlib.h"
#include <BinaryMatMul.h>

#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"

void set_sys_clock_40mhz() {
    // Cambia per un momento la sorgente di clock principale
    clock_configure(clk_sys,
        CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,   // Usa clk_ref (XOSC)
        0,                                        // Ignora auxsrc
        12 * MHZ,
        12 * MHZ);

    // Disabilita PLL SYS
    pll_deinit(pll_sys);

    // Reconfigura PLL SYS per ottenere 40 MHz da XOSC a 12 MHz
    // mol = 40, refdiv = 1 → VCO = 480 MHz → post_div1 = 12, post_div2 = 1 → 480 / 12 = 40 MHz
    pll_init(pll_sys, 1, 40 * MHZ, 12, 1);

    // Configura clk_sys a partire da PLL_SYS
    clock_configure(clk_sys,
        CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
        CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
        40 * MHZ, 40 * MHZ);
}


int main(){
    // set_sys_clock_40mhz();

    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
