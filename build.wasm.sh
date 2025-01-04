# Intended for *nix based systems; initially built using Ubuntu on Windows Subsystem for Linux (WSL)
# WSL tip: if emcc is not found, run source ./emsdk_env.sh from emsdk directory before re-reunning the script
emcc SAJSON-WASM.cpp SuperAnimCore.cpp -o dist/sajson.js -sEXPORTED_FUNCTIONS=_get_sam_json_string,_malloc,_free -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -sALLOW_MEMORY_GROWTH -sENVIRONMENT=node,worker
