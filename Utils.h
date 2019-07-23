/*
 * Utils.h
 *
 *  Created on: 10/03/2017
 *      Author: jorge
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "Macros.h"

class Utils {
public:
	static void log(const char* args, ...);
	static void exit(const char* args, ...);
	static double getSysTime() noexcept;
	static inline uint32_t popcount(uint32_t v) noexcept
        {
          uint32_t u;
          u = v - ((v>>1) & 033333333333) - ((v>>2) & 011111111111);
          return ((u + (u>>3)) & 030707070707) % 63;
        }
};

#endif /* UTILS_H_ */
