const { Worker } = require("node:worker_threads");

/**
 * @type {Worker}
 */
let worker;

/**
 * @param {WorkerCommandData} commandData
 */
async function runCommandInWorker(commandData) {
	console.time("worker command");
	// console.group("Worker Command Output");
	if (!worker) {
		worker = new Worker("./index-worker.js");
		await new Promise((resolve) => {
			const resolveOnReady = (e) => {
				console.log("got message", e);
				if (e.command === "ready" && e.success) {
					worker.off("message", resolveOnReady);
					resolve();
				}
			};
			worker.on("message", resolveOnReady);
		});
	}
	return new Promise((resolve, reject) => {
		worker.on("message", (...args) => {
			try {
				const result = handleWorkerMessage(...args);
				resolve(result);
			} catch (err) {
				reject(err);
			}
		}, { once: true });
		worker.postMessage(commandData);
	});
}

/**
 * @param {{ data: WorkerResultData }} event
 */
function handleWorkerMessage(event) {
	console.timeEnd("worker command");
	console.log("got result from worker", event);
	if (event?.success) {
		return event.result;
	} else {
		// terminate worker as WASM runtime has likely exited on error
		worker.terminate();
		worker = null;
		throw event?.result || new Error("An error occurred");
	}
}

async function main() {
	const data = await runCommandInWorker({
		command: "getSamJsonFromFilePath",
		// args: ["./Critical.sam", true],
		args: ["./unit_anime_850347.sam", false],
	}).catch(() => null);
	console.log("got result\n", data);
	worker?.terminate();
}

// run `node index-node.js` from this folder in the terminal
main();