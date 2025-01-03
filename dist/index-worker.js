let readyResolve;
const readyPromise = new Promise((resolve) => { readyResolve = resolve; })
var Module = {
	onRuntimeInitialized: () => { readyResolve(); },
};

importScripts("./sajson.js");

/**
 * @typedef {Object} EmscriptenWasmModule
 * @prop {(function_name: string, jsReturnType: string, jsArgTypes: string[], args: any[]) => any} ccall
 */

/**
 * @param {string} url Path to SAM file.
 * @param {boolean} isEffect Whether the SAM animation is a battle effect. Battle effects have a slightly different data structure.
 * @returns {Promise<Object>} SAJSON data as a JSON object.
 */
function getSamJson(url, isEffect = false) {
	/**
	 * @type {EmscriptenWasmModule}
	 */
	const wasmModule = Module;
	const samData = fetch(url)
		.then((r) => {
			let result;
			if (typeof r.bytes === "function") {
				result = r.bytes();
			} else {
				console.log("[sajson-worker] bytes() is not a function; using fallback");
				result = r.arrayBuffer().then((b) => new Uint8Array(b));
			}
			return result;
		});
	return samData.then((data) => {
		const failurePromise = new Promise((_, reject) => {
			Module.onAbort = (err) => {
				delete Module.onAbort;
				reject(err);
			};
		});
		const successPromise = Promise.resolve()
			.then(() => {
				console.log("[sajson-worker] allocating buffer of size:", data.length);
				// based off of fetch code at https://github.com/emscripten-core/emscripten/blob/5e3ed77b2609dfe5b186249207466e2e2ad7e3e1/src/Fetch.js#L394-L405
				// and memory access docs at https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html#access-memory-from-javascript
				const ptr = Module._malloc(data.length);
				Module.HEAPU8.set(data, ptr);
				console.log("[sajson-worker] calling get_sam_json_string");
				const samJsonString = wasmModule.ccall("get_sam_json_string", "string", ["number", "number", "boolean"], [ptr, data.length, isEffect]);
				const samJson = JSON.parse(samJsonString);
				console.log("[sajson-worker] freeing buffer");
				Module._free(ptr);
				delete Module.onAbort;
				return samJson;
			});
		return Promise.race([successPromise, failurePromise]);
	});
}

/**
 * @typedef {"getSamJson"} WorkerCommand
 *
 * @typedef {object} WorkerCommandData
 * @prop {WorkerCommand} command
 * @prop {any[]} args
 *
 * @typedef {object} WorkerResultData
 * @prop {WorkerCommand} command
 * @prop {any[]} args
 * @prop {any} result
 * @prop {boolean} success
 */

/**
 *
 * @param {{ data: WorkerCommandData }} event
 */
self.onmessage = (event) => {
	/**
	 * @type {WorkerResultData}
	 */
	const returnValue = {
		command: event?.data?.command,
		args: event?.data?.args,
	};
	switch (event?.data?.command) {
		case "getSamJson":
			getSamJson(...event.data.args)
				.then((result) => {
					returnValue.success = true;
					returnValue.result = result;
					self.postMessage(returnValue);
				}).catch((error) => {
					returnValue.success = false;
					returnValue.result = error;
					self.postMessage(returnValue);
				});
				break;
		default:
			returnValue.success = false;
			returnValue.result = "Unknown command";
			self.postMessage(returnValue);
			break;
	}
};
readyPromise.then(() => {
	/**
	 * @type {WorkerResultData}
	 */
	const returnValue = {
		command: "ready",
		args: [],
		success: true,
		result: true,
	};
	self.postMessage(returnValue);
	console.log("[sajson-worker] ready");
	console.log("[sajson-worker] module keys", Object.keys(Module))
});
