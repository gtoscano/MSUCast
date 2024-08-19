# find Boost
pkg_check_modules(IPOPT REQUIRED IMPORTED_TARGET ipopt)
find_package(Boost 1.74.0 COMPONENTS log log_setup REQUIRED)

add_executable(external_archive_test 
    external_archive_test.cpp
    ${SOURCE_DIR}/external_archive.cpp
    ${SOURCE_DIR}/particle.cpp 
    )


add_executable(execute 
    exec_test.cpp 
    )

add_executable(merge_parquet_files
    merge_parquet_files.cpp 
    ${SOURCE_DIR}/misc_utilities.cpp
    )

add_executable(merge_csv_files
    merge_csv_files.cpp 
    )

add_executable(pso_test
    pso_test.cpp 
    ${SOURCE_DIR}/external_archive.cpp
    ${SOURCE_DIR}/particle.cpp 
    ${SOURCE_DIR}/pso.cpp 
    )

add_executable(ipopt_json_test
    ipopt_json.cpp 
)

add_executable(scenario_test
    scenario_test.cpp 
)

target_link_libraries(merge_parquet_files PRIVATE arrow parquet fmt pthread)

target_link_libraries(merge_csv_files PRIVATE arrow parquet fmt pthread)

target_link_libraries(ipopt_json_test PRIVATE msucast arrow parquet fmt pthread crossguid hiredis redis++ SimpleAmqpClient)

target_link_libraries(pso_test PRIVATE arrow msucast parquet fmt pthread crossguid hiredis redis++ SimpleAmqpClient)

target_link_libraries(scenario_test PRIVATE msucast arrow parquet fmt pthread crossguid hiredis redis++ SimpleAmqpClient) 

target_link_libraries(external_archive_test PRIVATE msucast fmt crossguid pthread) 

target_link_libraries(execute PRIVATE msucast fmt crossguid pthread) 
