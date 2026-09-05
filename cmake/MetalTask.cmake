function(add_mlxgym_task TASK_NAME)
  set(TASK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  set(AIR_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TASK_NAME}.air)
  set(METALLIB_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TASK_NAME}.metallib)
  set(METAL_FLAGS -std=metal3.2 -Wall)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND METAL_FLAGS -gline-tables-only -frecord-sources)
  else()
    list(APPEND METAL_FLAGS -O3)
  endif()

  add_custom_command(OUTPUT ${AIR_FILE}
    COMMAND ${CMAKE_COMMAND} -E env "TOOLCHAINS=${METAL_TOOLCHAIN_ID}"
            ${XCRUN_EXECUTABLE} -sdk macosx metal ${METAL_FLAGS}
            -c ${TASK_SOURCE_DIR}/kernel.metal -o ${AIR_FILE}
    DEPENDS ${TASK_SOURCE_DIR}/kernel.metal
    COMMENT "Compiling ${TASK_NAME} Metal source")
  add_custom_command(OUTPUT ${METALLIB_FILE}
    COMMAND ${CMAKE_COMMAND} -E env "TOOLCHAINS=${METAL_TOOLCHAIN_ID}"
            ${XCRUN_EXECUTABLE} -sdk macosx metallib ${AIR_FILE} -o ${METALLIB_FILE}
    DEPENDS ${AIR_FILE}
    COMMENT "Linking ${TASK_NAME}.metallib")
  add_custom_target(${TASK_NAME}_shader DEPENDS ${METALLIB_FILE})

  add_executable(${TASK_NAME} ${PROJECT_SOURCE_DIR}/framework/src/main.cpp solution.mm)
  add_dependencies(${TASK_NAME} ${TASK_NAME}_shader)
  target_link_libraries(${TASK_NAME} PRIVATE mlxgym_framework)
  target_compile_options(${TASK_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc;-Wall;-Wextra;-Wpedantic>)
  target_compile_definitions(${TASK_NAME} PRIVATE
    MLXGYM_TASK_NAME="${TASK_NAME}"
    MLXGYM_METALLIB_PATH="${METALLIB_FILE}")
  add_test(NAME ${TASK_NAME} COMMAND ${TASK_NAME})
endfunction()
