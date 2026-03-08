add_library(dsl_driver STATIC
    driver.cpp
    vhf_device.cpp
)

target_include_directories(dsl_driver PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(dsl_driver PUBLIC cxx_std_20)
