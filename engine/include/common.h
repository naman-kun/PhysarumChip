#ifndef PHYSARUM_COMMON_H
#define PHYSARUM_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define MAX_NAME_LEN 128
#define MAX_PORTS_PER_COMP 16
#define MAX_COMPONENTS 256
#define MAX_NETS 512
#define MAX_PATH_NODES 1024
#define MAX_OBSTACLES 128

typedef enum {
    STATUS_OK = 0,
    STATUS_ERROR_INVALID_PARAM = 1,
    STATUS_ERROR_OUT_OF_BOUNDS = 2,
    STATUS_ERROR_OVERLAP = 3,
    STATUS_ERROR_NO_PATH = 4,
    STATUS_ERROR_CAPACITY_EXCEEDED = 5,
    STATUS_ERROR_ALLOCATION_FAILED = 6,
    STATUS_ERROR_JSON = 7,
    STATUS_ERROR_CONVERGENCE = 8
} StatusCode;

typedef struct {
    double x;
    double y;
} Point2D;

typedef struct {
    int gx;
    int gy;
} GridPoint;

typedef struct {
    double x;
    double y;
    double width;
    double height;
} Rect2D;

#endif /* PHYSARUM_COMMON_H */
