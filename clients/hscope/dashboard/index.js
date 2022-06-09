let websocket = null;
const ws_uri = "wss://nirvana.elektroline.cz:3778";
const user = "hscope";
const password = "holyshit!";

const txt_log = document.getElementById("txt_log");
document.getElementById("toggle_log").onclick = () => {
	if (txt_log.className === "hide") {
		txt_log.className = "";
		txt_log.scrollTop = txt_log.scrollHeight;
	} else {
		txt_log.className = "hide";
	}
};

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

const sort_rows = () => {
	const col_num =
		document.getElementById("sort_path").checked ? 1 :
		document.getElementById("sort_severity").checked ? 2 :
		document.getElementById("sort_message").checked ? 3 :
		1; // We'll sort by path by default.

	const order =
		document.getElementById("sort_dsc").checked ? "dsc" : "asc";

	[...document.querySelector("#hscope_container").querySelectorAll("tr")]
		.sort(row_comparator(col_num, order))
		.forEach((myElem) => document.getElementById("hscope_container").appendChild(myElem));
};

const resolve_hscope_tree = (path, container) => {

	websocket.callRpcMethod(path, "dir").then((methods) => {
		if (methods.rpcValue.value[2].value.some(x => x.value === "run")) {
			const user_facing_path = path.replace("hscope/instances/", "");
			const node_container = document.createElement("tr");

			// Make sure the elements are sorted in the correct order.
			container.appendChild(node_container);

			const run_cell_element = document.createElement("td");
			run_cell_element.style.textAlign = "center";
			const run_element = document.createElement("button");
			run_element.innerText = "Run";
			run_element.onclick = () => {
				websocket.callRpcMethod(path, "run");
			};
			run_cell_element.appendChild(run_element);
			node_container.appendChild(run_cell_element);

			const path_element = document.createElement("td");
			path_element.innerText = user_facing_path;
			node_container.appendChild(path_element);
			const severity_element = document.createElement("td");
			severity_element.className = "center-text";
			const message_element = document.createElement("td");
			message_element.className = "center-text";
			const time_changed_element = document.createElement("td");
			time_changed_element.className = "center-text";
			const last_run_element = document.createElement("td");
			last_run_element.className = "center-text";

			let should_animate = false;
			const animate_element = (elem) => {
				if (should_animate) {
					elem.classList.add("animate-change");
					// This dance refreshes the animation.
					// https://stackoverflow.com/a/45036752
					elem.style.animation = "none";
					elem.offsetHeight;
					elem.style.animation = null;
					elem.onanimationend = () => {
						elem.classList.remove("animate-change");
					};
				}
			};

			const update_elements = (value) => {
				if (typeof value.severity !== "undefined") {
					severity_element.innerText = format_severity(value.severity.value);
				} else {
					severity_element.innerText = "❓";
				}
				animate_element(severity_element);

				if (typeof value.message !== "undefined") {
					message_element.innerText = value.message.value;
				} else {
					message_element.innerText = "";
				}
				animate_element(message_element);

				if (typeof value.time_changed !== "undefined") {
					time_changed_element.innerText = new Date(value.time_changed.value.epochMsec).toLocaleString([]);
				} else {
					time_changed_element.innerText = "";
				}
				animate_element(time_changed_element);
				sort_rows();
			};

			const got_first_status = websocket.callRpcMethod(path + "/status", "get").then((value) => {
				update_elements(value.rpcValue.value[2].value);
			});

			websocket.subscribe(path + "/status", "chng", (changedPath, type, value) => {
				update_elements(value[1].value);
			});

			const got_first_last_run = websocket.callRpcMethod(path + "/lastRunTimestamp", "get").then((value) => {
				last_run_element.innerText = new Date(value.rpcValue.value[2].value.epochMsec).toLocaleString([]);
			});

			Promise.all([got_first_status, got_first_last_run]).then(() => {
				should_animate = true;
			});

			websocket.subscribe(path + "/lastRunTimestamp", "chng", (changedPath, type, value) => {
				last_run_element.innerText = new Date(value[1].value.epochMsec).toLocaleString([]);
				animate_element(last_run_element);
			});

			node_container.appendChild(severity_element);
			node_container.appendChild(message_element);
			node_container.appendChild(time_changed_element);
			node_container.appendChild(last_run_element);
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
				setInterval(send_ping, 1000 * 30);
				resolve_hscope_tree("hscope", hscope_container);
			}
		});
	} catch (exception) {
		debug('EXCEPTION: ' + exception);
	}
}

const row_comparator = (col_num, order) => (a, b) =>  {
	// Always put elements with empty text at the back.
	if (a.children[col_num].innerText === "") {
		return 1;
	}

	if (b.children[col_num].innerText === "") {
		return -1;
	}

	const left = order === "asc" ? a : b;
	const right = order === "asc" ? b : a;

	if (left.children[col_num].innerText === right.children[col_num].innerText) {
		// Columns have the same value, so we'll sort column 1 (which is path)
		return row_comparator(1, order)(a, b);
	}

	return left.children[col_num].innerText < right.children[col_num].innerText ? -1 : 1;
};

[...document.querySelectorAll("#radio_buttons > input")].forEach(elem => elem.onclick = sort_rows);

const filter_rows = (filter) => {
	[...document.querySelector("#hscope_container").querySelectorAll("tr")]
		.forEach((row) =>
			[...row.children].some((cell) => cell.innerText.match(filter)) ? row.classList.remove("hide") : row.classList.add("hide"));
}

const txt_filter = document.getElementById("txt_filter");
txt_filter.oninput = () => {
	filter_rows(txt_filter.value)
};

txt_filter.select();

connect_websocket();
