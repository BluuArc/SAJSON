#include <emscripten.h>
#include <emscripten/fetch.h>
#include "utils.h"

#include <string>
#include <iostream>
#include <memory>


/**
 * Convert all std::strings to const char* using constexpr if (C++17)
 */
template<typename T>
auto convert(T&& t) {
  if constexpr (std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, std::string>::value) {
    return std::forward<T>(t).c_str();
  }
  else {
    return std::forward<T>(t);
  }
}

/**
 * printf like formatting for C++ with std::string
 * Original source: https://stackoverflow.com/a/26221725/11722
 */
template<typename ... Args>
std::string stringFormatInternal(const std::string& format, Args ... args)
{
  const auto size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;
  if( size <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args ...);
  return std::string(buf.get(), buf.get() + size - 1);
}

// Source: https://gist.github.com/Zitrax/a2e0040d301bf4b8ef8101c0b1e3f1d5
template<typename ... Args>
std::string stringFormat(std::string fmt, Args ... args) {
  return stringFormatInternal(fmt, convert(std::forward<Args>(args))...);
}

EM_JS(char*, call_b64decode, (char* input), {
	const inputAsString = UTF8ToString(input);
	const output = atob(inputAsString);
	console.log({ inputAsString: inputAsString.slice(0, 20), input, output: output.slice(0, 20), outputLength: output.length });
	return stringToNewUTF8(output);
});

unsigned char* base64Decode(char* input) {
	return (unsigned char*) call_b64decode(input);
}

EM_JS(void, call_dispatchWindowEvent, (const char* eventName, const char* eventKey), {
  const eventNameAsString = UTF8ToString(eventName);
  const eventKeyAsString = UTF8ToString(eventKey);
  console.log("dispatching event", { eventNameAsString, eventKeyAsString });
	const event = new CustomEvent(eventNameAsString, { detail: { key: eventKeyAsString } });
  window.dispatchEvent(event);
});

static emscripten_fetch_t *lastFetch = NULL;

void downloadSucceeded(emscripten_fetch_t *fetch) {
  std::printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
  lastFetch = fetch;
  // NOTE: not scalable beyond one concurrent fetch
  std::string eventName = "sajsonfetchcompleted";
  std::string eventResult = "success";
  call_dispatchWindowEvent(eventName.c_str(), eventResult.c_str());
  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
  // emscripten_fetch_close(fetch); // Free data associated with the fetch.
}

void downloadFailed(emscripten_fetch_t *fetch) {
  std::printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
  lastFetch = fetch;
  // NOTE: not scalable beyond one concurrent fetch
  std::string eventName = "sajsonfetchcompleted";
  std::string eventResult = "failed";
  call_dispatchWindowEvent(eventName.c_str(), eventResult.c_str());
  // emscripten_fetch_close(fetch); // Also free data on failure.
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