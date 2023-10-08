#include <string>
#include <emscripten/fetch.h>

unsigned char* base64Decode(char*);

void prefetchUrl(char* url);
emscripten_fetch_t* getPrefetchedUrl();

template<typename ... Args>
std::string stringFormat(std::string fmt, Args ... args);