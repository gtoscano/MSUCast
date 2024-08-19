//
// Created by gtoscano on 4/1/23.
//

#ifndef CBO_EVALUATION_MISC_UTILITIES_H
#define CBO_EVALUATION_MISC_UTILITIES_H

#include <vector>
#include <string>
#include <tuple>
#include <nlohmann/json.hpp>
namespace misc_utilities {
    /**
    * reads environment variables
    *
    * This function reads an environment variable.
    * If the variable is not set it returns a default value
    *
    * @param key the environment variable's name
    * @param  default_value The default value
    * @return either the environment variable or the default value.
    */

    std::unordered_map<std::string, double> read_loads(std::string loads_filename);
    bool copy_prefix_in_to_prefix_out(const std::string& source, const std::string& destination, const std::string& prefix_in, const std::string& prefix_out);
    std::string change_extension(const std::string& filename, const std::string& new_extension);
    void move_files(const std::string& source_dir, const std::string& destination_dir, int n_sets, int delta_counter);

    void move_pf(const std::string& source_dir, const std::string& destination_dir, std::vector<std::string> pf_files);
    void ls_path(std::string path);

    std::tuple<std::string, std::string> extract_path_and_id(const std::string& path); 
    void merge_parquet_files(const std::string& file1, const std::string& file2, const std::string& output_file);

    std::string get_env_var(std::string const &key, std::string const &default_value);

    std::string find_file(std::string path,
                          std::string prefix);

    std::vector<std::string> find_files(std::string path, std::string prefix);
    void split_str(std::string const &str,
                   const char delim,
                   std::vector<std::string> &out);

    bool copy_full_directory(const std::string& source, const std::string& destination);
    bool copy_file(const std::string& source,
                   const std::string& destination);
    std::string current_time();

    double rand_double(double lower_bound, double upper_bound);
    void mkdir(std::string dir_path);

    nlohmann::json read_json_file(const std::string& filename);

    void write_json_file(const std::string& filename, nlohmann::json& json_obj);

    nlohmann::json merge_json(const nlohmann::json& j1, const nlohmann::json& j2);

    void merge_json_files(const std::string& parent_json_path, 
                          const std::string& current_json_path, 
                          const std::string& output_json_path);
}

#endif //CBO_EVALUATION_MISC_UTILITIES_H
