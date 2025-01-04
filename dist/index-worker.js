let readyResolve;
const readyPromise = new Promise((resolve) => { readyResolve = resolve; })
var Module = {
	onRuntimeInitialized: () => { readyResolve(); },
};

/**
 * @type {typeof postMessage}
 */
let doPostMessage;
let registerOnMessage;

/**
 * @type {typeof console.log}
 */
const log = (...args) => console.log("[sajson-worker]", ...args);

if (typeof importScripts === "function") {
	log("Initializing in browser context");
	importScripts("./sajson.js");
	doPostMessage = (...args) => self.postMessage(...args);
	registerOnMessage = (fn) => { self.onmessage = fn; };
} else {
	log("Initializing in node context");
	const { parentPort } = require("node:worker_threads");
	const importedModule = require("./sajson.js");
	Object.assign(importedModule, Module);
	Module = importedModule;
	doPostMessage = (...args) => parentPort.postMessage(...args);
	registerOnMessage = (fn) => {
		parentPort.on("message", fn);
	};
}

/**
 * @typedef {Object} EmscriptenWasmModule
 * @prop {(function_name: string, jsReturnType: string, jsArgTypes: string[], args: any[]) => any} ccall
 */

/**
 * @param {Uint8Array} data
 * @param {boolean} isEffect Whether the SAM animation is a battle effect. Battle effects have a slightly different data structure.
 * @returns {Promise<Object>} SAJSON data as a JSON object.
 */
function convertSamDataToJson(data, isEffect = false) {
	/**
	 * @type {EmscriptenWasmModule}
	 */
	const wasmModule = Module;
	const failurePromise = new Promise((_, reject) => {
		Module.onAbort = (err) => {
			delete Module.onAbort;
			reject(err);
		};
	});
	const successPromise = Promise.resolve()
		.then(() => {
			log("allocating buffer of size:", data.length);
			// based off of fetch code at https://github.com/emscripten-core/emscripten/blob/5e3ed77b2609dfe5b186249207466e2e2ad7e3e1/src/Fetch.js#L394-L405
			// and memory access docs at https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html#access-memory-from-javascript
			const ptr = Module._malloc(data.length);
			Module.HEAPU8.set(data, ptr);
			log("calling get_sam_json_string");
			const samJsonString = wasmModule.ccall("get_sam_json_string", "string", ["number", "number", "boolean"], [ptr, data.length, isEffect]);
			log("freeing buffer");
			Module._free(ptr);
			delete Module.onAbort;
			let samJson = null;
			if (samJsonString) {
				samJson = JSON.parse(samJsonString);
			} else {
				log("samJsonString is falsy, probably due to error in parsing");
			}
			return samJson;
		});
	return Promise.race([successPromise, failurePromise]);
}

/**
 * @param {string} url Path to SAM file.
 * @param {boolean} isEffect Whether the SAM animation is a battle effect. Battle effects have a slightly different data structure.
 * @returns {Promise<Object>} SAJSON data as a JSON object.
 */
function getSamJsonFromUrl(url, isEffect = false) {
	const samData = fetch(url)
		.then((r) => {
			let result;
			if (typeof r.bytes === "function") {
				result = r.bytes();
			} else {
				log("bytes() is not a function; using fallback");
				result = r.arrayBuffer().then((b) => new Uint8Array(b));
			}
			return result;
		});
	return samData.then((data) => convertSamDataToJson(data, isEffect));
}

/**
 * @param {string} filePath Path to SAM file.
 * @param {boolean} isEffect Whether the SAM animation is a battle effect. Battle effects have a slightly different data structure.
 * @returns {Promise<Object>} SAJSON data as a JSON object.
 */
function getSamJsonFromFilePath(filePath, isEffect = false) {
	const fs = require("node:fs/promises");
	const samData = fs.readFile(filePath)
		.then((b) => new Uint8Array(b));
	return samData.then((data) => convertSamDataToJson(data, isEffect));
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
registerOnMessage((event) => {
	/**
	 * @type {WorkerResultData}
	 */
	const returnValue = {
		command: event?.data?.command,
		args: event?.data?.args,
	};
	if (!event.data) {
		Object.assign(returnValue, {
			command: event?.command,
			args: event?.args,
		})
	}
	log("got command:", returnValue.command);
	switch (returnValue.command) {
		case "getSamJsonFromUrl":
			getSamJsonFromUrl(...returnValue.args)
				.then((result) => {
					returnValue.success = !!result;
					returnValue.result = result ? result : "An error occurred while parsing.";
				}).catch((error) => {
					returnValue.success = false;
					returnValue.result = error;
				}).finally(() => {
					doPostMessage(returnValue);
				});
				break;
		case "getSamJsonFromFilePath":
			getSamJsonFromFilePath(...returnValue.args)
				.then((result) => {
					returnValue.success = !!result;
					returnValue.result = result ? result : "An error occurred while parsing.";
				}).catch((error) => {
					returnValue.success = false;
					returnValue.result = error;
				}).finally(() => {
					doPostMessage(returnValue);
				});
				break;
		default:
			returnValue.success = false;
			returnValue.result = "Unknown command";
			doPostMessage(returnValue);
			break;
	}
});
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
	doPostMessage(returnValue);
	log("ready");
	log("module keys", Object.keys(Module))
});
