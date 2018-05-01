#pragma once

//only enable logging when the DEBUG constant is set
#ifdef DEBUG

//allow an application to pass debugging output to any other Stream
#ifndef DEBUG_STREAM
#define DEBUG_STREAM Serial
#endif

#define LOG(x) (DEBUG_STREAM).print(x)
#define LOGLN(x) (DEBUG_STREAM).println(x)

#else

#define LOG(x)
#define LOGLN(x)

#endif
