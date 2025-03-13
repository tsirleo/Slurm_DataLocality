/*****************************************************************************\
 *  alluxio.h - communication with Alluxio storage for data
 *  locality tracking.
\*****************************************************************************/

#ifndef _CONS_TRES_LOCALITY_ALLUXIO_H
#define _CONS_TRES_LOCALITY_ALLUXIO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure to hold node name and count
typedef struct {
    char* name;
    int count;
} NodeCount;

// Function to execute command and process results
int process_alluxio_location(const char* data_path, char** node_list, NodeCount** node_map, int* map_size);

#endif /* !_CONS_TRES_LOCALITY_ALLUXIO_H */
