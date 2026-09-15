#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app_factory_battery.h"
#include "bc_pmic.h"

static unsigned checks, depth, adc_calls, ship_calls, sent_level, status_sent;
static enum pmic_charge_status fixture_charge;
static uint8_t fixture_percent;
static bool interrupt_charge, observed_charge_cycle, reenter;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#x); return 1; } } while (0)
#define BC_LOG_INFO(...) ((void)0)

void test_task_enter(void) { ++depth; }
void test_task_exit(void) { assert(depth); --depth; }
enum pmic_charge_status bc_pmic_get_charge_status(void) {
    assert(!depth);
    return fixture_charge;
}
uint8_t bc_pmic_get_vbat_percen(void) {
    assert(!depth); ++adc_calls;
    if (reenter) {
        unsigned before = adc_calls;
        reenter = false;
        assert(app_factory_battery_percent() == 255);
        assert(adc_calls == before);
    }
    if (interrupt_charge) { interrupt_charge = false; fixture_charge = PMIC_CHARGED_ING; }
    if (observed_charge_cycle) {
        observed_charge_cycle = false;
        /* A PMIC-observed transition invalidates a batch even if the two
         * sampled states are equal after a rapid attach/detach cycle. */
        app_factory_battery_charge_changed();
    }
    return fixture_percent;
}
static void app_pmic_timer_not_charge_time_update(void) {}
static void app_pmic_timer_chargeing_time_update(void) {}
void app_package_precent_up(uint8_t p) { sent_level = p; }
void app_package_precent_status_up(uint16_t s) { status_sent = s; }
void app_rtc_ushut_down_time_record(void) {}
void bc_pmic_set_shipmode(void) { ++ship_calls; }

/* Includes the actual prepared getvpct wrappers and PMIC handler, not a
 * parallel model of their low-battery and notification decisions. */
static enum pmic_charge_status pmic_state, pmic_state_check;
static uint8_t pmic_percent_low_count, precent;
static bool led_flag;
#include "factory_pmic.inc"

static void new_epoch(void) {
    fixture_charge = PMIC_CHARGED_ING; fixture_percent = 50;
    (void)getvpct(); fixture_charge = PMIC_CHARGED_NOT;
}

int main(void) {
    /* Exact reported reproductions: full battery followed by a lower input. */
    for (unsigned p = 0; p <= 100; ++p) {
        new_epoch(); fixture_percent = 100;
        for (unsigned i=0;i<10;++i) CHECK(getvpct()==100);
        fixture_percent = p;
        for (unsigned i=0;i<10;++i) (void)getvpct();
        CHECK(getvpct()==p);
    }
    new_epoch(); fixture_percent=20; CHECK(getvpct()==20);
    fixture_percent=80;
    for (unsigned i=0;i<20;++i) (void)getvpct();
    CHECK(getvpct()==80);
    new_epoch(); fixture_percent=100; CHECK(getvpct()==100);
    fixture_percent=90; CHECK(getvpct()==95);
    fixture_percent=0; CHECK(getvpct()==90); /* Last element is minimum. */

    fixture_charge=PMIC_CHARGED_ING; fixture_percent=10; CHECK(getvpct()==10);
    fixture_charge=PMIC_CHARGED_OVER; fixture_percent=40; CHECK(getvpct()==40);
    fixture_charge=PMIC_CHARGED_NOT; fixture_percent=70; CHECK(getvpct()==70);
    fixture_percent=255; CHECK(getvpct()==255);
    fixture_percent=70; CHECK(getvpct()==70);
    reenter=true; CHECK(getvpct()==70); CHECK(!depth);
    fixture_percent=5; interrupt_charge=true; CHECK(getvpct()==255);
    fixture_percent=85; CHECK(getvpct()==85);
    fixture_charge=(enum pmic_charge_status)255;
    unsigned prior=adc_calls; CHECK(getvpct()==255); CHECK(adc_calls==prior);
    fixture_charge=PMIC_CHARGED_NOT; fixture_percent=30; CHECK(getvpct()==30);

    /* Charge-complete does not call getvpct in the actual factory handler.
     * An observed complete interval must still retire off-charger history. */
    new_epoch(); fixture_percent=20; CHECK(getvpct()==20);
    pmic_state_check=PMIC_CHARGED_NOT;
    fixture_charge=PMIC_CHARGED_OVER; prior=adc_calls; app_pmic_handler();
    CHECK(adc_calls==prior && status_sent==2);
    fixture_charge=PMIC_CHARGED_NOT; fixture_percent=80;
    CHECK(getvpct()==80);
    observed_charge_cycle=true; CHECK(getvpct()==255);
    fixture_percent=40; CHECK(getvpct()==40);

    /* Invalid reads must break a sequence of low shutdown votes. */
    new_epoch(); fixture_percent=0; ship_calls=0; pmic_percent_low_count=0;
    for(unsigned i=0;i<3;++i) app_pmic_handler();
    CHECK(ship_calls==0 && pmic_percent_low_count==3 && sent_level==0);
    fixture_percent=255; app_pmic_handler();
    CHECK(ship_calls==0 && pmic_percent_low_count==0 && sent_level==255);
    fixture_percent=0;
    for(unsigned i=0;i<3;++i) app_pmic_handler();
    CHECK(ship_calls==0);
    app_pmic_handler(); CHECK(ship_calls==1);

    /* A charging interval also breaks consecutive off-charger low votes. */
    pmic_percent_low_count=3; ship_calls=0;
    fixture_charge=PMIC_CHARGED_ING; app_pmic_handler();
    CHECK(pmic_percent_low_count==0 && ship_calls==0 && status_sent==1);
    fixture_charge=PMIC_CHARGED_NOT; app_pmic_handler();
    CHECK(ship_calls==0 && pmic_percent_low_count==1 && status_sent==0);
    CHECK(!depth);
    printf("PASS P08 battery integration: %u checks; constant levels, recovery, transitions, errors, reentrancy, notifications and shutdown\n",checks);
    return 0;
}
