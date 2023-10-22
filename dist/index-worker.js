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
	return (new Promise((resolve, reject) => {
		self.addEventListener("sajsonfetchcompleted", ({ detail }) => {
			console.log("[sajson-worker] got fetch result", detail);
			if (detail.key === "success") {
				resolve();
			} else {
				reject(`Failed to retrieve file at URL [${url}] with [isEffect=${isEffect}]. See browser log for info.`);
			}
		}, { once: true });
		wasmModule.ccall("prefetch_url", "number", ["string"], [url]);
	})).then(() => {
		const failurePromise = new Promise((_, reject) => {
			Module.onAbort = (err) => {
				delete Module.onAbort;
				reject(err);
			};
		});
		const successPromise = Promise.resolve()
			.then(() => {
				console.log("[sajson-worker] calling get_sam_json_string");
				const samJsonString = wasmModule.ccall("get_sam_json_string", "string", ["boolean"], [isEffect]);
				const samJson = JSON.parse(samJsonString);
				console.log("[sajson-worker] calling clear_prefetched_data");
				wasmModule.ccall("clear_prefetched_data", "number", []);
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
});
