set(PROJECTS
    JUCE
)

set(JUCE_GIT_REPOSITORY https://github.com/juce-framework/JUCE.git)
set(JUCE_GIT_BRANCH develop)

if(NOT CMAKE_INSTALL_PREFIX)
    message(FATAL_ERROR "Missing CMAKE_INSTALL_PREFIX")
endif()
if(NOT BUILD_DIR)
    message(FATAL_ERROR "Missing CMAKE_INSTALL_PREFIX")
endif()
if(EXISTS ${BUILD_DIR})
    if(NOT IS_DIRECTORY ${BUILD_DIR})
        message(FATAL_ERROR "BUILD_DIR is not a directory: ${BUILD_DIR}")
    endif()
else()
    file(MAKE_DIRECTORY ${BUILD_DIR})
endif()

find_package(Git REQUIRED)

# Helper macros to execute cmake commands.
macro(cmake)
    execute_process(COMMAND ${CMAKE_COMMAND} ${ARGV}
        COMMAND_ERROR_IS_FATAL ANY)
endmacro()

foreach(project IN LISTS PROJECTS)
    set(s ${BUILD_DIR}/${project}/s)
    if(NOT IS_DIRECTORY ${s})
        if(DEFINED ${project}_GIT_BRANCH)
            set(git_branch_option --branch ${${project}_GIT_BRANCH})
        endif()
        execute_process(
            COMMAND ${GIT_EXECUTABLE}
                clone --depth 1 ${git_branch_option}
                    ${${project}_GIT_REPOSITORY} ${s}
            COMMAND_ERROR_IS_FATAL ANY
        )
    else()
        message(STATUS "Not cloning ${project}, directory exists")
    endif()
endforeach()

foreach(project IN LISTS PROJECTS)
    foreach(config Debug Release)
        set(s ${BUILD_DIR}/${project}/s)
        set(b ${BUILD_DIR}/${project}/${config})
        message(STATUS "[${project}]: calling cmake-config ${config}")
        cmake(-S ${s} -B ${b}
            -D CMAKE_BUILD_TYPE=${config}
            -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
            -D CMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
            -D BUILD_SHARED_LIBS=0
            -D CMAKE_DEBUG_POSTFIX=_d
            -D CMAKE_CXX_STANDARD=23
            -D CMAKE_CXX_STANDARD_REQUIRED=1
            -D CMAKE_FIND_PACKAGE_PREFER_CONFIG=1
            -D BUILD_TESTING=0
        )
        message(STATUS "[${project}]: calling cmake-build ${config}")
        cmake(--build ${b} --target install --config Debug -j)
    endforeach()
endforeach()