#ifndef LabSound_Logging_h
#define LabSound_Logging_h

#include <cstdarg>
#include <stdio.h>
#include <string>
#include <iostream>

#define LOG(...) LabSoundLog(__FILE__, __LINE__, __VA_ARGS__);

void LabSoundLog(const char* file, int line, const char* fmt, ...);
void LabSoundAssertLog(const char* file, int line, const char * function, const char * assertion);

#endif
