add_executable(OCASI-Tests
    "src/Test.cpp"
)

target_link_libraries(OCASI-Tests OCASI)

PostBuildCopy(OCASI-Tests ${CMAKE_CURRENT_SOURCE_DIR}/Resources $<TARGET_FILE_DIR:OCASI-Tests>/Resources)