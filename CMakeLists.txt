cmake_minimum_required(VERSION 3.27)
project(braw_to_dng)

set(CMAKE_CXX_STANDARD 11)

include_directories("/Applications/Blackmagic RAW/Blackmagic RAW SDK/Mac/Include")
include_directories("/Applications/Blackmagic RAW/Blackmagic RAW SDK/Mac/Libraries")

#find_library(BLACKMAGIC_RAW_API BlackmagicRawAPIDispatch)

add_executable(braw_to_dng main.cpp)

target_link_libraries(braw_to_dng "-framework CoreServices")
target_link_libraries(braw_to_dng "-framework CoreFoundation")
target_link_libraries(braw_to_dng "-framework CoreMedia")
target_link_libraries(braw_to_dng "-framework CoreGraphics")
target_link_libraries(braw_to_dng "-framework ImageIO")
#target_link_libraries(braw_to_dng ${BLACKMAGIC_RAW_API})
set_target_properties(braw_to_dng PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
target_link_libraries(braw_to_dng "/Applications/Blackmagic RAW/Blackmagic RAW SDK/Mac/Libraries/BlackmagicRawAPI.framework/BlackmagicRawAPI")