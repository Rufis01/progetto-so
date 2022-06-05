#ifndef INLINE_RW_UTILS_H
#define INLINE_RW_UTILS_H

#include <stdint.h>

static inline int readWL(int fd, char *buf, uint8_t bufLen)
{
	uint8_t msgLen;

	if(read(fd, &msgLen, 1) == 0)
		return 0;

	msgLen = bufLen < msgLen ? bufLen : msgLen;
	return read(fd, buf, msgLen);
	
}

static inline int writeWL(int fd, char *buf, uint8_t msgLen)
{
	write(fd, &msgLen, 1);
	return write(fd, buf, msgLen);
}

#endif