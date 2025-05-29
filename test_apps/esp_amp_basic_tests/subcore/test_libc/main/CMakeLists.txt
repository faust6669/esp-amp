idf_component_register(
    SRCS main.cpp
    REQUIRES esp_amp
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=use-after-free)
