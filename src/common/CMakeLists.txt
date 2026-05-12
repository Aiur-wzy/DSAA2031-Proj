add_library(onebase_common STATIC
    config.cpp
    rid.cpp
)
target_include_directories(onebase_common PUBLIC ${PROJECT_SOURCE_DIR}/src/include)
