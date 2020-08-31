cmake_minimum_required(VERSION 3.4.1)
file(GLOB my_source src/main/cpp/*.cpp src/main/cpp/*.c)
set(distribution_DIR ../../../../libs)

add_library( native-lib
             SHARED
             ${my_source}
              )
add_library( avcodec
             SHARED
             IMPORTED)
set_target_properties( avcodec
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libavcodec.so)
add_library( avfilter
             SHARED
             IMPORTED)
set_target_properties( avfilter
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libavfilter.so)

add_library( avformat
             SHARED
             IMPORTED)
set_target_properties( avformat
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libavformat.so)

add_library( avutil
             SHARED
             IMPORTED)
set_target_properties( avutil
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libavutil.so)

add_library( swresample
             SHARED
             IMPORTED)
set_target_properties( swresample
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libswresample.so)

add_library( swscale
             SHARED
             IMPORTED)
set_target_properties( swscale
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libswscale.so)
include_directories(libs/include)
target_link_libraries( native-lib
                       avcodec
                       avfilter
                       avformat
                       avutil
                       swresample
                       swscale
                       log
                       android
                       OpenSLES
                        )