#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>

#include "current.h"

void current_init(void)
{
	rcc_periph_clock_enable(RCC_ADC1);
	rcc_periph_clock_enable(RCC_ADC2);
	rcc_periph_clock_enable(RCC_ADC3);

	rcc_periph_clock_enable(RCC_GPIOC);

	gpio_mode_setup(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO2 | GPIO3);

	adc_off(ADC1);
	adc_off(ADC2);
	adc_off(ADC3);

	adc_set_clk_prescale(ADC_CCR_ADCPRE_BY2);

	adc_enable_scan_mode(ADC1);
    adc_enable_scan_mode(ADC2);
    adc_enable_scan_mode(ADC3);

    adc_set_single_conversion_mode(ADC1);
    adc_set_single_conversion_mode(ADC2);
    adc_set_single_conversion_mode(ADC3);

    adc_enable_external_trigger_injected(ADC1, ADC_CR2_JEXTSEL_TIM5_CC4, ADC_CR2_JEXTEN_FALLING_EDGE); // in center of pwm=low
    adc_enable_external_trigger_injected(ADC2, ADC_CR2_JEXTSEL_TIM5_CC4, ADC_CR2_JEXTEN_FALLING_EDGE); // in center of pwm=low
    adc_enable_external_trigger_injected(ADC3, ADC_CR2_JEXTSEL_TIM5_CC4, ADC_CR2_JEXTEN_FALLING_EDGE); // in center of pwm=low

    adc_set_right_aligned(ADC1);
    adc_set_right_aligned(ADC2);
    adc_set_right_aligned(ADC3);

	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);
	adc_set_sample_time_on_all_channels(ADC2, ADC_SMPR_SMP_3CYC);
	adc_set_sample_time_on_all_channels(ADC3, ADC_SMPR_SMP_15CYC);

	adc_set_resolution(ADC1, ADC_CR1_RES_12BIT);
    adc_set_resolution(ADC2, ADC_CR1_RES_12BIT);
    adc_set_resolution(ADC3, ADC_CR1_RES_12BIT);

    uint8_t adc1_channel = ADC_CHANNEL10;
    uint8_t adc2_channel = ADC_CHANNEL11;
    uint8_t adc3_channel = ADC_CHANNEL12;

	adc_set_injected_sequence(ADC1, 1, &adc1_channel);
	adc_set_injected_sequence(ADC2, 1, &adc2_channel);
	adc_set_injected_sequence(ADC3, 1, &adc3_channel);

    adc_enable_eoc_interrupt_injected(ADC1);
	adc_enable_eoc_interrupt_injected(ADC2);
	adc_enable_eoc_interrupt_injected(ADC3);

	adc_power_on(ADC1);
	adc_power_on(ADC2);
	adc_power_on(ADC3);

	nvic_enable_irq(NVIC_ADC_IRQ);

	/* Wait for ADC starting up. */
    for (int i = 0; i < 800000; i++)    /* Wait a bit. */
        __asm__("nop");

}

static uint16_t _adc_value[3] = { 0,0,0 };
static uint16_t _adc_counter[3] = { 0,0,0 };

void adc_finish(uint16_t values[]);

void adc_isr(void)
{
    if (adc_eoc_injected(ADC1))
    {
        _adc_value[0] += adc_read_injected(ADC1, 1);
        _adc_counter[0]++;

        ADC_SR(ADC1) = ~ADC_SR_JEOC;
    }

    if (adc_eoc_injected(ADC2))
    {
        _adc_value[1] += adc_read_injected(ADC2, 1);
        _adc_counter[1]++;

        ADC_SR(ADC2) = ~ADC_SR_JEOC;
    }

    if (adc_eoc_injected(ADC3))
    {
        _adc_value[2] += adc_read_injected(ADC3, 1);
        _adc_counter[2]++;

        ADC_SR(ADC3) = ~ADC_SR_JEOC;
    }

    if ((_adc_counter[0] >= 16) && (_adc_counter[1] >= 16) && (_adc_counter[2] >= 16))
    {
        // measurement cycle end
        adc_finish(_adc_value);
        _adc_counter[0] = _adc_counter[1] = _adc_counter[2] = 0;
        _adc_value[0] = _adc_value[1] = _adc_value[2] = 0;
    }
}
