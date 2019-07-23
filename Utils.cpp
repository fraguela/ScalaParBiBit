/*
 * Utils.cpp
 *
 *  Created on: 10/03/2017
 *      Author: jorge
 */

#include "Utils.h"
#include <sys/time.h>

void Utils::log(const char* args, ...)
{
	va_list va_ptr;
	fprintf(stderr, "[P%d]", getpid());
	va_start(va_ptr, args);
	vfprintf(stderr, args, va_ptr);
	va_end(va_ptr);
}

void Utils::exit(const char* args, ...)
{
	va_list va_ptr;
 	fprintf(stderr, "[P%d]", getpid());
	//print out the message
	va_start(va_ptr, args);
	vfprintf(stderr, args, va_ptr);
	va_end(va_ptr);

	//exit the program
	::exit(-1);
}

double Utils::getSysTime() noexcept
{
	double dtime;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	dtime = (double) tv.tv_sec;
	dtime += (double) (tv.tv_usec) / 1000000.0;

	return dtime;

}

/*
uint32_t Utils::popcount(uint32_t v)
{
    uint32_t u;
    u = v - ((v>>1) & 033333333333) - ((v>>2) & 011111111111);
    return ((u + (u>>3)) & 030707070707) % 63;
}
*/
