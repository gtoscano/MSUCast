# find Boost
pkg_check_modules(IPOPT REQUIRED IMPORTED_TARGET ipopt)
find_package(Boost 1.74.0 COMPONENTS log log_setup REQUIRED)

add_executable(eps_cnstr
    ${CMAKE_CURRENT_SOURCE_DIR}/nlp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/eps_cnstr.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/eps_cnstr_main.cpp 
    )


target_include_directories(eps_cnstr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(eps_cnstr PRIVATE msucast arrow parquet fmt pthread crossguid hiredis redis++ SimpleAmqpClient PkgConfig::IPOPT)

# Specify the installation rules for the executable
install(TARGETS eps_cnstr
    RUNTIME DESTINATION bin
)
