(() => {
	const outputElement = document.getElementById("form-output");
	const copyJsonButton = document.getElementById("copy-json-button");
	const submitButton = document.getElementById("sam-form-submit");
	let latestSajsonData;
	/**
	 * @type {Worker}
	 */
	let worker;

	/**
	 * @param {{ data: WorkerResultData }} event
	 */
	function handleWorkerMessage(event) {
		console.timeEnd("worker command");
		console.log("got result from worker", event);
		if (event?.data?.success) {
			handleSuccessfulResult(event.data.result);
		} else {
			handleFailedResult(event?.data?.result);
		}
		console.groupEnd("Worker Command Output");
	}

	function copySajsonDataToClipboard() {
		return self.navigator.clipboard.writeText(JSON.stringify(latestSajsonData, null, "\t")).then(() => {
			alert("Copied JSON data to clipboard");
		});
	}
	/**
	 * @param {object} sajsonData
	 */
	function handleSuccessfulResult(sajsonData) {
		latestSajsonData = sajsonData;
		const jsonContainer = document.createElement("pre");
		jsonContainer.textContent = JSON.stringify(sajsonData, null, "\t");
		logStatus(jsonContainer);
		copyJsonButton.addEventListener("click", copySajsonDataToClipboard);
		copyJsonButton.removeAttribute("disabled");
		submitButton.removeAttribute("disabled");
	}

	/**
	 * @param {Error | string} errorMessage
	 */
	function handleFailedResult(errorMessage) {
		latestSajsonData = null;
		copyJsonButton.removeEventListener("click", copySajsonDataToClipboard);
		copyJsonButton.setAttribute("disabled", "");
		console.error(errorMessage);
		let errorMessageToDisplay = errorMessage.message || errorMessage;
		if (errorMessageToDisplay?.includes?.("We don't have valid frames")) {
			errorMessageToDisplay += "\nSuggestion: Change the value of the Battle Effect checkbox and try again."
		}
		logStatus(`An error has occurred.\n${errorMessageToDisplay}`);
		submitButton.removeAttribute("disabled");

		// terminate worker as WASM runtime has likely exited on error
		worker.terminate();
		worker = null;
	}

	/**
	 * @param {string | HTMLElement} statusMessage 
	 */
	function logStatus(statusMessage) {
		outputElement.innerHTML = "";
		if (statusMessage instanceof HTMLElement) {
			outputElement.append(statusMessage);
		} else {
			outputElement.textContent = statusMessage;
		}
	}

	/**
	 * @param {WorkerCommandData} commandData
	 */
	async function runCommandInWorker(commandData) {
		console.time("worker command");
		console.group("Worker Command Output");
		if (!worker) {
			worker = new Worker("./index-worker.js");
			await new Promise((resolve) => {
				worker.addEventListener("message", (e) => {
					if (e.data?.command === "ready" && e.data?.success) {
						resolve();
					}
				}, { once: true });
			});
			worker.addEventListener("message", handleWorkerMessage);
		}
		worker.postMessage(commandData);
	}

	document.getElementById("sam-form").addEventListener("submit", (e) => {
		e.preventDefault();
		submitButton.setAttribute("disabled", "");
		logStatus("Loading... If this takes a while, either look at the browser console for errors or refresh the page and try again.");
		const formData = new FormData(e.target);
		console.log(formData);
		runCommandInWorker({
			command: "getSamJson",
			args: [formData.get("samFileUrl"), formData.get("isBattleEffect") === "on"],
		});
	});

	logStatus("App is ready. Output will appear here.");

	window.runCommandInWorker = runCommandInWorker;
})();