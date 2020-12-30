cmake_minimum_required(VERSION 3.17)
project(tau_os_ex4 C)

set(CMAKE_C_STANDARD 11)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_executable(tau_os_ex4 pfind.c)
#target_link_libraries(tau_os_ex4 PRIVATE Threads::Threads -fsanitize=thread )
target_link_libraries(tau_os_ex4 PRIVATE Threads::Threads  )
