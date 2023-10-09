#include <emscripten.h>
#include <emscripten/fetch.h>
#include "utils.h"

#include <string>
#include <iostream>

EM_JS(void, call_dispatchWindowEvent, (const char* eventName, const char* eventKey), {
  const eventNameAsString = UTF8ToString(eventName);
  const eventKeyAsString = UTF8ToString(eventKey);
  console.log("dispatching event", { eventNameAsString, eventKeyAsString });
	const event = new CustomEvent(eventNameAsString, { detail: { key: eventKeyAsString } });
  (window || self).dispatchEvent(event);
});

static emscripten_fetch_t *lastFetch = NULL;

/**
 * Fetch code based on https://emscripten.org/docs/api_reference/fetch.html?highlight=malloc#introduction.
 *
 * Initially tried passing a base64 string to pass the SAM bytes to the WASM code, but it did not transfer 1:1,
 * so the implementation here keeps it all on the WASM side to fetch the data and store it in memory.
 *
 * Due to how fetch works currently with Emscripten as of Oct 8, 2023, it's easier to use an event-based approach between
 * the WASM code and the web page than it is to try and support a synchronous or waitable fetch that's entirely on the WASM
 * side.
 */

void downloadSucceeded(emscripten_fetch_t *fetch) {
  std::printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
  lastFetch = fetch;
  // NOTE: not scalable beyond one concurrent fetch
  std::string eventName = "sajsonfetchcompleted";
  std::string eventResult = "success";
  call_dispatchWindowEvent(eventName.c_str(), eventResult.c_str());
  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
  // emscripten_fetch_close(fetch); // Free data associated with the fetch; deferred to initiator of fetch
}

void downloadFailed(emscripten_fetch_t *fetch) {
  std::printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
  lastFetch = fetch;
  // NOTE: not scalable beyond one concurrent fetch
  std::string eventName = "sajsonfetchcompleted";
  std::string eventResult = "failed";
  call_dispatchWindowEvent(eventName.c_str(), eventResult.c_str());
  // emscripten_fetch_close(fetch); // Also free data on failure; deferred to initiator of fetch
}

void prefetchUrl(char* url) {
  if (lastFetch != NULL) {
		emscripten_fetch_close(lastFetch);
		lastFetch = NULL;
	}
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  std::strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = downloadSucceeded;
  attr.onerror = downloadFailed;
  emscripten_fetch(&attr, url);
}

emscripten_fetch_t *getPrefetchedUrl() {
  return lastFetch;
}