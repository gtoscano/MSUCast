#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Function to read the contents of a CSV file into a vector of strings
std::vector<std::string> read_csv(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::string> lines;
    std::string line;

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    file.close();
    return lines;
}

// Function to append two CSV files with headers
void append_csv_files(const std::string& file1, const std::string& file2, const std::string& output_file) {
    // Read the contents of the first CSV file
    std::vector<std::string> lines1 = read_csv(file1);

    // Read the contents of the second CSV file
    std::vector<std::string> lines2 = read_csv(file2);

    // Check if both files have at least one line (header)
    if (lines1.empty() || lines2.empty()) {
        throw std::runtime_error("One of the files is empty or does not contain a header.");
    }

    // Check if headers match
    if (lines1.front() != lines2.front()) {
        throw std::runtime_error("Headers do not match between the two files.");
    }

    // Write the combined contents to the output file
    std::ofstream outfile(output_file);
    if (!outfile.is_open()) {
        throw std::runtime_error("Could not open output file: " + output_file);
    }

    // Write the header
    outfile << lines1.front() << '\n';

    // Write the content of the first file (excluding the header)
    for (size_t i = 1; i < lines1.size(); ++i) {
        outfile << lines1[i] << '\n';
    }

    // Write the content of the second file (excluding the header)
    for (size_t i = 1; i < lines2.size(); ++i) {
        outfile << lines2[i] << '\n';
    }

    outfile.close();
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_csv_file1> <input_csv_file2> <output_csv_file>" << std::endl;
        return 1;
    }

    const std::string file1 = argv[1];
    const std::string file2 = argv[2];
    const std::string output_file = argv[3];

    try {
        append_csv_files(file1, file2, output_file);
        std::cout << "Appended " << file1 << " and " << file2 << " into " << output_file << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
