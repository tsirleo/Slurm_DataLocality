/*****************************************************************************\
 *  alluxio.c - communication with Alluxio storage for data
 *  locality tracking.
\*****************************************************************************/

#include "alluxio.h"
#include "select_cons_tres_locality.h"


int process_alluxio_location(const char* data_path, char** node_list, NodeCount** node_map, int* map_size) {
    char command[1024];

    snprintf(command, sizeof(command),
        "alluxio fs location %s/* | "
        "grep -oE '\\b([0-9]{1,3}\\.){3}[0-9]{1,3}\\b|\\bmichman[a-zA-Z0-9-]+\\b' | "
        "grep -vE '\\b(data_|file_|id|is|nodes|on|with)\\b|[0-9]{9,}' | "
        "sort | uniq -c",
        data_path);
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        return SLURM_ERROR;
    }

    char line[256];
    int capacity = 10;
    int count = 0;
    size_t name_list_size = 256;
    char* temp_node_list = xmalloc(name_list_size);
    NodeCount* temp_map = xmalloc(sizeof(NodeCount) * capacity);
    *temp_node_list = '\0';

    while (fgets(line, sizeof(line), pipe) != NULL) {
        int num;
        char node[256];

        if (sscanf(line, "%d %s", &num, node) == 2) {
            if (count >= capacity) {
                capacity *= 2;
                NodeCount* new_map = xrealloc(temp_map, sizeof(NodeCount) * capacity);

                if (!new_map) {
                    xfree(temp_node_list);
                    for (int i = 0; i < count; i++) {
                        xfree(temp_map[i].name);
                    }
                    xfree(temp_map);
                    pclose(pipe);
                    return SLURM_ERROR;
                }
                temp_map = new_map;
            }

            temp_map[count].name = xstrdup(node);
            temp_map[count].count = num;

            const size_t needed_size = strlen(temp_node_list) + strlen(node) + 2;
            if (needed_size > name_list_size) {
                name_list_size *= 2;
                char* new_list = xrealloc(temp_node_list, name_list_size);

                if (!new_list) {
                    xfree(temp_node_list);
                    for (int i = 0; i <= count; i++) {
                        xfree(temp_map[i].name);
                    }
                    xfree(temp_map);
                    pclose(pipe);
                    return SLURM_ERROR;
                }
                temp_node_list = new_list;
            }

            if (count > 0) {
                xstrcat(temp_node_list, ",");
            }
            xstrcat(temp_node_list, node);

            count++;
        }
    }

    const int status = pclose(pipe);
    if (status == -1) {
        xfree(temp_node_list);
        for (int i = 0; i < count; i++) {
            xfree(temp_map[i].name);
        }
        xfree(temp_map);
        return SLURM_ERROR;
    }
    
    *node_list = temp_node_list;
    *node_map = temp_map;
    *map_size = count;

    return SLURM_SUCCESS;
}
