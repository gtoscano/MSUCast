cmake_minimum_required(VERSION 3.10)

# Set the project name
project(MSUCast)

set(CMAKE_CXX_STANDARD 20)

find_package(crossguid CONFIG REQUIRED)
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

# Specify the directories where the source and header files are located
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")


set(LIB_REPORT_LOADS_HEADERS
        include/json.hpp
        include/csv.hpp
        )


set(LIB_BASE_SRC
    ${SOURCE_DIR}/scenario.cpp 
    ${SOURCE_DIR}/misc_utilities.cpp
    ${SOURCE_DIR}/amqp.cpp
    ${SOURCE_DIR}/execute.cpp
)

set(LIB_BASE_HEADERS
    ${INCLUDE_DIR}/scenario.h
    ${INCLUDE_DIR}/misc_utilities.h
    ${INCLUDE_DIR}/amqp.h
    ${INCLUDE_DIR}/execute.h
    ${INCLUDE_DIR}/json.hpp
    ${INCLUDE_DIR}/csv.hpp
)

add_library(msucast ${LIB_BASE_SRC} ${LIB_BASE_HEADERS})

target_include_directories(msucast PUBLIC include)
target_compile_features(msucast PUBLIC cxx_std_20)
target_link_libraries(msucast PRIVATE arrow parquet fmt pthread crossguid hiredis redis++ SimpleAmqpClient Threads::Threads)
# Include directories (add the path where external_archive.h is located)

include_directories(${INCLUDE_DIR})
# Add the executable with your source files



add_executable(pso
    ${SOURCE_DIR}/pso_cast.cpp 
    ${SOURCE_DIR}/pso.cpp 
    ${SOURCE_DIR}/particle.cpp 
    ${SOURCE_DIR}/external_archive.cpp
)

target_include_directories(pso PUBLIC include)
target_link_libraries(pso PRIVATE msucast arrow parquet fmt pthread crossguid hiredis redis++ SimpleAmqpClient)

add_subdirectory(test)
add_subdirectory(eps_cnstr)
