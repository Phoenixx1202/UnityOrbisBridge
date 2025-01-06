#pragma once

extern const char *logPrefixes[];

extern uint16_t inputTextBuffer[513];
extern uint16_t inputImeTitle[512];

extern "C" int SYSCALL(long num, ...);

void printToConsole(int type, const char *format, ...);
void printAndLog(int type, const char *message, ...);

int convert_to_utf16(const char *utf8, uint16_t *utf16, uint32_t available);
int convert_from_utf16(const uint16_t *utf16, char *utf8, uint32_t size);
bool getKeyboardInput(const char *Title, const char *initialTextBuffer, char *out_buffer);

int df(std::string mountPoint, long &percentUsed, double &totalSpace, double &usedSpace, double &freeSpace);
void build_iovec(struct iovec **iov, int *iovlen, const char *name, const void *val, size_t len);
void mount_large_fs(const char *device, const char *mountpoint, const char *fstype, const char *mode, unsigned int flags);
bool if_exists(const char *path);