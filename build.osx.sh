# g++ -m32 SAJSON.cpp SuperAnimCore.cpp -o SAJSON

# if emcc is not found, run source ./emsdk_env.sh
emcc SAJSON-WASM.cpp SuperAnimCore.cpp utils.cpp -o test.html -sEXPORTED_FUNCTIONS=_get_sam_json_string,_prefetch_url -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$stringToNewUTF8' -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -sALLOW_MEMORY_GROWTH -sFETCH