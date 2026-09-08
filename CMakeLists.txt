#设置cmake名称
cmake_minimum_required(VERSION 3.10)
project(mprpc)

#生成debug版本，可以进行gdb调试
set(CMAKE_BUILD_TYPE "Debug")
#设置可执行文件的输入路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#设置项目库文件的输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置项目编译头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/example)
include_directories(/home/wyman/package/muduo)
include_directories(/home/wyman/package/apache-zookeeper-3.9.5/zookeeper-client/zookeeper-client-c/include)
include_directories(/home/wyman/package/apache-zookeeper-3.9.5/zookeeper-client/zookeeper-client-c/generated)

#设置项目库文件的搜索路径
link_directories(${PROJECT_SOURCE_DIR}/lib)
link_directories(/home/wyman/package/muduo/build/lib)
link_directories(/home/wyman/package/apache-zookeeper-3.9.5/zookeeper-client/zookeeper-client-c/build)

#src包含了mprpc框架的所有相关代码
add_subdirectory(src)
#example包含了mprpc框架的所有示例代码
add_subdirectory(example)

