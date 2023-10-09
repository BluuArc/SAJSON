#include <emscripten/fetch.h>

void prefetchUrl(char* url);
emscripten_fetch_t* getPrefetchedUrl();
