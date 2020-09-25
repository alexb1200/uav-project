cmake_minimum_required(VERSION 3.0.0)
project(uav VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(Threads REQUIRED)


find_package(MAVSDK REQUIRED)
add_subdirectory( ${CMAKE_SOURCE_DIR}/libs/pybind11)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    MAVSDK::mavsdk_telemetry
    MAVSDK::mavsdk_action
    MAVSDK::mavsdk_follow_me
    MAVSDK::mavsdk
    ${CMAKE_THREAD_LIBS_INIT}
)



target_link_libraries(${PROJECT_NAME} pybind11::embed)




set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/pythonCode $<TARGET_FILE_DIR:${PROJECT_NAME}>
)


