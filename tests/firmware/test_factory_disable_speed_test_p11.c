/* Generated callback under test; no hardware, audio, or task APIs are linked. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct app_cmd_package {
    uint8_t first, frame_id, cmd, subcmd;
    uint8_t data[250];
    uint8_t length;
};
static unsigned sends;
static uint8_t answer[5];
static void app_package_send_enqueue(struct app_cmd_package *packet, unsigned length)
{
    assert(length == sizeof(answer));
    memcpy(answer, packet, length);
    ++sends;
}
#include "speed-reject.inc"

int main(void)
{
    unsigned checks = 0;
    assert(app_test_ble_speed_callback(NULL) == 0 && sends == 0);
    for (unsigned command = 0; command < 256; ++command) {
        for (unsigned length = 0; length < 256; ++length) {
            for (unsigned control = 0; control < 256; ++control) {
                struct app_cmd_package request, saved;
                memset(&request, 0xa5, sizeof(request));
                request.first = 0; request.frame_id = 0x6b;
                request.cmd = (uint8_t)command; request.subcmd = 0x3b;
                request.length = (uint8_t)length;
                request.data[0] = (uint8_t)control;
                request.data[1] = 255; /* Former unchecked packet length. */
                saved = request;
                unsigned before = sends;
                assert(app_test_ble_speed_callback(&request) == 0);
                assert(memcmp(&request, &saved, sizeof(request)) == 0);
                assert(sends == before + (length >= 4));
                if (length >= 4) {
                    assert(memcmp(answer, &request, 4) == 0);
                    assert(answer[4] == 0);
                }
                ++checks;
            }
        }
    }
    printf("P11 speed-test rejection: %u request combinations passed\n", checks);
    return 0;
}
