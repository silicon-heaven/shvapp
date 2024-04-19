import WsClient from "libshv-js/ws-client.ts"
import {type RpcValue, IMap, ShvMap} from "libshv-js/rpcvalue.ts"
import { RpcResponse } from "libshv-js/rpcmessage.ts";
let websocket: WsClient;
const ws_uri = "wss://nirvana.elektroline.cz:37778";

const txt_log = document.getElementById("txt_log") as HTMLTextAreaElement;
const toggle_log = () => {
    if (txt_log.className === "d-none") {
        txt_log.className = "";
        txt_log.scrollTop = txt_log.scrollHeight;
    } else {
        txt_log.className = "d-none";
    }
};

(document.getElementById("toggle_log") as HTMLButtonElement).onclick = toggle_log;

const debug = (...args: any) => {
    if (txt_log) {
        txt_log.value += args.join(" ") + "\n";
        txt_log.scrollTop = txt_log.scrollHeight;
    }
};

const format_severity = (value: string) => {
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
        (document.getElementById("sort_path") as HTMLInputElement).checked ? 1 :
        (document.getElementById("sort_severity") as HTMLInputElement).checked ? 2 :
        (document.getElementById("sort_message") as HTMLInputElement).checked ? 3 :
        1; // We'll sort by path by default.

    const order =
        (document.getElementById("sort_dsc") as HTMLInputElement).checked ? "dsc" : "asc";

    ([...document.querySelector("#hscope_container")!.querySelectorAll("tr")])
        .sort(row_comparator(col_num, order))
        .forEach((myElem: HTMLElement) => document.getElementById("hscope_container")!.appendChild(myElem));
};

type HscopeValue = {
    message: string;
    severity: string;
    time_changed: Date;
};

const as_hscope_value = (x?: RpcValue): HscopeValue | undefined => {
    if (!(x instanceof ShvMap) ||
        typeof x.value["message"] === "undefined" ||
        typeof x.value["severity"] === "undefined" ||
        !(x.value["time_changed"] instanceof Date)) {
        console.log("Unexpected value for HscopeValue", x);
        return;
    };

    return {
        message: x.value.message as string,
        severity: x.value.severity as string,
        time_changed: x.value.time_changed as Date,
    }
};

const resolve_hscope_tree = (path: string, container: HTMLElement) => {

    websocket.callRpcMethod(path, "dir").then((methods: RpcResponse) => {
        if ((methods.result as Array<RpcValue>).some(x => (x as IMap).value[1] === "run")) {
            const user_facing_path = path.replace("hscope/instances/", "");
            const node_container = document.createElement("tr");

            // Make sure the elements are sorted in the correct order.
            container.appendChild(node_container);

            const run_cell_element = document.createElement("td");
            run_cell_element.className = "align-middle";
            const run_element = document.createElement("button");
            run_element.innerText = "Run";
            run_element.className = "btn btn-outline-dark";
            run_element.onclick = () => {
                websocket.callRpcMethod(path, "run");
            };
            run_cell_element.appendChild(run_element);
            node_container.appendChild(run_cell_element);

            const path_element = document.createElement("td");
            path_element.innerText = user_facing_path;
            path_element.className = "align-middle";
            node_container.appendChild(path_element);
            const severity_element = document.createElement("td");
            severity_element.className = "text-center align-middle";
            const message_element = document.createElement("td");
            message_element.className = "text-center align-middle";
            const time_changed_element = document.createElement("td");
            time_changed_element.className = "text-center align-middle";
            const last_run_element = document.createElement("td");
            last_run_element.className = "text-center align-middle";

            let should_animate = false;
            const animate_element = (elem: HTMLElement) => {
                if (should_animate) {
                    elem.classList.add("animate-change");
                    // This dance refreshes the animation.
                    // https://stackoverflow.com/a/45036752
                    elem.style.animation = "none";
                    elem.offsetHeight;
                    elem.style.animation = "";
                    elem.onanimationend = () => {
                        elem.classList.remove("animate-change");
                    };
                }
            };

            const update_elements = (value: HscopeValue) => {
                if (typeof value.severity !== "undefined") {
                    severity_element.innerText = format_severity(value.severity);
                } else {
                    severity_element.innerText = "❓";
                }
                animate_element(severity_element);

                if (typeof value.message !== "undefined") {
                    message_element.innerText = value.message;
                } else {
                    message_element.innerText = "";
                }
                animate_element(message_element);

                if (typeof value.time_changed !== "undefined") {
                    time_changed_element.innerText = value.time_changed.toLocaleString([]);
                } else {
                    time_changed_element.innerText = "";
                }
                animate_element(time_changed_element);
                sort_rows();
            };

            const got_first_status = websocket.callRpcMethod(path + "/status", "get").then((value) => {
                const parsed = as_hscope_value(value.result);
                if (parsed !== undefined) {
                    update_elements(parsed);
                }
            });

            websocket.subscribe(path + "/status", "chng", (_changedPath, _method, value) => {
                const parsed = as_hscope_value(value);
                if (parsed !== undefined)  {
                    update_elements(parsed);
                }
            });

            const got_first_last_run = websocket.callRpcMethod(path + "/lastRunTimestamp", "get").then((value) => {
                last_run_element.innerText = value.result!.toLocaleString([]);
            });

            Promise.all([got_first_status, got_first_last_run]).then(() => {
                should_animate = true;
            });

            websocket.subscribe(path + "/lastRunTimestamp", "chng", (changedPath, _method, value) => {
                if (!(value instanceof Date)) {
                    console.log(`Got unexpected params for lastRunTimestamp on ${changedPath}`, value);
                    return;
                }
                last_run_element.innerText = value.toLocaleString([]);
                animate_element(last_run_element);
            });

            node_container.appendChild(severity_element);
            node_container.appendChild(message_element);
            node_container.appendChild(time_changed_element);
            node_container.appendChild(last_run_element);
        }
    })

    websocket.callRpcMethod(path, "ls").then((paths) => {
        (paths.result as string[]).forEach((childPath) => {
            resolve_hscope_tree(path + "/" + childPath, container);
        });
    });
};

