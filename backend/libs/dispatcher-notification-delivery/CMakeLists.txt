add_library(dispatcher-notification-delivery
    src/alarm_notification_delivery.cpp
    src/file_notification_delivery_channel.cpp
    src/in_memory_notification_delivery_channel.cpp
    src/notification_delivery_dispatcher.cpp
    src/notification_delivery_retry.cpp
    src/notification_delivery_validator.cpp
    src/webhook_notification_delivery_channel.cpp
)

target_include_directories(dispatcher-notification-delivery
    PUBLIC
        include
)

target_compile_features(dispatcher-notification-delivery
    PUBLIC
        cxx_std_20
)