import {WsClient, DIR_NAME} from 'libshv-js/ws-client.ts';
import {ShvMap} from 'libshv-js/rpcvalue.ts';
import {type RpcResponse, RpcError} from 'libshv-js/rpcmessage.ts';

let websocket: WsClient;
const ws_uri = 'wss://nirvana.elektroline.cz:37778';

const txt_log = document.querySelector('#txt_log') as HTMLTextAreaElement;
const toggle_log = () => {
    if (txt_log.className === 'd-none') {
        txt_log.className = '';
        txt_log.scrollTop = txt_log.scrollHeight;
    } else {
        txt_log.className = 'd-none';
    }
};

(document.querySelector('#toggle_log') as HTMLButtonElement).addEventListener('click', toggle_log);

const debug = (...args: string[]) => {
    if (txt_log) {
        (txt_log).value += args.join(' ') + '\n';
        txt_log.scrollTop = txt_log.scrollHeight;
    }
};

const format_severity = (value: string) => {
    switch (value) {
        case 'ok':
            return '🟢';
        case 'warn':
            return '🟠';
        case 'error':
            return '🔴';
        default:
            return 'unknown severity';
    }
};

const sort_rows = () => {
    const col_num
        = (document.querySelector('#sort_path') as HTMLInputElement).checked ? 1
            : (document.querySelector('#sort_severity') as HTMLInputElement).checked ? 2
                : (document.querySelector('#sort_message') as HTMLInputElement).checked ? 3
                    : 1; // We'll sort by path by default.

    const order
        = (document.querySelector('#sort_dsc') as HTMLInputElement).checked ? 'dsc' : 'asc';

    ([...document.querySelector('#hscope_container')!.querySelectorAll('tr')])
        .sort(row_comparator(col_num, order))
        .forEach((myElem: HTMLElement) => {
            document.querySelector('#hscope_container')!.append(myElem);
        });
};

type HscopeValue = {
    message: string;
    severity: string;
    time_changed: Date;
};

const is_hscope_value = (x: RpcResponse): x is ShvMap<HscopeValue> => {
    if (!(x instanceof ShvMap)
        || x.value.message === undefined
        || x.value.severity === undefined
        || !(x.value.time_changed instanceof Date)) {
        console.log('Unexpected value for HscopeValue', x);
        return false;
    }

    return true;
};

const resolve_hscope_tree = (path: string, container: HTMLElement) => {
    websocket.callRpcMethod(path, 'dir').then((methods): undefined => {
        if (methods instanceof RpcError) {
            console.log(`Couldn't retrieve methods for ${path}: ${methods.message()}`);
            return;
        }

        if (methods.some(x => x.value[DIR_NAME] === 'run')) {
            const user_facing_path = path.replace('hscope/instances/', '');
            const node_container = document.createElement('tr');

            // Make sure the elements are sorted in the correct order.
            container.append(node_container);

            const run_cell_element = document.createElement('td');
            run_cell_element.className = 'align-middle';
            const run_element = document.createElement('button');
            run_element.textContent = 'Run';
            run_element.className = 'btn btn-outline-dark';
            run_element.addEventListener('click', () => {
                websocket.callRpcMethod(path, 'run').catch(error => {
                    console.log(`Failed to run '${path}': ${error}`);
                });
            });
            run_cell_element.append(run_element);
            node_container.append(run_cell_element);

            const path_element = document.createElement('td');
            path_element.textContent = user_facing_path;
            path_element.className = 'align-middle';
            node_container.append(path_element);
            const severity_element = document.createElement('td');
            severity_element.className = 'text-center align-middle';
            const message_element = document.createElement('td');
            message_element.className = 'text-center align-middle';
            const time_changed_element = document.createElement('td');
            time_changed_element.className = 'text-center align-middle';
            const last_run_element = document.createElement('td');
            last_run_element.className = 'text-center align-middle';

            let should_animate = false;
            const animate_element = (elem: HTMLElement) => {
                if (should_animate) {
                    elem.classList.add('animate-change');
                    // This dance refreshes the animation.
                    // https://stackoverflow.com/a/45036752
                    elem.style.animation = 'none';
                    // eslint-disable-next-line @typescript-eslint/no-unused-expressions
                    elem.offsetHeight;
                    elem.style.animation = '';
                    elem.addEventListener('animationend', () => {
                        elem.classList.remove('animate-change');
                    });
                }
            };

            const update_elements = (value: HscopeValue) => {
                severity_element.textContent = value.severity !== undefined ? format_severity(value.severity) : '❓';
                animate_element(severity_element);

                message_element.textContent = value.message ?? '';
                animate_element(message_element);

                time_changed_element.textContent = value.time_changed !== undefined ? value.time_changed.toLocaleString([]) : '';
                animate_element(time_changed_element);
                sort_rows();
            };

            const got_first_status = websocket.callRpcMethod(path + '/status', 'get').then(value => {
                if (is_hscope_value(value)) {
                    update_elements(value.value);
                }
            });

            websocket.subscribe(path + '/status', 'chng', (_changedPath, _method, value) => {
                if (is_hscope_value(value)) {
                    update_elements(value.value);
                }
            });

            const got_first_last_run = websocket.callRpcMethod(path + '/lastRunTimestamp', 'get').then(value => {
                last_run_element.textContent = value instanceof Date ? value.toLocaleString([]) : 'Couldn\'t retrieve timestamp';
            });

            void Promise.all([got_first_status, got_first_last_run]).then(() => {
                should_animate = true;
            });

            setTimeout(() => {
                websocket.subscribe(path + '/lastRunTimestamp', 'chng', (changedPath, _method, value) => {
                    if (!(value instanceof Date)) {
                        console.log(`Got unexpected params for lastRunTimestamp on ${changedPath}`, value);
                        return;
                    }
                    last_run_element.textContent = value.toLocaleString([]);
                    animate_element(last_run_element);
                });
            }, 5000);

            node_container.append(severity_element);
            node_container.append(message_element);
            node_container.append(time_changed_element);
            node_container.append(last_run_element);
        }
    }).catch(error => {
        console.log(`Failed to fetch methods for '${path}': ${error}`);
    });

    websocket.callRpcMethod(path, 'ls').then(paths => {
        if (paths instanceof RpcError) {
            console.log(`Couldn't retrieve child paths for ${path}: ${paths.message()}`);
            return;
        }
        for (const childPath of paths) {
            resolve_hscope_tree(path + '/' + childPath, container);
        }
    }).catch(error => {
        console.log(`Failed to child nodes for '${path}': ${error}`);
    });
};