const send_ping = () => {
    websocket.callRpcMethod(".broker/app", "ping");
};

const connect_websocket = () => {
    try {
        if (txt_log.className !== "d-none") {
            toggle_log();
        }

        document.querySelector("#hscope_container")!.innerHTML = "";

        const user = (document.getElementById("txt_user") as HTMLTextAreaElement).value;
        localStorage.setItem("lastUser", user);
        if (user === "") {
            throw new Error("SHV username mustn't be empty");
        }

        const password = (document.getElementById("txt_password") as HTMLTextAreaElement).value;
        if (password === "") {
            throw new Error("SHV password mustn't be empty");
        }
        localStorage.setItem("lastPassword", password);

        websocket = new WsClient({
            isUseCpon: true,
            user,
            password,
            wsUri: ws_uri,
            logDebug: debug,
            onConnected: () => {
                setInterval(send_ping, 1000 * 30);
                resolve_hscope_tree("hscope", document.querySelector("#hscope_container")!);
            },
            onRequest: () => {}
        });
    } catch (exception) {
        debug('EXCEPTION: ' + exception);
        if (txt_log.className === "d-none") {
            toggle_log();
        }
    }
}

const row_comparator = (col_num: number, order: string) => (a: HTMLElement, b: HTMLElement): number =>  {
    // Always put elements with empty text at the back.
    if ((a.children[col_num] as HTMLTableCellElement).innerText === "") {
        return 1;
    }

    if ((b.children[col_num] as HTMLTableCellElement).innerText === "") {
        return -1;
    }

    const left = order === "asc" ? a : b;
    const right = order === "asc" ? b : a;

    if ((left.children[col_num] as HTMLTableCellElement).innerText === (right.children[col_num] as HTMLTableCellElement).innerText) {
        // Columns have the same value, so we'll sort column 1 (which is path)
        return row_comparator(1, order)(a, b);
    }

    return (left.children[col_num] as HTMLTableCellElement).innerText < (right.children[col_num] as HTMLTableCellElement).innerText ? -1 : 1;
};

[...document.querySelectorAll("input.form-check-input")].forEach(elem => (elem as HTMLInputElement).onclick = sort_rows);

const filter_rows = (filter: string) => {
    [...document.querySelector("#hscope_container")!.querySelectorAll("tr")]
        .forEach((row) =>
            [...row.children].some((cell) => (cell as HTMLTableCellElement).innerText.match(new RegExp(filter, "i"))) ? row.classList.remove("d-none") : row.classList.add("d-none"));
}

const txt_filter = (document.getElementById("txt_filter") as HTMLTextAreaElement);
txt_filter.oninput = () => {
    filter_rows(txt_filter.value)
};

txt_filter.select();

(document.getElementById("btn_connect") as HTMLButtonElement).onclick = connect_websocket;
const last_user = localStorage.getItem("lastUser");
(document.getElementById("txt_user") as HTMLTextAreaElement).value = last_user !== null ? last_user : "";
const last_password = localStorage.getItem("lastPassword");
(document.getElementById("txt_password") as HTMLTextAreaElement).value = last_password !== null ? last_password : "";

connect_websocket();
