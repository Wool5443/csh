#ifndef CHELL_H
#define CHELL_H

#include "Logger.h"

ErrorCode CompileAndRunFile(const char path[static 1], const char* args[], const char cshellPath[static 1]);

#endif // CHELL_H
