cmake_minimum_required (VERSION 2.6)
project(audioPlayer)
include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/lib)
aux_source_directory(. SRC_LIST)
add_executable (audioPlayer ${SRC_LIST})
target_link_libraries(audioPlayer
	avcodec
	avformat
	avutil
	swresample
	EFX-Util
	OpenAL32
)