const send_ping = () => {
    websocket.callRpcMethod('.broker/app', 'ping')
        .catch(error => {
            console.log(`Failed to send ping: ${error}`);
        });
};

const connect_websocket = () => {
    try {
        if (txt_log.className !== 'd-none') {
            toggle_log();
        }

        document.querySelector('#hscope_container')!.innerHTML = '';

        const user = (document.querySelector('#txt_user') as HTMLInputElement).value;
        localStorage.setItem('lastUser', user);
        if (user === '') {
            throw new Error('SHV username mustn\'t be empty');
        }

        const password = (document.querySelector('#txt_password') as HTMLInputElement).value;
        if (password === '') {
            throw new Error('SHV password mustn\'t be empty');
        }
        localStorage.setItem('lastPassword', password);

        websocket = new WsClient({
            user,
            password,
            wsUri: ws_uri,
            logDebug: debug,
            onConnected() {
                setInterval(send_ping, 1000 * 30);
                resolve_hscope_tree('hscope', document.querySelector('#hscope_container')!);
            },
            onRequest() {/* empty */},
        });
    } catch (error) {
        debug('EXCEPTION: ' + String(error));
        if (txt_log.className === 'd-none') {
            toggle_log();
        }
    }
};

const row_comparator = (col_num: number, order: string) => (a: HTMLElement, b: HTMLElement): number => {
    // Always put elements with empty text at the back.
    if ((a.children[col_num] as HTMLTableCellElement).textContent === '') {
        return 1;
    }

    if ((b.children[col_num] as HTMLTableCellElement).textContent === '') {
        return -1;
    }

    const left = order === 'asc' ? a : b;
    const right = order === 'asc' ? b : a;

    if ((left.children[col_num] as HTMLTableCellElement).textContent === (right.children[col_num] as HTMLTableCellElement).textContent) {
        // Columns have the same value, so we'll sort column 1 (which is path)
        return row_comparator(1, order)(a, b);
    }

    return ((left.children[col_num] as HTMLTableCellElement).textContent ?? '') < ((right.children[col_num] as HTMLTableCellElement).textContent ?? '') ? -1 : 1;
};

for (const elem of document.querySelectorAll('input.form-check-input')) {
    (elem as HTMLInputElement).addEventListener('click', sort_rows);
}

const filter_rows = (filter: string) => {
    for (const row of document.querySelector('#hscope_container')!.querySelectorAll('tr')) {
        if ([...row.children].some(cell => new RegExp(filter, 'i').exec((cell as HTMLTableCellElement).textContent ?? ''))) {
            row.classList.remove('d-none');
        } else {
            row.classList.add('d-none');
        }
    }
};

const txt_filter = (document.querySelector('#txt_filter') as HTMLTextAreaElement);
txt_filter.addEventListener('input', () => {
    filter_rows(txt_filter.value);
});

txt_filter.select();

(document.querySelector('#btn_connect')!).addEventListener('click', connect_websocket);
const last_user = localStorage.getItem('lastUser');
(document.querySelector('#txt_user') as HTMLInputElement).value = last_user ?? '';
const last_password = localStorage.getItem('lastPassword');
(document.querySelector('#txt_password') as HTMLInputElement).value = last_password ?? '';

connect_websocket();
