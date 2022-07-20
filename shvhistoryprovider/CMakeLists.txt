add_executable(shvhistoryprovider
    src/appclioptions.cpp
    src/application.cpp
    src/checklogtask.cpp
    src/devicelogrequest.cpp
    src/devicemonitor.cpp
    src/dirconsistencychecktask.cpp
    src/dirtylogmanager.cpp
    src/diskcleaner.cpp
    src/getlogmerge.cpp
    src/logdir.cpp
    src/logdirreader.cpp
    src/logsanitizer.cpp
    src/main.cpp
    src/rootnode.cpp
    src/shvsubscription.cpp
    src/siteitem.cpp
    )

target_link_libraries(shvhistoryprovider Qt::Concurrent libshviotqt)
