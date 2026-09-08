#set(SRC_LIST calluserservice.cc ../user.pb.cc)
set(SRC_LIST callfriendservice.cc ../friend.pb.cc)

add_executable(consumer ${SRC_LIST})
target_link_libraries(consumer mprpc protobuf)

add_executable(bench_echo bench_echo.cc ../friend.pb.cc)
target_link_libraries(bench_echo mprpc protobuf pthread)