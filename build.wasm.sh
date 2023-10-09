# Intended for *nix based systems; initially built using Ubuntu on Windows Subsystem for Linux (WSL)
# WSL tip: if emcc is not found, run source ./emsdk_env.sh before re-reunning the script
emcc SAJSON-WASM.cpp SuperAnimCore.cpp utils.cpp -o sajson.html -sEXPORTED_FUNCTIONS=_get_sam_json_string,_prefetch_url -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -sALLOW_MEMORY_GROWTH -sFETCH