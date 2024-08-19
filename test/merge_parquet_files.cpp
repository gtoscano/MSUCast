#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#include <parquet/arrow/reader.h>
#include <iostream>
#include <misc_utilities.h>


int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_parquet_file1> <input_parquet_file2> <output_parquet_file>" << std::endl;
        return 1;
    }

    const std::string file1 = argv[1];
    const std::string file2 = argv[2];
    const std::string output_file = argv[3];
    
    misc_utilities::merge_parquet_files(file1, file2, output_file);
    
    std::cout << "Merged " << file1 << " and " << file2 << " into " << output_file << std::endl;

    return 0;
}
