#include "app_config.h"
#include "tasks/task_ldr_servo.h"

void app_main(void)
{
#if ACTIVE_TASK == TASK_LDR_SERVO
    task_ldr_servo_run();
#else
#error "Invalid ACTIVE_TASK selected in app_config.h"
#endif
}