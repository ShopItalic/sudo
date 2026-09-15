#include <stdint.h>
struct app_cmd_package;
void app_package_send_enqueue(struct app_cmd_package *packet, uint8_t length);
