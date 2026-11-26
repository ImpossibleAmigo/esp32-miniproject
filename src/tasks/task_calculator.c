#include "tasks/task_calculator.h"
#include "drivers/encoder.h"
#include "utils/math_utils.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <inttypes.h>
#include <stdio.h>

typedef enum {
    CALC_OP1 = 0,
    CALC_OPERATOR,
    CALC_OP2,
} calc_state_t;

static const char OPS[4] = {'+', '-', '*', '/'};

void task_calculator_run(void)
{
    esp_log_level_set("*", ESP_LOG_WARN);
    ESP_ERROR_CHECK(encoder_init());
    QueueHandle_t q = encoder_get_queue();

    calc_state_t state = CALC_OP1;
    int32_t op1 = 0;
    int32_t op2 = 0;
    int op_idx = 0;
    int token_len = 0;
    char buf[24];

    printf("\n=== ENCODER CALCULATOR ===\n");
    printf("Rotate to edit, short press to confirm, long press to clear.\n\n");
    console_overwrite_token(&token_len, "0");

    encoder_event_t evt;
    for (;;) {
        if (xQueueReceive(q, &evt, portMAX_DELAY) != pdTRUE) continue;

        if (evt == ENC_EVT_BTN_LONG) {
            printf(" [CLR]\n");
            fflush(stdout);
            state = CALC_OP1;
            op1 = 0;
            op2 = 0;
            op_idx = 0;
            token_len = 0;
            console_overwrite_token(&token_len, "0");
            continue;
        }

        int delta = (evt == ENC_EVT_CW) ? 1 : ((evt == ENC_EVT_CCW) ? -1 : 0);

        switch (state) {
            case CALC_OP1:
                if (delta != 0) {
                    op1 += delta;
                    snprintf(buf, sizeof(buf), "%" PRId32, op1);
                    console_overwrite_token(&token_len, buf);
                } else if (evt == ENC_EVT_BTN_SHORT) {
                    fputs(" ", stdout);
                    fflush(stdout);
                    token_len = 0;
                    op_idx = 0;
                    snprintf(buf, sizeof(buf), "%c ", OPS[op_idx]);
                    console_overwrite_token(&token_len, buf);
                    state = CALC_OPERATOR;
                }
                break;

            case CALC_OPERATOR:
                if (delta != 0) {
                    op_idx = (op_idx + delta + 4) % 4;
                    snprintf(buf, sizeof(buf), "%c ", OPS[op_idx]);
                    console_overwrite_token(&token_len, buf);
                } else if (evt == ENC_EVT_BTN_SHORT) {
                    token_len = 0;
                    op2 = 0;
                    console_overwrite_token(&token_len, "0");
                    state = CALC_OP2;
                }
                break;

            case CALC_OP2:
                if (delta != 0) {
                    op2 += delta;
                    snprintf(buf, sizeof(buf), "%" PRId32, op2);
                    console_overwrite_token(&token_len, buf);
                } else if (evt == ENC_EVT_BTN_SHORT) {
                    switch (OPS[op_idx]) {
                        case '+':
                            printf(" = %" PRId64 "\n", (int64_t)op1 + op2);
                            break;
                        case '-':
                            printf(" = %" PRId64 "\n", (int64_t)op1 - op2);
                            break;
                        case '*':
                            printf(" = %" PRId64 "\n", (int64_t)op1 * op2);
                            break;
                        case '/':
                            if (op2 == 0) {
                                printf(" = ERR (division by zero)\n");
                            } else {
                                printf(" = %.2f\n", (double)op1 / (double)op2);
                            }
                            break;
                    }
                    fflush(stdout);

                    /* Автоматичний старт наступного обчислення */
                    state = CALC_OP1;
                    op1 = 0;
                    op2 = 0;
                    op_idx = 0;
                    token_len = 0;
                    console_overwrite_token(&token_len, "0");
                }
                break;
        }
    }
}