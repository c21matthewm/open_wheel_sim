foreach(REQUIRED_VARIABLE APP_EXECUTABLE SDL_SOURCE SDL_BUNDLED)
    if(NOT DEFINED ${REQUIRED_VARIABLE})
        message(FATAL_ERROR "BundleSDL3.cmake requires ${REQUIRED_VARIABLE}")
    endif()
endforeach()

execute_process(
    COMMAND /usr/bin/otool -D "${SDL_SOURCE}"
    OUTPUT_VARIABLE SDL_ID_OUTPUT
    RESULT_VARIABLE OTOOL_RESULT
)
if(NOT OTOOL_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not read SDL3 install name from ${SDL_SOURCE}")
endif()

string(REPLACE "\n" ";" SDL_ID_LINES "${SDL_ID_OUTPUT}")
list(FILTER SDL_ID_LINES EXCLUDE REGEX "^$")
list(POP_BACK SDL_ID_LINES SDL_INSTALL_NAME)
string(STRIP "${SDL_INSTALL_NAME}" SDL_INSTALL_NAME)

execute_process(
    COMMAND /usr/bin/install_name_tool -id "@rpath/libSDL3.0.dylib" "${SDL_BUNDLED}"
    RESULT_VARIABLE SET_ID_RESULT
)
if(NOT SET_ID_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not set bundled SDL3 install name")
endif()

execute_process(
    COMMAND /usr/bin/install_name_tool -change
        "${SDL_INSTALL_NAME}"
        "@rpath/libSDL3.0.dylib"
        "${APP_EXECUTABLE}"
    RESULT_VARIABLE CHANGE_ID_RESULT
)
if(NOT CHANGE_ID_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not update the app's SDL3 install name")
endif()
