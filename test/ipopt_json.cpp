#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <map>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::vector<std::unordered_map<std::string, double>> read_scenarios_keyed_json(std::string filename) {
    std::vector<std::unordered_map<std::string, double>> scenarios_list;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file: "<<filename<<".\n";
        exit(-1);
    }


    json json_obj = json::parse(file);
    for (const auto &scenario_list : json_obj){
        std::unordered_map<std::string, double> parcel_map;
        for(const auto& parcel : scenario_list){
            auto key = parcel["name"].get<std::string>();
            auto amount = parcel["amount"].get<double>();
            parcel_map[key] = amount;
        }
        scenarios_list.emplace_back(parcel_map);
    }
    return scenarios_list;

}

    std::vector<std::vector<std::tuple<int, int, int, int, double>>> read_scenarios_keyed_json2(std::string filename) {
        std::vector<std::vector<std::tuple<int, int, int, int, double>>> scenarios_list;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open the file: "<<filename<<".\n";
            exit(-1);
        }
    
        std::vector<std::tuple<int, int, int, int, double>> parent_x;
    
        json json_obj = json::parse(file);
        for (const auto &scenario_list : json_obj){
            std::vector<std::tuple<int, int, int, int, double>> parcel_list;
            for(const auto& parcel : scenario_list){
                std::vector<std::string> result_vec;
                auto key = parcel["name"].get<std::string>();
                boost::split(result_vec, key, boost::is_any_of("_"));
                auto amount = parcel["amount"].get<double>();
                parcel_list.emplace_back(std::stoi(result_vec[0]), std::stoi(result_vec[1]), std::stoi(result_vec[2]), std::stoi(result_vec[3]), amount);
            }
            scenarios_list.emplace_back(parcel_list);
        }
        return scenarios_list;
    }

    std::vector<std::tuple<int, int, int, int, double>> read_scenario_json(std::string filename) {
        std::vector<std::tuple<int, int, int, int, double>> parcel_list;
    
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open the file: "<<filename<<".\n";
            exit(-1);
        }
    
        // Parse the JSON file directly into a nlohmann::json object
        json json_obj = json::parse(file);
    
        auto scenario = json_obj.get<std::unordered_map<std::string, double>>();
        for (const auto& [key, amount] : scenario) {
            std::vector<std::string> result_vec;
            boost::split(result_vec, key, boost::is_any_of("_"));
            parcel_list.emplace_back(std::stoi(result_vec[0]), std::stoi(result_vec[1]), std::stoi(result_vec[2]), std::stoi(result_vec[3]), amount);
        }
    
        return parcel_list;
    }

int main (int argc, char *argv[]) {
    
    int counter_solution = 0;
    std::string filename = argv[1];
    std::string filename2 = argv[2];

    fmt::print("{}\n", filename);
    auto scenarios_list = read_scenarios_keyed_json2(filename);
    auto parent_list = read_scenario_json(filename2);

    fmt::print("Parent\n");

    for (const auto& parcel: parent_list) {
        auto [s, h, u, b, amount] = parcel;
        fmt::print("{},{},{},{}: {}\n", s, h, u, b, amount);
    }
    fmt::print("End parent\n");


    std::unordered_map<int, int> counters;
    for (const auto& parcel_list: scenarios_list) {
        int counter = 0;
        fmt::print("Solution {}\n", counter_solution);
        auto combined = parent_list; 
        combined.insert(combined.end(), parcel_list.begin(), parcel_list.end());
        for (const auto& parcel: combined) {
            auto [s, h, u, b, amount] = parcel;
            fmt::print("{},{},{},{}: {}\n", s, h, u, b, amount);
            counter++;
        }
        counters[counter_solution] = counter;
        counter_solution++;
    }

    /*
    auto scenarios_list = read_scenarios_keyed_json(filename);

    std::unordered_map<int, int> counters;
    for (const auto& parcel_map : scenarios_list) {
        int counter = 0;
        for (const auto& [key, amount] : parcel_map) {
            fmt::print("{}: {}\n", key, amount);
            counter++;
        }
        counters[counter_solution] = counter;
        counter_solution++;
    }
    */
    for (const auto& [key, value] : counters) {
        fmt::print("{}: {}\n", key, value);
    }
    

    return 0;
}
