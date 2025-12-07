#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <openxr/openxr.h>
#include <string>

// Error handling utilities
const char* XrResultToString(XrResult result);
void LogXrError(XrResult result, const char* function, const char* file, int line);

#define LOG_XR_ERROR(result) LogXrError(result, __FUNCTION__, __FILE__, __LINE__)

#endif // ERROR_HANDLER_H

