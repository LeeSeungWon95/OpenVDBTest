cmake_minimum_required(VERSION 3.20)

project(VDBTest2 LANGUAGES CXX)

# C++17 이상 권장
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# (중요) vcpkg toolchain은 보통 명령줄에서 넘깁니다.
# 여기서는 CMakeLists 안에 박아 넣는 방식도 예시로 보여드릴게요.
# 하지만 실제로는 cmake 명령에 -DCMAKE_TOOLCHAIN_FILE=... 로 주는 게 더 깔끔합니다.

# find_package(OpenVDB)를 CONFIG 모드로 찾도록 시도
find_package(OpenVDB   REQUIRED)

add_executable(VDBTest2
    src/main.cpp
)

# OpenVDB가 제공하는 CMake target을 링크
# vcpkg로 설치하면 보통 OpenVDB::openvdb 타겟이 생깁니다.
target_link_libraries(VDBTest2 PRIVATE OpenVDB::openvdb)