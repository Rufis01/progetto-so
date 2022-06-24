#ifndef INLINE_RW_UTILS_H
#define INLINE_RW_UTILS_H

#include <stdint.h>

#include <unistd.h>

static inline int readWL(int fd, char *buf, uint8_t bufLen)
{
	uint8_t msgLen;

	if(read(fd, &msgLen, 1) == 0)
		return 0;
	
	//LOGD("Recieved message of %d bytes\n", msgLen);

	msgLen = bufLen < msgLen ? bufLen : msgLen;
	return read(fd, buf, msgLen);
	
}

static inline int writeWL(int fd, const char *buf, uint8_t msgLen)
{
	//LOGD("Sending message of %d bytes\n", msgLen);

	write(fd, &msgLen, 1);
	return write(fd, buf, msgLen);
}

#endif
