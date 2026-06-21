#include "Command/inc/command.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Algorithm/inc/Mecanum.h"
#include "Algorithm/inc/pid.h"
#include "device/inc/imu_hal.h"
#include "device/inc/motor.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "tusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui/inc/cmd_page.h"
#include "ui/inc/num_show_page.h"

extern motor_status_t motor_status;
extern Mecanum_Wheel_Speed_t target_wheel_speed;

static const char *TAG = "COMMAND";

// --- Private Types ---
typedef struct CommandNode {
  char type;
  int code;
  command_handler_t handler;
  struct CommandNode *next;
} CommandNode;

// --- Private Variables ---
static CommandNode *command_list_head = NULL;
static char line_buffer[256];
static int line_pos = 0;

// --- Helper Functions ---

// Simple parser for float
static float parse_float(char **ptr) {
  char *str = *ptr;
  char *endptr;
  float val = strtof(str, &endptr);
  if (str == endptr) {
    return 0.0f;  // No conversion performed
  }
  *ptr = endptr;
  return val;
}

static void parse_and_execute(char *line) {
  // ESP_LOGI(TAG, "Parsing: %s", line);
  // Skip leading spaces
  while (*line && isspace((int)*line)) line++;
  if (!*line) return;  // Empty line

  // 处理 OpenMV 初始化完成信号
  if (strncmp(line, "init_done", 9) == 0) {
    num_show_page_set_init_done();
    return;
  }

  GCode_t gcode = {0};

  // REQUIRE 'C' at start for car commands to avoid conflict with robot arm
  if (toupper((int)*line) != 'C') {
    return;
  }
  line++;

  // Parse Command Type (e.g. G)
  gcode.cmd_type = toupper((int)*line);
  if (gcode.cmd_type) line++;

  // Parse Command Code (e.g. 1)
  if (isdigit((int)*line)) {
    gcode.cmd_code = (int)strtol(line, &line, 10);
  } else {
    // Handle case where no number follows
    gcode.cmd_code = -1;
  }

  // Parse Parameters
  while (*line) {
    while (*line && isspace((int)*line)) line++;
    if (!*line) break;

    // Optional 'C' before parameter letter (flexible)
    if (toupper((int)*line) == 'C') {
      line++;
    }

    char param = toupper((int)*line);
    if (param >= 'A' && param <= 'Z') {
      line++;
      float val = parse_float(&line);
      gcode.params[param - 'A'] = val;
      gcode.param_seen[param - 'A'] = true;
    } else {
      // Unknown or comment or checksum (start with *)
      if (*line == ';' || *line == '(') break;  // Comment
      line++;
    }
  }

  // Find Handler
  CommandNode *curr = command_list_head;
  bool found = false;
  while (curr) {
    if (curr->type == gcode.cmd_type && curr->code == gcode.cmd_code) {
      curr->handler(&gcode);
      found = true;
      break;
    }
    curr = curr->next;
  }

  if (found) {
    // Optionally send OK, or let handler do it
    // command_print("OK\r\n");
  }
  // 未知命令静默丢弃，不回复（共享总线，避免干扰）
}

// --- Public APIs ---

// --- Command Handlers ---

extern Mecanum_Controller_t mecanum_ctrl;
extern motor_status_t motor_status;

static void handler_G1(GCode_t *gcode) {
  char msg[64];
  float x = 0;
  float y = 0;
  float theta = 0;

  if (gcode->param_seen['X' - 'A']) {
    x = gcode->params['X' - 'A'];
  }
  if (gcode->param_seen['Y' - 'A']) {
    y = gcode->params['Y' - 'A'];
  }
  if (gcode->param_seen['Z' - 'A']) {
    theta = gcode->params['Z' - 'A'];
  }

  // Handle PID Scaling 'S'
  if (gcode->param_seen['S' - 'A']) {
    float s_val = gcode->params['S' - 'A'];
    if (s_val > 0.0f) {
      for (int i = 0; i < 4; i++) {
        // Scale from the base configuration values (inc_kp holds the speed
        // PID parameters)
        motor_status.pid_controller[i].Kp = motor_status.inc_kp[i] * s_val;
        motor_status.pid_controller[i].Ki = motor_status.inc_ki[i] * s_val;
        motor_status.pid_controller[i].Kd = motor_status.inc_kd[i] * s_val;
      }
    }
  }

  // Update target pose based on commanded velocities
  mecanum_ctrl.target_pose.x = x;
  mecanum_ctrl.target_pose.y = -y;
  mecanum_ctrl.target_pose.theta = theta;

  snprintf(msg, sizeof(msg), "CCG1 Set D: CX%.1f CY%.1f CZ%.1f CS%.2f\n", x, y,
           theta,
           gcode->param_seen['S' - 'A'] ? gcode->params['S' - 'A'] : 1.0f);
  command_print(msg);
}

