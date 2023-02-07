set(EXE_TARGET_NAME spring)

set(${EXE_TARGET_NAME}_SRC
        main.cpp
)

set(${EXE_TARGET_NAME}_HDR
)

add_executable(${EXE_TARGET_NAME} ${${EXE_TARGET_NAME}_SRC} ${${EXE_TARGET_NAME}_HDR})

target_include_directories(${EXE_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${EXE_TARGET_NAME} libscip)

install(TARGETS ${EXE_TARGET_NAME} DESTINATION bin)
