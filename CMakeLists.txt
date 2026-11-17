# No component is enabled here. INTERFACE sources enter the final executable
# only when the parent build links the corresponding interface target.
foreach(_component IN ITEMS main task_manage heartbeat)
    add_library(application_${_component} INTERFACE)
    add_library(application::${_component} ALIAS application_${_component})
endforeach()
target_sources(application_main INTERFACE "${CMAKE_CURRENT_LIST_DIR}/main.c")
target_sources(application_task_manage INTERFACE "${CMAKE_CURRENT_LIST_DIR}/task/task_manage.c")
target_sources(application_heartbeat INTERFACE "${CMAKE_CURRENT_LIST_DIR}/task/task_heartbeat.c")
