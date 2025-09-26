#ifndef __CMD_CLI_H__
#define __CMD_CLI_H__


#include "embedded_cli.h"


typedef enum
{
    CMD_MODE_DEF = 1,
    CMD_MODE_DBG,
    CMD_MODE_NUM
} cmd_mode_e;


int cli_cmd_register(cmd_mode_e cmd_mode, CliCommandBinding *cmd_info_arry);
int cli_cmd_new_mode(cmd_mode_e cmd_mode, char *invitation);
QueueHandle_t cmd_cli_queue_get(void);

#endif