
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <unordered_map>

#include <crossguid/guid.hpp>
#include "json.hpp"

#include "pso.h"
#include "scenario.h"
#include "misc_utilities.h"

using json = nlohmann::json;


int main (int argc, char *argv[]) {
    std::string filename = "prueba.json";
    std::string filename_scenario = "scenario_prueba.json";
    Scenario scenario;
    bool is_ef_enabled = true;
    bool is_lc_enabled = true;
    bool is_animal_enabled = true;
    bool is_manure_enabled = true;
    std::string emo_uuid = xg::newGuid().str();
    fmt::print("emo_uuid: {}\n", emo_uuid);
    std::string emo_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", emo_uuid);
    std::unordered_map<std::string, std::tuple<int, double, double , double, double>> generation_fx;//key: UID, tuple: [idx, Nitrogen, Phosphorus, Sediments]
    std::unordered_map<std::string, int> generation_uuid_idx;
    misc_utilities::mkdir(emo_path);



    std::string manure_nutrients_file = "manure_nutrients.json";
    scenario.init(filename, filename_scenario, is_ef_enabled, is_lc_enabled, is_animal_enabled, is_manure_enabled, manure_nutrients_file);
    int lc_size = scenario.get_lc_size();
    int animal_size = scenario.get_animal_size();
    std::vector<double > x(lc_size + animal_size);
    scenario.initialize_vector(x);
    
    std::vector<std::tuple<int, int, int, int, double>> lc_x;
    std::unordered_map<std::string, double> amount_minus;
    std::unordered_map<std::string, double> amount_plus;

    auto total_cost = scenario.normalize_lc(x, lc_x, amount_minus, amount_plus);

    std::vector<std::string> exec_uuid_vec;

    std::string exec_uuid = xg::newGuid().str();
    
    exec_uuid_vec.push_back(exec_uuid);
    fmt::print("exec_uuid: {}\n", exec_uuid);  
    auto land_filename = fmt::format("{}/{}_impbmpsubmittedland.parquet", emo_path, exec_uuid);
    scenario.write_land(lc_x, land_filename);
    std::vector<std::tuple<int, int, int, int, int, double>> animal_x;
    total_cost += scenario.normalize_animal(x, animal_x); 
    auto animal_filename = fmt::format("{}/{}_impbmpsubmittedanimal.parquet", emo_path, exec_uuid);
    scenario.write_animal(animal_x, animal_filename);
    generation_uuid_idx[exec_uuid] = 0;

    auto results = scenario.send_files(emo_uuid, exec_uuid_vec);


    std::vector<std::string> result_vec;
    for (const auto& result : results) {
        result_vec.clear();
        misc_utilities::split_str(result, '_', result_vec);
        auto stored_idx = generation_uuid_idx[result_vec[0]];
        generation_fx[result_vec[0]] = std::make_tuple(stored_idx, total_cost, std::stod(result_vec[1]), std::stod(result_vec[2]), std::stod(result_vec[3]));
    } 
    for (const auto& [key, val] : generation_fx) {
        fmt::print("{}: [{}, {}, {}, {}, {}]\n", key, std::get<0>(val), std::get<1>(val), std::get<2>(val), std::get<3>(val), std::get<4>(val));
    }


    return 0;
}

