#include "app_config.h"
#include "tasks/task_calculator.h"
#include "tasks/task_encoder_servo.h"
#include "tasks/task_ldr_servo.h"
#include "tasks/task_safe.h"

void app_main(void)
{
#if ACTIVE_TASK == TASK_LDR_SERVO
    task_ldr_servo_run();
#elif ACTIVE_TASK == TASK_ENCODER_SERVO
    task_encoder_servo_run();
#elif ACTIVE_TASK == TASK_SAFE
    task_safe_run();
#elif ACTIVE_TASK == TASK_CALCULATOR
    task_calculator_run();
#else
#error "Invalid ACTIVE_TASK selected in app_config.h"
#endif
}