static void handler_M114(GCode_t *gcode) {
  char msg[128];
  snprintf(msg, sizeof(msg), "CV: %.1f %.1f %.1f %.1f\n",
           motor_status.speed_mm_s[0], motor_status.speed_mm_s[1],
           motor_status.speed_mm_s[2], motor_status.speed_mm_s[3]);
  command_print(msg);

  snprintf(msg, sizeof(msg), "CD: %.3f %.3f %.3f %.3f\n",
           motor_status.distance_m[0], motor_status.distance_m[1],
           motor_status.distance_m[2], motor_status.distance_m[3]);
  command_print(msg);

  snprintf(msg, sizeof(msg), "CP: CX%.2f CY%.2f CZ%.2f\n",
           mecanum_ctrl.current_pose.x, mecanum_ctrl.current_pose.y,
           mecanum_ctrl.current_pose.theta);
  command_print(msg);

  snprintf(msg, sizeof(msg), "CPID_OUT: %f %f %f %f\n",
           motor_status.pid_output[0], motor_status.pid_output[1],
           motor_status.pid_output[2], motor_status.pid_output[3]);
  command_print(msg);
}

static void handler_F1(GCode_t *gcode) {
  char msg[128];
  // F1 A<speed1> B<speed2> C<speed3> D<speed4>
  // A: Front Left (Motor 0)
  // B: Front Right (Motor 1)
  // C: Rear Left (Motor 2)
  // D: Rear Right (Motor 3)

  bool updated = false;

  if (gcode->param_seen['A' - 'A']) {
    motor_status.target_speed_mm_s[0] = gcode->params['A' - 'A'];
    updated = true;
  }
  if (gcode->param_seen['B' - 'A']) {
    motor_status.target_speed_mm_s[1] = gcode->params['B' - 'A'];
    updated = true;
  }
  if (gcode->param_seen['C' - 'A']) {
    motor_status.target_speed_mm_s[2] = gcode->params['C' - 'A'];
    updated = true;
  }
  if (gcode->param_seen['D' - 'A']) {
    motor_status.target_speed_mm_s[3] = gcode->params['D' - 'A'];
    updated = true;
  }

  if (updated) {
    snprintf(
        msg, sizeof(msg), "CF1 Set Wheels: CA%.1f CB%.1f CC%.1f CD%.1f\n",
        motor_status.target_speed_mm_s[0], motor_status.target_speed_mm_s[1],
        motor_status.target_speed_mm_s[2], motor_status.target_speed_mm_s[3]);
    command_print(msg);
  } else {
    command_print("CF1: No params (CA,CB,CC,CD)\r\n");
  }
}

static void handler_T1(GCode_t *gcode) {
  // command_print("T command received\r\n");

  int vals[6] = {0, 0, 0, 0, 0, 0};
  for (int i = 0; i < 6; ++i) {
    if (gcode->param_seen['A' + i - 'A']) {
      vals[i] = (int)gcode->params['A' + i - 'A'];
    }
  }
  num_show_page_update(vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]);
}

#define COMMAND_UART_PORT UART_NUM_0
#define COMMAND_UART_BAUD_RATE 115200
#define COMMAND_UART_RX_BUF_SIZE 1024

static void command_uart_task(void *pvParameters) {
  uint8_t *data = (uint8_t *)malloc(COMMAND_UART_RX_BUF_SIZE);
  while (1) {
    // Read data from the UART
    int len = uart_read_bytes(COMMAND_UART_PORT, data, COMMAND_UART_RX_BUF_SIZE,
                              20 / portTICK_PERIOD_MS);
    if (len > 0) {
      // printf("Received %d bytes from UART\n", len);
      command_receive(data, len);
    }
  }
  free(data);
  vTaskDelete(NULL);
}

static void command_uart_init(void) {
  uart_config_t uart_config = {
      .baud_rate = COMMAND_UART_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  // Install UART driver
  uart_driver_install(COMMAND_UART_PORT, COMMAND_UART_RX_BUF_SIZE * 2, 256, 0,
                      NULL, 0);
  uart_param_config(COMMAND_UART_PORT, &uart_config);
  // Set UART0 pins: TXD0=GPIO43, RXD0=GPIO44
  uart_set_pin(COMMAND_UART_PORT, 43, 44, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);

  // Create a task to handle UART reading
  xTaskCreate(command_uart_task, "command_uart_task", 4096, NULL, 3, NULL);
}

void command_init(void) {
  // Register default commands
  command_uart_init();
  command_register('G', 1, handler_G1);
  command_register('M', 114, handler_M114);
  command_register('F', 1, handler_F1);
  command_register('T', 1, handler_T1);
}

void command_register(char type, int code, command_handler_t handler) {
  CommandNode *node = (CommandNode *)malloc(sizeof(CommandNode));
  if (node) {
    node->type = type;
    node->code = code;
    node->handler = handler;
    node->next = command_list_head;
    command_list_head = node;
  }
}

void command_receive(uint8_t *data, size_t len) {
  cmd_page_add_data((const char *)data, len);
  for (size_t i = 0; i < len; i++) {
    char c = (char)data[i];
    if (c == '\r' || c == '\n') {
      if (line_pos > 0) {
        line_buffer[line_pos] = 0;  // Null terminate
        parse_and_execute(line_buffer);
        line_pos = 0;
      }
    } else {
      if (line_pos < sizeof(line_buffer) - 1) {
        line_buffer[line_pos++] = c;
      } else {
        // Buffer overflow: discard or reset
        line_pos = 0;
      }
    }
  }
}

void command_print(const char *str) {
  /* Send reply back to OpenMV via UART (non-blocking, hardware FIFO) */
  uart_write_bytes(COMMAND_UART_PORT, str, strlen(str));
  /* Also send via CDC ACM for USB serial debugging */
  tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, (const uint8_t *)str, strlen(str));
  tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
  /* Show on LCD terminal */
  cmd_page_add_data(str, strlen(str));
}
