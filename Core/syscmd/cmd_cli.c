#include "embedded_cli.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "cmd_cli.h"

#include "stdio.h"


#define CMD_CLI_TASK_NAME "cmd_cli_task"

#define CMD_CLI_BUFF_SIZE 256
#define CMD_CLI_REGISTE_NUM_MAX 16

#define CMD_CLI_QUEUE_NUM 128
#define CMD_CLI_QUEUE_UNIT_LEN sizeof(char)

static void __cmd_cli_main(void *argv);


#define CMD_ERR_LOG(fmt, ...) \
do  \
{   \
    printf("[Error]%s:%d :"fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__);\
} while (0)


// static EmbeddedCli *cur_cmd_cli = NULL;
static CLI_UINT cmd_cli_buffer[BYTES_TO_CLI_UINTS(CMD_CLI_BUFF_SIZE)];
static QueueHandle_t cmd_cli_queue_handle = NULL;


typedef struct 
{
    cmd_mode_e cmd_mode;
    EmbeddedCli *cli;
}cmd_cli_arry_t;

static cmd_cli_arry_t cmd_cli_arry[CMD_MODE_NUM] = {0};




QueueHandle_t cmd_cli_queue_get(void)
{
    return cmd_cli_queue_handle;
}

int cli_cmd_new_mode(cmd_mode_e cmd_mode, char *invitation)
{
    /* check */
    if ((invitation == NULL))
    {
        CMD_ERR_LOG("para err !");
        return -1;
    }

    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    config->cliBuffer           = cmd_cli_buffer;
    config->cliBufferSize       = CMD_CLI_BUFF_SIZE;
    // config->rxBufferSize = CLI_RX_BUFFER_SIZE;
    // config->cmdBufferSize = CLI_CMD_BUFFER_SIZE;
    // config->historyBufferSize = CLI_HISTORY_SIZE;
    config->maxBindingCount     = CMD_CLI_REGISTE_NUM_MAX;
    config->invitation          = invitation;

    // Create new CLI instance
    EmbeddedCli *tmp_cli = NULL;
    tmp_cli = embeddedCliNew(config);
    if (tmp_cli == NULL) 
    {
        CMD_ERR_LOG("embeddedCliNew err !");
        return -1;
    }

    cmd_cli_arry[cmd_mode-1].cmd_mode   = cmd_mode;
    cmd_cli_arry[cmd_mode-1].cli        = tmp_cli;

    // fixme! 这里可以添加默认的命令，比如exit, help, 
    return 0;
}


/* 单个注册 */
int cli_cmd_register(cmd_mode_e cmd_mode, CliCommandBinding *cmd_info)
{
    int i = 0;

    if (cmd_info == NULL)
    {
        printf("module[%d] register cli cmd err !\n", cmd_mode);
        return -1;
    }

    for (i = 0; i < CMD_MODE_NUM; i++)
    {
        if (cmd_cli_arry[i].cmd_mode != cmd_mode) 
            continue;
        
        embeddedCliAddBinding(cmd_cli_arry[i].cli, *cmd_info);
        break;
    }

    return 0;
}


/* cmd cli init */
// static int __cmd_cli_init(void)
// {
//     int ret = 0;
//     printf("cmd client init ...");
//     // fixme need fflush(stdout);

//     ret = cli_cmd_new_mode(CMD_MODE_DEF, "#");
//     if (ret == 0)
//         printf("\rcmd client init ok !\n");
//     else
//         printf("\rcmd client init fail !\n");

//     return ret;
// }


static void __cmd_cli_main(void *argv)
{
    char c = 0;

    cmd_cli_queue_handle = xQueueCreate(CMD_CLI_QUEUE_NUM, CMD_CLI_QUEUE_UNIT_LEN);
    if (cmd_cli_queue_handle == NULL)
    {
        CMD_ERR_LOG("Creat cmd cli queue error !");
        return;
    }

    /* 任务读队列 */
    while(1)
    {
        xQueueCRReceive( cmd_cli_queue_handle, &c, 20);
        vTaskDelay(50);
        // cur_cmd_cli 写入当前cmd cli
    }

    CMD_ERR_LOG("Cmd cli task exit !");
    return;
}


static TaskHandle_t syscmd_TaskHandle = NULL;
int cmd_cli_process_init(void)
{
    xTaskCreate(__cmd_cli_main, CMD_CLI_TASK_NAME, 256, (void*)NULL, 3, &syscmd_TaskHandle);
    if (syscmd_TaskHandle == NULL) 
    {
        CMD_ERR_LOG("Creat cmd cli task error !");
        return -1;
    }

  // taskEXIT_CRITICAL();
  return 0;
}