cmake_minimum_required(VERSION "3.5")

project("ft" LANGUAGES CXX)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if(WIN32)
    set(CMAKE_CXX_FLAGS "-std=c++17 -pthread -O3 -lws2_32 -lwsock32")
elseif(UNIX)
    set(CMAKE_CXX_FLAGS "-std=c++17 -pthread -O3")
endif()

add_executable("server"
	${PROJECT_SOURCE_DIR}/src/main_server.cpp
	${PROJECT_SOURCE_DIR}/src/ft_server.cpp
)

if(WIN32)
	target_link_libraries("server" wsock32 ws2_32)
endif()

target_link_libraries("server" Threads::Threads)

target_include_directories("server"
	PUBLIC ${PROJECT_SOURCE_DIR}/include
	PUBLIC ${PROJECT_SOURCE_DIR}/asio/include
)


add_executable("client"
	${PROJECT_SOURCE_DIR}/src/main_client.cpp
	${PROJECT_SOURCE_DIR}/src/ft_client.cpp
)

if(WIN32)
	target_link_libraries("client" wsock32 ws2_32)
endif()

target_link_libraries("client" Threads::Threads)

target_include_directories("client"
	PUBLIC ${PROJECT_SOURCE_DIR}/include
	PUBLIC ${PROJECT_SOURCE_DIR}/asio/include
)
