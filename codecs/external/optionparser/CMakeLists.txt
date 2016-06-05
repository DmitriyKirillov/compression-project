TARGET_NAME()

TARGET_LIB (
        NAME "${TARGET_NAME}"
        SOURCES optionparser.h
)
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
