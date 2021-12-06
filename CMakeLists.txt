cmake_minimum_required(VERSION "3.5")

project("my_app")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if(WIN32)
    set(CMAKE_CXX_FLAGS "-std=c++17 -pthread -O3 -lws2_32 -lwsock32")
elseif(UNIX)
    set(CMAKE_CXX_FLAGS "-std=c++17 -pthread -O3")
endif()

add_executable("my_app"
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${PROJECT_SOURCE_DIR}/src/ft_server.cpp
	${PROJECT_SOURCE_DIR}/src/ft_client.cpp
)

if(WIN32)
	target_link_libraries("my_app" wsock32 ws2_32)
	target_link_libraries("my_app" Threads::Threads)
endif()

target_include_directories("my_app"
	PUBLIC ${PROJECT_SOURCE_DIR}/include
	PUBLIC ${PROJECT_SOURCE_DIR}/asio/include
)
