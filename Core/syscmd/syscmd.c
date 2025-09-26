#include "cmd_cli.h"

#include "embedded_cli.h"


static void system_reboot_cmd(EmbeddedCli *cli, char *args, void *context)
{
    printf("not suport !\n");

    return;
}



static int sys_cmd_bind_init()
{
    int ret = 0; 

    CliCommandBinding cmd[] = {
        {
            .name = "reboot",
            .help = "Reboot device",
            .tokenizeArgs = true,
            .context = NULL,
            .binding = system_reboot_cmd
        },

    };

    ret = cli_cmd_register(CMD_MODE_DEF, cmd, sizeof(cmd)/sizeof(cmd[0]));
    return ret;
}
