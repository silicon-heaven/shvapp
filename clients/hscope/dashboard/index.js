let websocket = null;
const ws_uri = "wss://nirvana.elektroline.cz:3778";
const user = "hscope";
const password = "holyshit!";

const txt_log = document.getElementById("txt_log");

const debug = (...args) => {
	if (txt_log) {
		txt_log.value += args.join(" ") + "\n";
		txt_log.scrollTop = txt_log.scrollHeight;
	}
};

const format_severity = (value) => {
	switch (value) {
		case "ok":
			return "🟢";
		case "warn":
			return "🟠";
		case "error":
			return "🔴";
		default:
			return "unknown severity";
	}
};


const resolve_hscope_tree = (path, container) => {

	websocket.callRpcMethod(path, "dir").then((methods) => {
		if (methods.rpcValue.value[2].value.some(x => x.value === "status")) {
			const nodeContainer = document.createElement("tr");
			container.appendChild(nodeContainer);
			const runElement = document.createElement("button");
			runElement.innerText = "Run";
			runElement.onclick = () => {
				websocket.callRpcMethod(path, "run");
			};
			nodeContainer.appendChild(runElement);

			const pathElement = document.createElement("td");
			pathElement.innerText = path;
			nodeContainer.appendChild(pathElement);
			const severityElement = document.createElement("td");
			const messageElement = document.createElement("td");
			const dateElement = document.createElement("td");

			const updateElements = (value) => {
				if (typeof value.severity !== "undefined") {
					severityElement.innerText = format_severity(value.severity.value);
				} else {
					severityElement.innerText = "❓";
				}

				if (typeof value.message !== "undefined") {
					messageElement.innerText = value.message.value;
				} else {
					messageElement.innerText = "";
				}

				if (typeof value.timeChanged !== "undefined") {
					dateElement.innerText = new Date(value.timeChanged.value.epochMsec).toLocaleString();
				} else {
					dateElement.innerText = "";
				}
			};

			websocket.callRpcMethod(path, "status").then((value) => {
				updateElements(value.rpcValue.value[2].value);
			});

			websocket.subscribe(path, "chng", (changedPath, type, value) => {
				updateElements(value[1].value);
			});
			nodeContainer.appendChild(severityElement);
			nodeContainer.appendChild(messageElement);
			nodeContainer.appendChild(dateElement);
		}
	})

	websocket.callRpcMethod(path, "ls").then((paths) => {
		paths.rpcValue.value[2].value.forEach((childPath) => {
			resolve_hscope_tree(path + "/" + childPath, container);
		});
	});
};

const send_ping = () => {
	websocket.callRpcMethod(".broker/app", "ping");
};

const connect_websocket = () => {
	try {
		websocket = new WsClient({
			user,
			password,
			wsUri: ws_uri,
			requestHandler: (rpc_msg) => {
					const method = rpc_msg.method().asString();
					const resp = new RpcMessage();
					if (method === "dir") {
						resp.setResult(["ls", "dir", "appName"]);
					} else if (method === "ls") {
						resp.setResult([]);
					} else if (method === "appName") {
						resp.setResult("hscope-client");
					} else {
						debug('ERROR: ' + "Method: " + method + " is not defined.");
						resp.setError("Method: " + method + " is not defined.");
					}
					resp.setRequestId(rpc_msg.requestId());
					resp.setCallerIds(rpc_msg.callerIds());
					send_rpc_message(resp);
			},
			logDebug: debug,
			onConnected: () => {
				resolve_hscope_tree("hscope", hscope_container);
			}
		});
	} catch (exception) {
		debug('EXCEPTION: ' + exception);
	}
}

connect_websocket();
