/*
 *  macros.h
 *
 *  Created on: Jun 1, 2015
 *      Author: gonzales
 */

#ifndef MACROS_H_
#define MACROS_H_
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <array>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include <zlib.h>
#include <math.h>
#include <thread>
#include <mutex>

#include "mpi.h"

using namespace std;

// Necessary for some auxiliary arrays. The maximum number of biclusters
#define MAX_BICLUSTERS 512000000

#define PROGRAM_NAME	"ParBibit"
#define PROGRAM_VERSION "1.0"

// To indicate whether the partial runtime must be shown
#define BENCHMARKING 1

#endif /* MACROS_H_ */
