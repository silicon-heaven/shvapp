# "Complete beginner" shv tutorial
This is a tutorial on how to compile *shv*. If you've never had anything to do with *shv*, this is a good place to start. This article also includes simple examples of how to use the various utilities *shv* has.

## Dependencies
To compile *shv*, you'll need this:
- standard build suite (a C++ compiler, make, git, etc.)
- QT 5, this is the only external library needed

## Compiling
1) Clone the repository. Don't forget to clone the submodules.
```sh
$ git clone --recurse-submodules https://github.com/silicon-heaven/shvapp.git
$ mv shvapp shv
```
2) Enter the repository directory, make a build directory and enter it.
```sh
$ cd shv
$ mkdir build
$ cd build
```
3) Generate the Makefile via *qmake* and build the app.
```sh
$ cmake ..
$ make
```
For Qt5 build 
```sh
$ cmake -DUSE_QT6=OFF ..
$ make
```
For Qt5 build with custom Qt
```sh
$ cmake -DCMAKE_PREFIX_PATH:STRING=/path/to/qt/gcc_64 -DUSE_QT6=OFF ..
$ make
```

This should compile the whole project.

## Running
### The shv broker
First of all, you need to run the shv broker. To run it, you'll also need some configuration files. There's an example configuration in the `shvbroker/etc/shv/shvbroker/` directory. So, to run `shvbroker` from the build directory, you can use this command:
```sh
$ ./bin/shvbroker --config-dir ../shvbroker/etc/shv/shvbroker/
```
This runs the broker on `localhost:3755`. The `--config-dir` argument specifies the configuration directory.

### A shv client
`shvagent` is a simple client program that connects to the shv broker and has functionality for launching scripts and programs on the local device (that is, your PC). Besides the configuration (supplied by the `--config-dir` argument), you'll also need to supply a username and a password for the broker. For the default configuration, you can use `admin` for the username and `admin!123` for the password. From the build directory, you can use this command:
```sh
$ ./bin/shvagent --tester --config-dir ../shvagent/etc/shv/shvbroker/ -u admin --password 'admin!123' 
```
This runs `shvagent`, that connects to the `localhost` broker. The `--tester` argument also adds some dummy values into the shv tree, that you can play with. Use `shvagent -h` to show help for other command line arguments.

### Browsing the shv tree
The tree consists of "paths" and "methods". Paths have a filesystem-like hierarchy. Each path can also have methods. The shv protocol works by calling the methods on the paths. The methods can have "params" (in the JSON format) and can return values. Each path has two special methods:
- `ls` returns all child paths of a path
- `dir` returns all available methods of a path

It's also possible to subscribe to a path. You'll get a notification whenever there is a change the path's subtree.
TODO: explain this better

There are two simple/basic ways to explore the shv tree:
- `shvcall` is a simple CLI application, that calls a method on a path and shows the return value
- `shvspy` is a GUI app, that visualizes the shv tree, methods, subscriptions and notifications

#### shvcall
`shvcall` is an app that calls one method, shows its return value and exits.

This command prints all root-level paths of the localhost broker:
```sh
$ ./bin/shvcall -u admin --password 'admin!123' --path "/" --method "ls" 
```
This command calls the `get` method on path `test/shvagent/tester/prop1`. This prints out the value of `test/shvagent/tester/prop1`.
```sh
$ ./shvcall -u admin --password 'admin!123' --path test/shvagent/tester/prop1 --method "get"
```
This command calls the `set` method on path `test/shvagent/tester/prop1` with `123` as params. This sets the value of `test/shvagent/tester/prop1` to `123`.
```sh
$ ./shvcall -u admin --password 'admin!123' --path test/shvagent/tester/prop1 --method "set" --params 123
```

#### shvspy
`shvspy` is a GUI application that provides a convenient view of the shv tree. To build it, you can use the same procedure as with *shv*.
```sh
$ git clone --recurse-submodules https://github.com/silicon-heaven/shvspy
$ cd shvspy
$ mkdir build
$ cd build
$ qmake ..
$ make
```
This produces a `shvspy` binary inside the `bin/` directory. To run `shvspy`, simply run the binary. There's no need for a configuration file:
```sh
$ ./bin/shvpy
```

Next, you'll need to add your local broker. To do that, use the plus sign icon in the menu, which opens a form where you'll need to input:
- Name: an arbitrary name, can be anything, e.g. "my_local_broker"
- Host: `localhost`
- User: `admin`
- Password: `admin!123`
To connect to the broker, simply double-click it in the left part of the app.

The interface consists of three main boxes.

The left one shows available paths in a filesystem-like structure. You can navigate it using your mouse or keyboard arrows. The top-right box shows available methods. Top bottom-right box shows subcriptions, logs and notifications.

To call a method, firstly find the path containing the method in the left box, then find the desired method in the top-right box and double-click the green play button. You can also input params in the "Params" field. The result of the call can be seen inside the "Result" field. A method can have a description. To view it, hover over the method's name or right-click it and click "Method description".

To subscribe a path, find it in the left box, right click it and click "Subscribe". You'll then see the subcriptions and notifications in in the bottom-left box.
