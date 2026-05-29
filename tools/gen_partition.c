#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PARTITIONS 32
#define MAX_LINE_LEN 256
#define MAX_NAME_LEN 64

typedef struct {
    char name[MAX_NAME_LEN];
    char type[MAX_NAME_LEN];
    uint32_t address;
    uint32_t size;
    uint32_t end;
    int has_address;
} partition_t;

typedef struct {
    uint32_t flash_base;
    uint32_t flash_size;
    uint32_t erase_size;
    char start_after[MAX_NAME_LEN];
    char align[MAX_NAME_LEN];
    partition_t fixed[MAX_PARTITIONS];
    partition_t layout[MAX_PARTITIONS];
    size_t fixed_count;
    size_t layout_count;
} partition_config_t;

static char *trim(char *str)
{
    char *end = NULL;

    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    end = str + strlen(str) - 1;
    while ((end > str) && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    return str;
}

static void strip_comment(char *line)
{
    char *comment = strchr(line, '#');

    if (comment != NULL) {
        *comment = '\0';
    }
}

static int parse_u32(const char *text, uint32_t *value)
{
    char *end = NULL;
    unsigned long result = 0;
    char buffer[MAX_NAME_LEN];
    size_t len = strlen(text);
    unsigned long multiplier = 1;

    if ((text == NULL) || (value == NULL) || (len == 0) || (len >= sizeof(buffer))) {
        return -1;
    }

    strcpy(buffer, text);
    len = strlen(buffer);

    if ((buffer[len - 1] == 'K') || (buffer[len - 1] == 'k')) {
        multiplier = 1024UL;
        buffer[len - 1] = '\0';
    } else if ((buffer[len - 1] == 'M') || (buffer[len - 1] == 'm')) {
        multiplier = 1024UL * 1024UL;
        buffer[len - 1] = '\0';
    }

    errno = 0;
    result = strtoul(buffer, &end, 0);
    if ((errno != 0) || (end == buffer) || (*end != '\0')) {
        return -1;
    }

    result *= multiplier;
    if (result > UINT32_MAX) {
        return -1;
    }

    *value = (uint32_t)result;
    return 0;
}

static uint32_t align_up(uint32_t value, uint32_t align)
{
    return ((value + align - 1u) / align) * align;
}

static int indentation_of(const char *line)
{
    int count = 0;

    while (line[count] == ' ') {
        count++;
    }

    return count;
}

static int split_key_value(char *line, char **key, char **value)
{
    char *colon = strchr(line, ':');

    if (colon == NULL) {
        return -1;
    }

    *colon = '\0';
    *key = trim(line);
    *value = trim(colon + 1);
    return 0;
}

static partition_t *append_partition(partition_t *items, size_t *count)
{
    if (*count >= MAX_PARTITIONS) {
        return NULL;
    }

    memset(&items[*count], 0, sizeof(items[*count]));
    (*count)++;
    return &items[*count - 1u];
}

static int parse_partition_yaml(const char *path, partition_config_t *config)
{
    FILE *fp = NULL;
    char line_buf[MAX_LINE_LEN];
    char section[MAX_NAME_LEN] = "";
    partition_t *current = NULL;

    fp = fopen(path, "r");
    if (fp == NULL) {
        perror(path);
        return -1;
    }

    memset(config, 0, sizeof(*config));

    while (fgets(line_buf, sizeof(line_buf), fp) != NULL) {
        char *text = NULL;
        int indent = 0;

        strip_comment(line_buf);
        text = trim(line_buf);
        if (*text == '\0') {
            continue;
        }

        indent = indentation_of(line_buf);

        if ((indent == 0) && (text[strlen(text) - 1u] == ':')) {
            text[strlen(text) - 1u] = '\0';
            strncpy(section, text, sizeof(section) - 1u);
            section[sizeof(section) - 1u] = '\0';
            current = NULL;
            continue;
        }

        if (text[0] == '-') {
            char *key = NULL;
            char *value = NULL;
            char *rest = trim(text + 1);

            if (strcmp(section, "fixed") == 0) {
                current = append_partition(config->fixed, &config->fixed_count);
            } else if (strcmp(section, "layout") == 0) {
                current = append_partition(config->layout, &config->layout_count);
            } else {
                fprintf(stderr, "list item is not supported in section %s\n", section);
                fclose(fp);
                return -1;
            }

            if (current == NULL) {
                fprintf(stderr, "too many partitions\n");
                fclose(fp);
                return -1;
            }

            if (*rest == '\0') {
                continue;
            }

            if (split_key_value(rest, &key, &value) != 0) {
                fprintf(stderr, "invalid list item: %s\n", rest);
                fclose(fp);
                return -1;
            }

            if (strcmp(key, "name") == 0) {
                strncpy(current->name, value, sizeof(current->name) - 1u);
            }
            continue;
        }

        {
            char *key = NULL;
            char *value = NULL;

            if (split_key_value(text, &key, &value) != 0) {
                fprintf(stderr, "invalid line: %s\n", text);
                fclose(fp);
                return -1;
            }

            if (strcmp(section, "flash") == 0) {
                if (strcmp(key, "base") == 0) {
                    if (parse_u32(value, &config->flash_base) != 0) {
                        fclose(fp);
                        return -1;
                    }
                } else if (strcmp(key, "size") == 0) {
                    if (parse_u32(value, &config->flash_size) != 0) {
                        fclose(fp);
                        return -1;
                    }
                } else if (strcmp(key, "erase_size") == 0) {
                    if (parse_u32(value, &config->erase_size) != 0) {
                        fclose(fp);
                        return -1;
                    }
                }
            } else if (strcmp(section, "auto_layout") == 0) {
                if (strcmp(key, "start_after") == 0) {
                    strncpy(config->start_after, value, sizeof(config->start_after) - 1u);
                } else if (strcmp(key, "align") == 0) {
                    strncpy(config->align, value, sizeof(config->align) - 1u);
                }
            } else if ((strcmp(section, "fixed") == 0) || (strcmp(section, "layout") == 0)) {
                if (current == NULL) {
                    fprintf(stderr, "partition property without item: %s\n", key);
                    fclose(fp);
                    return -1;
                }

                if (strcmp(key, "type") == 0) {
                    strncpy(current->type, value, sizeof(current->type) - 1u);
                } else if (strcmp(key, "address") == 0) {
                    if (parse_u32(value, &current->address) != 0) {
                        fclose(fp);
                        return -1;
                    }
                    current->has_address = 1;
                } else if (strcmp(key, "size") == 0) {
                    if (parse_u32(value, &current->size) != 0) {
                        fclose(fp);
                        return -1;
                    }
                } else if (strcmp(key, "name") == 0) {
                    strncpy(current->name, value, sizeof(current->name) - 1u);
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

static const partition_t *find_partition(const partition_t *items, size_t count, const char *name)
{
    for (size_t i = 0; i < count; i++) {
        if (strcmp(items[i].name, name) == 0) {
            return &items[i];
        }
    }

    return NULL;
}

static int compare_partition_addr(const void *lhs, const void *rhs)
{
    const partition_t *a = (const partition_t *)lhs;
    const partition_t *b = (const partition_t *)rhs;

    if (a->address < b->address) {
        return -1;
    }

    if (a->address > b->address) {
        return 1;
    }

    return 0;
}

static int build_layout(partition_config_t *config, partition_t *entries, size_t *entry_count)
{
    uint32_t align = config->erase_size;
    uint32_t cursor = config->flash_base;
    uint32_t flash_end = config->flash_base + config->flash_size;
    const partition_t *start = NULL;

    if ((config->flash_base == 0u) || (config->flash_size == 0u) || (config->erase_size == 0u)) {
        fprintf(stderr, "flash base/size/erase_size must be configured\n");
        return -1;
    }

    if ((config->align[0] != '\0') && (strcmp(config->align, "erase_size") != 0)) {
        if (parse_u32(config->align, &align) != 0) {
            fprintf(stderr, "invalid auto_layout.align: %s\n", config->align);
            return -1;
        }
    }

    *entry_count = 0;

    for (size_t i = 0; i < config->fixed_count; i++) {
        partition_t item = config->fixed[i];

        if (!item.has_address || (item.size == 0u) || (item.name[0] == '\0')) {
            fprintf(stderr, "invalid fixed partition\n");
            return -1;
        }

        item.end = item.address + item.size;
        entries[(*entry_count)++] = item;
    }

    if (config->start_after[0] != '\0') {
        start = find_partition(entries, *entry_count, config->start_after);
        if (start == NULL) {
            fprintf(stderr, "start_after partition not found: %s\n", config->start_after);
            return -1;
        }
        cursor = start->end;
    }

    for (size_t i = 0; i < config->layout_count; i++) {
        partition_t item = config->layout[i];

        if ((item.size == 0u) || (item.name[0] == '\0') || (item.type[0] == '\0')) {
            fprintf(stderr, "invalid layout partition\n");
            return -1;
        }

        if ((item.size % align) != 0u) {
            fprintf(stderr, "%s size is not aligned to %u\n", item.name, align);
            return -1;
        }

        cursor = align_up(cursor, align);
        item.address = cursor;
        item.end = item.address + item.size;
        entries[(*entry_count)++] = item;
        cursor = item.end;
    }

    qsort(entries, *entry_count, sizeof(entries[0]), compare_partition_addr);

    for (size_t i = 0; i < *entry_count; i++) {
        if ((entries[i].address < config->flash_base) || (entries[i].end > flash_end)) {
            fprintf(stderr, "%s is outside flash range\n", entries[i].name);
            return -1;
        }

        if (((entries[i].address % align) != 0u) || ((entries[i].size % align) != 0u)) {
            fprintf(stderr, "%s is not aligned to %u\n", entries[i].name, align);
            return -1;
        }

        if ((i > 0u) && (entries[i - 1u].end > entries[i].address)) {
            fprintf(stderr, "partition overlap: %s and %s\n", entries[i - 1u].name, entries[i].name);
            return -1;
        }
    }

    return 0;
}

static void macro_name(const char *name, char *out, size_t out_size)
{
    size_t i = 0;

    for (; (name[i] != '\0') && (i + 1u < out_size); i++) {
        if ((name[i] == '-') || (name[i] == ' ')) {
            out[i] = '_';
        } else {
            out[i] = (char)toupper((unsigned char)name[i]);
        }
    }

    out[i] = '\0';
}

static int write_header(const char *path, const partition_config_t *config,
                        const partition_t *entries, size_t entry_count)
{
    FILE *fp = fopen(path, "w");

    if (fp == NULL) {
        perror(path);
        return -1;
    }

    fprintf(fp, "#ifndef PARTITION_CONFIG_H\n");
    fprintf(fp, "#define PARTITION_CONFIG_H\n\n");
    fprintf(fp, "#include <stdint.h>\n\n");
    fprintf(fp, "/*\n");
    fprintf(fp, " * Auto-generated by tools/gen_partition.c.\n");
    fprintf(fp, " * Do not edit this file by hand; edit partition/partition.yaml instead.\n");
    fprintf(fp, " */\n\n");
    fprintf(fp, "#define PARTITION_FLASH_BASE_ADDR    0x%08Xu\n", config->flash_base);
    fprintf(fp, "#define PARTITION_FLASH_SIZE         %uu\n", config->flash_size);
    fprintf(fp, "#define PARTITION_FLASH_END_ADDR     0x%08Xu\n", config->flash_base + config->flash_size);
    fprintf(fp, "#define PARTITION_FLASH_ERASE_SIZE   %uu\n\n", config->erase_size);
    fprintf(fp, "#define PARTITION_TYPE_STAGE0             1u\n");
    fprintf(fp, "#define PARTITION_TYPE_PARTITION_TABLE   2u\n");
    fprintf(fp, "#define PARTITION_TYPE_BOOTLOADER        3u\n");
    fprintf(fp, "#define PARTITION_TYPE_APP               4u\n");
    fprintf(fp, "#define PARTITION_TYPE_BOOT_CONTROL      5u\n");
    fprintf(fp, "#define PARTITION_TYPE_CONFIG            6u\n");
    fprintf(fp, "#define PARTITION_TYPE_RESERVED          7u\n\n");

    for (size_t i = 0; i < entry_count; i++) {
        char name_macro[MAX_NAME_LEN];
        char type_macro[MAX_NAME_LEN];

        macro_name(entries[i].name, name_macro, sizeof(name_macro));
        macro_name(entries[i].type, type_macro, sizeof(type_macro));

        fprintf(fp, "#define PARTITION_%s_ADDR        0x%08Xu\n", name_macro, entries[i].address);
        fprintf(fp, "#define PARTITION_%s_SIZE        %uu\n", name_macro, entries[i].size);
        fprintf(fp, "#define PARTITION_%s_END_ADDR    0x%08Xu\n", name_macro, entries[i].end);
        fprintf(fp, "#define PARTITION_%s_TYPE        PARTITION_TYPE_%s\n\n", name_macro, type_macro);
    }

    if (find_partition(entries, entry_count, "config") != NULL) {
        fprintf(fp, "#define CONFIG_PART_ADDR             PARTITION_CONFIG_ADDR\n");
        fprintf(fp, "#define CONFIG_PART_SIZE             PARTITION_CONFIG_SIZE\n\n");
    }

    fprintf(fp, "#endif /* PARTITION_CONFIG_H */\n");
    fclose(fp);
    return 0;
}

static int write_report(const char *path, const partition_t *entries, size_t entry_count)
{
    FILE *fp = fopen(path, "w");

    if (fp == NULL) {
        perror(path);
        return -1;
    }

    fprintf(fp, "# Partition Layout\n\n");
    fprintf(fp, "| Name | Type | Address | Size | End |\n");
    fprintf(fp, "| --- | --- | ---: | ---: | ---: |\n");

    for (size_t i = 0; i < entry_count; i++) {
        fprintf(fp, "| %s | %s | 0x%08X | %u | 0x%08X |\n",
                entries[i].name,
                entries[i].type,
                entries[i].address,
                entries[i].size,
                entries[i].end);
    }

    fclose(fp);
    return 0;
}

static int ensure_directory(const char *path)
{
    char command[MAX_LINE_LEN];
    int ret = snprintf(command, sizeof(command), "mkdir -p %s", path);

    if ((ret < 0) || ((size_t)ret >= sizeof(command))) {
        return -1;
    }

    return system(command);
}

int main(int argc, char **argv)
{
    const char *input = "partition/partition.yaml";
    const char *out_dir = "generated";
    char header_path[MAX_LINE_LEN];
    char report_path[MAX_LINE_LEN];
    partition_config_t config;
    partition_t entries[MAX_PARTITIONS];
    size_t entry_count = 0;

    if (argc >= 2) {
        input = argv[1];
    }

    if (argc >= 3) {
        out_dir = argv[2];
    }

    if (parse_partition_yaml(input, &config) != 0) {
        return 1;
    }

    if (build_layout(&config, entries, &entry_count) != 0) {
        return 1;
    }

    if (ensure_directory(out_dir) != 0) {
        fprintf(stderr, "failed to create output directory: %s\n", out_dir);
        return 1;
    }

    snprintf(header_path, sizeof(header_path), "%s/partition_config.h", out_dir);
    snprintf(report_path, sizeof(report_path), "%s/partition_layout.md", out_dir);

    if (write_header(header_path, &config, entries, entry_count) != 0) {
        return 1;
    }

    if (write_report(report_path, entries, entry_count) != 0) {
        return 1;
    }

    for (size_t i = 0; i < entry_count; i++) {
        printf("%-18s 0x%08X %8u bytes\n", entries[i].name, entries[i].address, entries[i].size);
    }

    return 0;
}
