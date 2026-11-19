# No component is enabled here. INTERFACE sources enter the final executable
# only when the parent build links the corresponding interface target.
foreach(_component IN ITEMS main task_manage heartbeat audio es8388)
    add_library(application_${_component} INTERFACE)
    add_library(application::${_component} ALIAS application_${_component})
endforeach()
target_sources(application_main INTERFACE "${CMAKE_CURRENT_LIST_DIR}/task/main.c")
target_sources(application_task_manage INTERFACE "${CMAKE_CURRENT_LIST_DIR}/task/task_manage.c")
target_sources(application_heartbeat INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/heart_beat/heartbeat.c"
    "${CMAKE_CURRENT_LIST_DIR}/task/task_heartbeat.c"
)
target_sources(application_es8388 INTERFACE "${CMAKE_CURRENT_LIST_DIR}/audio/es8388/es8388.c")
target_include_directories(application_es8388 INTERFACE "${CMAKE_CURRENT_LIST_DIR}/audio/es8388")
target_sources(application_audio INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/audio/audio.c"
    "${CMAKE_CURRENT_LIST_DIR}/task/task_audio.c"
)
