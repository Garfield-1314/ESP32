#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* G-Code Parameter Structure
   Stores parsed parameters from a G-code line.
*/
typedef struct {
  char cmd_type;  // 'G' or 'M' (or others)
  int cmd_code;   // The number after the letter (e.g., 1 for G1)

  // Parameters A-Z.
  // We can store them in an array float params[26] where index 0 is 'A', 1 is
  // 'B'...
  float params[26];
  bool param_seen[26];  // True if the parameter was present
} GCode_t;

// Function pointer for command handlers
typedef void (*command_handler_t)(GCode_t *gcode);

/**
 * @brief Initialize the command module.
 */
void command_init(void);

/**
 * @brief Register a command handler.
 *
 * @param type Command type character (e.g., 'G', 'M')
 * @param code Command code (e.g., 1, 28)
 * @param handler The function to call when this command is received
 */
void command_register(char type, int code, command_handler_t handler);

/**
 * @brief Feed data into the command processor.
 *        Called by the communication interface (e.g., USB serial).
 *
 * @param data Pointer to data buffer
 * @param len Length of data
 */
void command_receive(uint8_t *data, size_t len);

/**
 * @brief Send a response string back to the host (via USB).
 *        Wraps the USB write function.
 *
 * @param str The string to send.
 */
void command_print(const char *str);

#endif  // __COMMAND_H__