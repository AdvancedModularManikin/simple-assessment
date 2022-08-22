
set(
        SIMPLE_ASSESSMENT_SOURCES
        main.cpp
)

add_executable(simple_assessment ${SIMPLE_ASSESSMENT_SOURCES})

target_link_libraries(simple_assessment PUBLIC amm_std)

target_link_libraries(
        simple_assessment
        PUBLIC amm_std
        PUBLIC tinyxml2
        pthread
)

install(TARGETS simple_assessment RUNTIME DESTINATION bin)
install(DIRECTORY ../config DESTINATION bin)
