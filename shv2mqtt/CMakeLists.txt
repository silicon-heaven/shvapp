add_executable(shv2mqtt
    src/appclioptions.cpp
    src/main.cpp
    src/shv2mqttapp.cpp
    )

target_link_libraries(shv2mqtt libshviotqt Qt::Mqtt)

install(TARGETS shv2mqtt)
