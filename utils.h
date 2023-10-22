#include <emscripten/fetch.h>

void prefetchUrl(char* url);
void clearPrefetchedData();
emscripten_fetch_t* getPrefetchedUrl();
