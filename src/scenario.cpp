// Created by Gregorio Toscano

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "csv.hpp"
#include "pso.h"
#include "json.hpp"
#include <fmt/core.h>
#include <unordered_map>
#include <regex>

#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <parquet/exception.h>
#include <parquet/stream_writer.h>

#include <arrow/api.h>
#include <arrow/io/file.h>
#include <arrow/io/memory.h>
#include <arrow/ipc/reader.h>
#include <arrow/status.h>
#include <arrow/type.h>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <arrow/csv/writer.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/table.h>
#include <arrow/compute/api_aggregate.h>
#include <boost/algorithm/string.hpp>   

#include <vector>
#include <tuple>
#include <memory>

#include "amqp.h"
#include "misc_utilities.h"
#include "scenario.h"

using json = nlohmann::json;


const double MIN_LC_THRESHOLD = 10.0;
const double MAX_PCT_LC_BMP = 0.30;
const double MAX_PCT_ANIMAL_BMP = 0.30;
const double MAX_PCT_MANURE_BMP = 0.30;


namespace {
    std::string REDIS_HOST = misc_utilities::get_env_var("REDIS_HOST", "127.0.0.1");
    std::string REDIS_PORT = misc_utilities::get_env_var("REDIS_PORT", "6379");
    std::string REDIS_DB_OPT = misc_utilities::get_env_var("REDIS_DB_OPT", "1");
    std::string REDIS_URL = fmt::format("tcp://{}:{}/{}", REDIS_HOST, REDIS_PORT, REDIS_DB_OPT);

}


Scenario::Scenario() {
    is_ef_enabled = false;
    is_lc_enabled = false;
    is_animal_enabled = false;
    is_manure_enabled = false;
    load_to_opt_ = 0;
    lc_size_ = 0;
    animal_size_ = 0;
    manure_size_ = 0;
    nvars_ = 0;
    ef_begin_ = 0;
    lc_begin_ = 0;
    animal_begin_ = 0;
    manure_begin_ = 0;

    std::string msu_cbpo_path = misc_utilities::get_env_var("MSU_CBPO_PATH", "/opt/opt4cast");
    csvs_path = fmt::format("{}/csvs",msu_cbpo_path);
}

void Scenario::init(const std::string& filename, const std::string& filename_scenario, bool is_ef_enabled, bool is_lc_enabled, bool is_animal_enabled, bool is_manure_enabled, const std::string& manure_nutrients_file) {
    //std::string manure_nutrients_file_ = "manurenutrientsconfinement.parquet";
    this->is_ef_enabled = is_ef_enabled;
    this->is_lc_enabled = is_lc_enabled;
    this->is_animal_enabled = is_animal_enabled;
    this->is_manure_enabled = is_manure_enabled;
    nvars_ = 0;
    load(filename, filename_scenario);
    if (is_ef_enabled) {
        compute_efficiency_keys();
        ef_begin_ = nvars_;
        auto ef_size_tmp = compute_efficiency_size();
        nvars_ += ef_size_tmp;
        ef_end_ = nvars_;
    }
    if (is_lc_enabled) {
        compute_lc_keys();
        lc_begin_ = nvars_;
        auto lc_size_tmp = compute_lc_size();
        nvars_ += lc_size_tmp;
    }
    if (is_animal_enabled) {
        compute_animal_keys();
        animal_begin_ = nvars_;
        nvars_ += compute_animal_size();
    }

    if (is_manure_enabled) {
        //manure_counties_ = {"43"};//102: Nelson
        auto neighbors_file = "/opt/opt4cast/csvs/cast_neighbors.json";
        load_neighbors(neighbors_file);
        manure_dry_lbs_ = read_manure_nutrients(manure_nutrients_file); //call it after load_neighbors
                                                                        //
                                                                        
        //print manure_dry_lbs_
        fmt::print("Manure Dry Lbs:\n");
        for (const auto& [key, value] : manure_dry_lbs_) {
             double moisture = 0.7;
             double amount = value/ (1.0 - moisture); //convert to wet pounds 
             amount = amount / 2000.0; //convert to wet tons
             fmt::print("{}: {} wet tons\n", key, amount);
        }

        compute_manure_keys();
        manure_begin_ = nvars_;
        nvars_ += compute_manure_size();
    }
}

// EFICCIENCY BEGIN


int Scenario::compute_ef() {
    double total_cost = 0;

    std::vector<double> fx(2, 0.0);
    std::vector<double> g(2, 0.0);
    int nconstraints = 2;

    compute_eta();

    std::vector<double> pt_load(3, 0.0);
    for (const auto &key: ef_keys_) {
        auto bmp_groups =  efficiency_[key];
        std::vector<double> prod(3, 1);
        std::vector <std::string> out;
        misc_utilities::split_str(key, '_', out);
        auto s = out[0];
        auto lrseg = std::stoi(out[0]);
        auto u = out[2];
        auto [fips, state_id, county, geography] = lrseg_dict_[lrseg];
        auto alpha = amount_[key];
        auto bmp_group_idx = 0;
        for (const auto &bmp_group: bmp_groups) {
            std::vector<double> sigma(3, 0.0);
            auto bmp_idx = 0;
            auto sigma_cnstr = 0.0;

            for (const auto &bmp: bmp_group) {
                auto pct = ef_x_[key][bmp_group_idx][bmp_idx];
                sigma_cnstr += pct;
                double amount = pct * alpha;
                auto bmp_cost_idx = fmt::format("{}_{}", state_id, bmp);
                double cost = amount * bmp_cost_[bmp_cost_idx];

                std::string s_tmp = fmt::format("{}_{}_{}", bmp, s, u);
                auto eta = eta_dict_[s_tmp];
                sigma[0] += eta[0] * pct;
                sigma[1] += eta[1] * pct;
                sigma[2] += eta[2] * pct;
                sigma_cnstr += pct;
                ++bmp_idx;
            }
            prod[0] *= (1.0 - sigma[0]);
            prod[1] *= (1.0 - sigma[1]);
            prod[2] *= (1.0 - sigma[2]);
            //g[nconstraints] = (1.0 - sigma_cnstr);

            ++bmp_group_idx;
            ++nconstraints;
        }
        if(!phi_dict_.contains(key)) {
            std::cout<<"nooooooo.... no key here\n"<<key<<"\n";
        }
        auto phi = phi_dict_[key];

        pt_load[0] += phi[0] * alpha * prod[0];
        pt_load[1] += phi[1] * alpha * prod[1];
        pt_load[2] += phi[2] * alpha * prod[2];

    }

    //g[0] =  (sum_load_invalid[0] + pt_load[0]) - 0.8*(sum_load_invalid[0] + sum_load_valid[0]) ;
    double load_ub = sum_load_invalid_[load_to_opt_] + sum_load_valid_[load_to_opt_];
    double load_obtained = sum_load_invalid_[load_to_opt_] + pt_load[load_to_opt_];

    double load_lb = 0.8 * load_ub; //20% reduction
    //g[0] = (load_obtained < load_lb)?(load_obtained-load_lb)/load_ub:0;

    double cost_ub = 1000000.0;
    double cost_steps = 1000000.0;
    g[1] = (total_cost > cost_ub)? (cost_ub-total_cost)/cost_steps:0;

    fx[0] = total_cost;
    fx[1] = sum_load_invalid_[load_to_opt_] + pt_load[load_to_opt_];
    fmt::print("Total Cost: {}. Total Load: {}.\n", fx[0], fx[1]);

    //write_files_csv2 (selected_bmps, emo_uuid, exec_uuid);
    //write_files_animal_csv2 (selected_bmps, emo_uuid, exec_uuid);

    //int nrows = write_and_send_parquet_file2(selected_bmps, emo_uuid, exec_uuid);
    return 0;
}

/*// Esta funcion no esta actualizada que cebe de hacer uso del diccionary ETA
void Scenario::compute_eta() {
    auto redis = sw::redis::Redis(REDIS_URL);

    for (const auto &key: ef_keys_) {
        auto bmp_groups =  efficiency_[key];
        std::vector <std::string> out;
        misc_utilities::split_str(key, '_', out);
        auto lrseg = out[0];
        auto load_src = out[2];
        for (const auto &bmp_group: bmp_groups) {
            for (const auto &bmp: bmp_group) {
                std::string s_tmp = fmt::format("{}_{}_{}", bmp, lrseg, load_src);
                std::vector <std::string> eta_tmp;
                redis.lrange(s_tmp, 0, -1, std::back_inserter(eta_tmp));

                if (!eta_tmp.empty()) {
                    eta_dict_[s_tmp] = {stof(eta_tmp[0]), stof(eta_tmp[1]), stof(eta_tmp[2])};
                } else {
                    eta_dict_[s_tmp] = {0.0, 0.0, 0.0};
                    //std::cout << "ETA not found"<<key<<"\n";
                }
            }
        }
    }
}
*/

void Scenario::compute_eta() {
    auto redis = sw::redis::Redis(REDIS_URL);

    for (const auto &key: ef_keys_) {
        auto& bmp_groups =  efficiency_[key];
        std::vector <std::string> out;
        misc_utilities::split_str(key, '_', out);
        auto lrseg = out[0];
        auto load_src = out[2];
        for (const auto &bmp_group: bmp_groups) {
            for (const auto &bmp: bmp_group) {
                std::string s_tmp = fmt::format("{}_{}_{}", bmp, lrseg, load_src);

	        	std::vector<std::string> eta_tmp;
                std::string eta_str = "0.0_0.0_0.0";
                if (redis.hexists("ETA", s_tmp)) {
                    eta_str = *redis.hget("ETA", s_tmp);
                }
                else {
                    std::cout << "No ETA for " << s_tmp << std::endl;
                }

                boost::split(eta_tmp, eta_str, boost::is_any_of("_"));

                if(!eta_tmp.empty()) {
                   std::vector<double> content_eta({stof(eta_tmp[0]), stof(eta_tmp[1]), stof(eta_tmp[2])});
                   eta_dict_[s_tmp] = content_eta;
                }
            }
        }
    }
}

size_t Scenario::compute_efficiency_size() {
    int counter = 0;

    for (const auto &key: ef_keys_) {
        auto bmp_groups =  efficiency_[key];
        for (const auto &bmp_group : bmp_groups) {
            ++counter;
            for (const auto &bmp : bmp_group) {
                ++counter;
            }
        }
    }

    ef_size_ = counter;
    ef_begin_ = 0;
    ef_end_ = counter;
    return counter;
}

size_t Scenario::compute_lc_size() {
    size_t counter = 0;
    for (const auto &key: lc_keys_) {
        auto bmp_group = land_conversion_from_bmp_to[key];
        ++counter; //to take into account the dummy bmp
        for (const auto &bmp: bmp_group) {
            ++counter;
        }
    }
    lc_size_ = counter;
    return counter;
}

size_t Scenario::compute_animal_size() {
    size_t counter = 0;
    for (const auto &key: animal_keys_) {
        auto bmp_group = animal_complete_[key];
        ++counter; //to take into account the dummy bmp
        for (const auto &bmp: bmp_group) {
            ++counter;
        }
    }
    animal_size_ = counter;
    return counter;
}

size_t Scenario::compute_manure_size() {
    size_t counter = 0;
    for (const auto &key: manure_keys_) {
        auto neighbors = manure_all_[key];
        ++counter; //to take into account the dummy bmp
        for (const auto &neighbor_to: neighbors) {
            ++counter;
        }
    }
    manure_size_ = counter;
    return counter;
}

void Scenario::compute_efficiency_keys() {
    for (const auto& pair : efficiency_) {
        ef_keys_.push_back(pair.first);
    }
    // Sort the vector of keys
    std::sort(ef_keys_.begin(), ef_keys_.end());
}

void Scenario::compute_lc_keys() {
    for (const auto& [key, value]: land_conversion_from_bmp_to) {
        std::vector <std::string> key_split;
        misc_utilities::split_str(key, '_', key_split);
        auto load_src = key_split[2];
        if(pct_by_valid_load_.contains(std::stoi(load_src))){
            double pct = pct_by_valid_load_.at(std::stoi(load_src));
            if (pct > MIN_LC_THRESHOLD) {
                lc_keys_.push_back(key);
            }
        }
    }

    // Sort the vector of keys
    std::sort(lc_keys_.begin(), lc_keys_.end());
}

void Scenario::compute_animal_keys() {

    for (const auto& pair : animal_complete_) {
        animal_keys_.push_back(pair.first);
    }

    // Sort the vector of keys
    std::sort(animal_keys_.begin(), animal_keys_.end());
}

void Scenario::compute_manure_keys() {
    for (const auto& pair : manure_dry_lbs_) {
        manure_keys_.push_back(pair.first);
    }
    std::sort(manure_keys_.begin(), manure_keys_.end());
}

void Scenario::load_neighbors(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file: "<<filename<<".\n";

        exit(-1);
        return;
    }

    json json_obj = json::parse(file);
    auto tmp_neighbors = json_obj.get<std::unordered_map<std::string, std::vector<int>>>();
    for (const auto& [key, value] : tmp_neighbors) {
        std::vector<int> tmp;
        for (const auto& val : value) {
            tmp.push_back(val);
        }
        if (tmp.size() > 0) {
            neighbors_dict_[key] = tmp;
        }
    }
}

std::vector<double> compute_loads(std::vector<std::string>& parcel_keys, std::unordered_map<std::string, std::vector<double>>& phi_dict, std::unordered_map<std::string, double>& amount) {
    double n_eos_sum = 0.0;
    double p_eos_sum = 0.0;
    double s_eos_sum = 0.0;
    double n_eor_sum = 0.0;
    double p_eor_sum = 0.0;
    double s_eor_sum = 0.0;
    double n_eot_sum = 0.0;
    double p_eot_sum = 0.0;
    double s_eot_sum = 0.0;

    for (const auto &key: parcel_keys) {
        auto phi = phi_dict[key];
        n_eos_sum += phi[0] * amount[key];
        p_eos_sum += phi[1] * amount[key];
        s_eos_sum += phi[2] * amount[key];
        n_eor_sum += phi[3] * amount[key];
        p_eor_sum += phi[4] * amount[key];
        s_eor_sum += phi[5] * amount[key];
        n_eot_sum += phi[6] * amount[key];
        p_eot_sum += phi[7] * amount[key];
        s_eot_sum += phi[8] * amount[key];
    }
    std::vector<double> ret = {n_eos_sum, p_eos_sum, s_eos_sum,
                               n_eor_sum, p_eor_sum, s_eor_sum,
                               n_eot_sum, p_eot_sum, s_eot_sum};
    return ret;
}

void Scenario::load(const std::string& filename, const std::string& filename_scenario) {

    // Open the JSON file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open the base scenario file: " << filename<<std::endl;
        exit(-1);
        return;
    }

    // Parse the JSON file directly into a nlohmann::json object
    json json_obj = json::parse(file);
    if (!json_obj.contains("scenario_id")) {
        scenario_id_ = 3814;
    }
    else {
        scenario_id_ = json_obj["scenario_id"].get<size_t>();
    }
    std::vector<std::string> keys_to_check = {"amount", "bmp_cost", "animal_unit", "lrseg", "scenario_data_str", "u_u_group", "counties" };
    for (const auto& key : keys_to_check) {
        if (!json_obj.contains(key)) {
            std::cout << "The JSON object does not contain the key '" << key << "'\n";
            exit(-1);
        }
    }

    std::ifstream file_scenario(filename_scenario);
    if (!file_scenario.is_open()) {
        std::cerr << "Failed to open the scenario file: " << filename_scenario<<std::endl;
        exit(-1);
        return;
    }
    // Parse the JSON file_scenario directly into a nlohmann::json object
    json json_obj_scenario = json::parse(file_scenario);
    std::vector<std::string> keys_to_check_scenario = {"selected_bmps", "bmp_cost", "selected_reduction_target", "sel_pollutant", "target_pct", "manure_counties"};

    for (const auto& key : keys_to_check_scenario) {
        if (!json_obj_scenario.contains(key)) {
            std::cout << "The JSON object of the scenario file does not contain key '" << key << "'\n";
            exit(-1);
        }
    }

    std::vector<int> selected_bmps = json_obj_scenario["selected_bmps"].get<std::vector<int>>();
    //manure_counties_ = json_obj_scenario["manure_counties"].get<std::vector<std::string>>();
    //manure_counties_ = {"43"};//Lancaster: Nelson
    std::unordered_map<std::string, double> updated_bmp_cost = json_obj_scenario["bmp_cost"].get<std::unordered_map<std::string, double>>();


    // Access the JSON data
    amount_ = json_obj["amount"].get<std::unordered_map<std::string, double>>();

    phi_dict_ = json_obj["phi"].get<std::unordered_map<std::string, std::vector<double>>>();
    efficiency_ = json_obj["efficiency"].get<std::unordered_map<std::string, std::vector<std::vector<int>>>>();


    std::unordered_map<std::string, std::vector<double>> phi_dict = json_obj["phi"].get<std::unordered_map<std::string, std::vector<double>>>();

    std::unordered_map<std::string, std::vector<std::vector<int>>> filtered_efficiency;
    std::vector<std::string> filtered_valid_ef_keys;
    for (const auto&[key, val]: efficiency_) {
        std::vector<std::vector<int>> filtered_bmps;
        for (const auto& bmp_group: val) {
            std::vector<int> filtered_bmps_group;
            for (const auto& bmp: bmp_group) {
                if (std::find(selected_bmps.begin(), selected_bmps.end(), bmp) != selected_bmps.end()) {
                    filtered_bmps_group.push_back(bmp);
                }
            }
            if (!filtered_bmps_group.empty()) {
                filtered_bmps.push_back(filtered_bmps_group);
            }
        }
        if (!filtered_bmps.empty()) {
            filtered_efficiency[key] = filtered_bmps;
            filtered_valid_ef_keys.push_back(key);
        }
    }

    std::vector<std::string> filtered_invalid_ef_keys;
    for (const auto&[key, val]: efficiency_) {
        if (std::find(filtered_valid_ef_keys.begin(), filtered_valid_ef_keys.end(), key) == filtered_valid_ef_keys.end()) {
            filtered_invalid_ef_keys.push_back(key);
        }
    }


    efficiency_ = filtered_efficiency; 


    auto sum_load_valid = compute_loads(filtered_valid_ef_keys, phi_dict, amount_);
    auto sum_load_invalid = compute_loads(filtered_invalid_ef_keys, phi_dict, amount_);


    //print filtered_efficiency
    /*
    for (const auto&[key, val]: filtered_efficiency) {
        std::cout << key << " : ";
        for (const auto& bmp_group: val) {
            std::cout << "[";
            for (const auto& bmp: bmp_group) {
                std::cout << bmp << ", ";
            }
            std::cout << "], ";
        }
        std::cout << std::endl;
    }
    */


    bmp_cost_ = json_obj["bmp_cost"].get<std::unordered_map<std::string, double>>();

    // replace bmp_cost with updated_bmp_cost 
    for (const auto&[key, val]: updated_bmp_cost) {
        if (bmp_cost_.find(key) != bmp_cost_.end()) {
            bmp_cost_[key] = updated_bmp_cost[key];

        }
    }



    //valid_lc_bmps_ = json_obj["valid_lc_bmps"].get<std::vector<std::string>>();
    std::vector<std::string> accepted_lc_bmps;
    //8, 13, 22, 
    //accepted_lc_bmps = {"7", "8", "9", "11", "13", "17", "22", "61", "62", "66", "275", "276", "277", "278", "279", "280"};
    //accepted_lc_bmps = {"7", "8", "9", "11", "12", "13", "15", "17", "18", "22", "61", "62", "65", "66", "140", "200", "275", "276", "277", "278", "280"};
    accepted_lc_bmps = {"9", "12", "13", "15", "22", "200"};//, "140", "7"};
    for (const auto& my_bmp: accepted_lc_bmps) {
        auto int_my_bmp = std::stoi(my_bmp);
            if (std::find(selected_bmps.begin(), selected_bmps.end(), int_my_bmp) != selected_bmps.end()) {
                    valid_lc_bmps_.push_back(my_bmp);
            }
    }

    //valid_lc_bmps_ = {"7", "9", "13"};
    //accepted_lc_bmps_ = {"9", "11", "61", "62", "66", "275", "276", "277", "278", "279", "280"]

    //Landuse Change': [7, 9, 11, 61, 62, 66, 275, 276, 277, 278, 279, 280], 

    auto land_conversion_from_bmp_to_tmp = json_obj["land_conversion_to"].get<std::unordered_map<std::string, std::vector<std::string>>>();
    for (const auto& [key, value] : land_conversion_from_bmp_to_tmp) {
        std::vector<std::string> bmp_group;
        for (const auto& bmp_load_src : value) {
            std::vector<std::string> bmp_split;
            misc_utilities::split_str(bmp_load_src, '_', bmp_split);
            // if bmp_split[0] is in valid_lc_bmps, then add it to the bmp_group
            if(std::ranges::find(valid_lc_bmps_, bmp_split[0]) != valid_lc_bmps_.end()) {
                bmp_group.push_back(bmp_load_src);
            }
        }
        if (bmp_group.size() > 0) {
            land_conversion_from_bmp_to[key] = bmp_group;
        }

    }
    
    //lrseg_ = json_obj["lrseg"].get<std::unordered_map<std::string, std::vector<int>>>();
    animal_complete_ = json_obj["animal_complete"].get<std::unordered_map<std::string, std::vector<int>>>();
    animal_ = json_obj["animal_unit"].get<std::unordered_map<std::string, double>>();
    auto lrseg_tmp = json_obj["lrseg"].get<std::unordered_map<std::string, std::vector<int>>>();
    scenario_data_str_ = json_obj["scenario_data_str"].get<std::string>();
    //boost::replace_all(scenario_data_str_, "A", "38");
    //boost::replace_all(scenario_data_str_, "N", "7");
    //std::vector<int> counties ={381, 364, 402, 391, 362, 367, 365};
    //td::string counties_str = "381_364_402_391_362_367_365";
    //counties_str = counties_str;//"410";//_364_402_391_362_367_365";
    //scenario_data_str_ = fmt::format("{}_{}", scenario_data_str_, counties_str);
    std::cout<<"Scenario Data Str: "<<scenario_data_str_<<std::endl;
    //scenario_data_str_ = "empty_38_6611_256_6_4_59_1_6608_158_2_31_8_294";
    /*  

    std::string scenario_name = "empty";
    int atm_dep_data_set= row["AtmDepDataSetId"].get<int>();
    int back_out_scenario= row["BackoutScenarioId"].get<int>();
    int base_condition= row["BaseConditionId"].get<int>();
    int base_load= row["BaseLoadId"].get<int>();
    int cost_profile= row["CostProfileId"].get<int>();
    int climate_change_data_set= row["ClimateChangeDataSetId"].get<int>();
    int historical_crop_need_scenario= 6608;
    int point_source_data_set= row["PointSourceDataSetId"].get<int>();
    int scenario_type= row["ScenarioTypeId"].get<int>();
    int soil_p_data_set= row["SoilPDataSetId"].get<int>();
    int source_data_revision= row["SourceDataRevisionId"].get<int>();
    std::string number = "N";
     std::string scenario_data2 = fmt::format("{}_{}_{}_{}_{}_{}_{}_{}_{}_{}_{}_{}_{}",scenario_name,\
                                      atm_dep_data_set, back_out_scenario, base_condition,\
                                      base_load, cost_profile, climate_change_data_set,\
                                      number, historical_crop_need_scenario, point_source_data_set,\
                                      scenario_type, soil_p_data_set, source_data_revision);
    */

    //fmt::print("scenario_data_str_: {}\n", scenario_data_str_);
    for (const auto& [key, vec] : lrseg_tmp) {
        if (vec.size() == 4) {
            lrseg_dict_[std::stoi(key)] = std::make_tuple(vec[0], vec[1], vec[2], vec[3]);
        } else {
            std::cerr << "The vector size is not 4.\n";
        }
    }

    auto u_u_group_tmp = json_obj["u_u_group"].get<std::unordered_map<std::string, int>>();
    for (const auto& [key, value] : u_u_group_tmp) {
        u_u_group_dict[std::stoi(key)] = value;
    }

    auto counties_tmp = json_obj["counties2"].get<std::unordered_map<std::string, int>>();
    for (const auto& [key, value] : counties_tmp) {
        counties_[std::stoi(key)] = value;
    }


    auto geography_county_tmp = json_obj["counties"].get<std::unordered_map<std::string, std::tuple<int, int, std::string, std::string, std::string>>>();
    for (const auto& [key, val] : geography_county_tmp){
        geography_county_[std::stoi(key)] = val;
    }

    auto pct_by_valid_load_tmp = json_obj["pct_by_valid_load"].get<std::unordered_map<std::string,double>>();
    for (const auto& [key, val] : pct_by_valid_load_tmp){
        pct_by_valid_load_[std::stoi(key)] = val;
    }
}

void Scenario::initialize_vector(std::vector<double>& x) {
    if (x.size() != nvars_) {
        x.resize(nvars_);
    }

    if (is_lc_enabled == true) {
        size_t lc_idx = lc_begin_;
        for (const auto &key: lc_keys_) {
            auto bmp_group = land_conversion_from_bmp_to[key];
            x[lc_idx] = 1.0;
            ++lc_idx;
            /*
            bool flag = false;
            if (std::find(my_s_h_u_vec.begin(), my_s_h_u_vec.end(), key) != my_s_h_u_vec.end()) {
                flag = true;
            }
            */
            for (const auto &bmp: bmp_group) {
                /********if(flag) {
                    x[lc_idx] = 0.0;
                } else {*/
                    x[lc_idx] = misc_utilities::rand_double(0.0, 1.0); 
                    //x[lc_idx] = misc_utilities::rand_double(0.0, 1.0); 
                //}
                ++lc_idx;
            }
        }
    }

    if (is_animal_enabled == true) {
        size_t animal_idx = animal_begin_;
        for (const auto &key: animal_keys_) {
            auto bmp_group = animal_complete_[key];
            x[animal_idx] = 1.0;
            ++animal_idx;
            for (const auto &bmp: bmp_group) {
                x[animal_idx] = misc_utilities::rand_double(0.0, 1.0); 
                ++animal_idx;
            }
        }
    }
    if (is_manure_enabled == true) {
        size_t manure_idx = manure_begin_;
        for (const auto &key: manure_keys_) {
            auto neighbors = manure_all_[key];
            x[manure_idx] = 1.0;
            ++manure_idx;
            for (const auto &neighbor : neighbors) {
                x[manure_idx] = misc_utilities::rand_double(0.0, 1.0); 
                ++manure_idx;
            }
        }
    }

}
/*
 * 
 *
 * */
void Scenario::sum_alpha(std::unordered_map<std::string, double>& am,  const std::string& key, double acreage) {
    double tmp = 0.0;
    if (am.contains(key)) {
        tmp = am.at(key);
    }
    tmp += acreage;
    am[key] = tmp;
}

double Scenario::alpha_minus(const std::string& key, double original_amount, const std::unordered_map<std::string, double>& amount_minus ) {
    double minus = 0.0;
    double original = original_amount;
    /*
    if (amount_.contains(key)) {
        original = amount_.at(key);
        std::cout<<"Original "<<original<<std::endl;
    }
     */

    if (amount_minus.contains(key)) {
        minus = amount_minus.at(key);
        //std::cout<<"My Minus "<<minus<<std::endl;
    }

    return original - minus;
}

double Scenario::alpha_plus_minus(const std::string& key, double original_amount, 
        const std::unordered_map<std::string, double>& amount_minus,
        const std::unordered_map<std::string, double>& amount_plus) {

    double plus = 0.0;

    if (amount_plus.contains(key)) {
        plus = amount_plus.at(key);
    }

    return plus + Scenario::alpha_minus(key, original_amount, amount_minus);
}

void Scenario::load_alpha(std::vector<double>& my_alpha, 
        const std::unordered_map<std::string, double>& amount_minus,
        const std::unordered_map<std::string, double>& amount_plus
        ) {
    size_t parcel_idx = 0;
    my_alpha.clear();
    for (auto &[from_key, value]: amount_) {
        //auto new_alpha = scenario__alpha_minus(from_key, amount_[from_key]);
        double new_alpha = Scenario::alpha_plus_minus(from_key, amount_[from_key], amount_minus, amount_plus);
        if (amount_[from_key] != new_alpha) {
            std::cout << "New Alpha: " << new_alpha << " Previos Alpha: " << amount_[from_key]<<"\n"; 
        }

        my_alpha.push_back(new_alpha);
    }
}


double Scenario::normalize_efficiency(const std::vector<double>& x, 
    std::unordered_map<std::string, std::vector<std::vector<double>>> ef_x
        ) {
    int counter = 0;
    for (const auto &key: ef_keys_) {
        auto bmp_groups =  efficiency_[key];
        std::vector<std::vector<double>> grps_tmp;
        for (const auto& bmp_group : bmp_groups) {
            std::vector<double> grp_tmp;
            //The representation has a slack variable to represent the unused space
            double sum = x[counter];
            ++counter;

            for (const auto& bmp : bmp_group) {
                grp_tmp.push_back(x[counter]);
                sum += x[counter];
                ++counter;
            }

            for (auto &bmp: grp_tmp) {
                bmp = bmp / sum;
            }
            grps_tmp.push_back(grp_tmp);
        }
        ef_x_[key] = grps_tmp;
    }
    return 0.0;
}

double Scenario::compute_cost(const std::vector<std::tuple<int, int, int, int, double>>& parcel) {
    double total_cost = 0.0;
    for(const auto& entry : parcel) { 
        auto [lrseg, agency, load_src, bmp_idx, amount] = entry;
        auto [fips, state, county, geography] = lrseg_dict_[lrseg];
        auto key_bmp_cost = fmt::format("{}_{}", state, bmp_idx);
        double cost = amount * bmp_cost_[key_bmp_cost];
        total_cost += cost;
    }
    return total_cost;
}

double Scenario::compute_cost_animal(const std::vector<std::tuple<int, int, int, int, int, double>>& parcel) {
    double total_cost = 0.0;
    for(const auto& entry : parcel) { 
        auto [base_condition, county, load_src, animal_id, bmp, amount] = entry; 
        auto [geography, geography2_id, fips, county_name, state_abbr] = geography_county_[county];
        auto state = counties_[county];
        auto key_bmp_cost = fmt::format("{}_{}", state, bmp);
        double cost = bmp_cost_[key_bmp_cost];
        total_cost += cost;
    }
    return total_cost;
}

double Scenario::compute_cost_manure(const std::vector<std::tuple<int, int, int, int, int, double>>& parcel) {
    double total_cost = 0.0;
    for(const auto& entry : parcel) { 
        auto [county_from, county_to, load_src, animal_id, bmp, amount] = entry; 
        auto [geography_from, geography2_id_from, fips_from, county_name_from, state_abbr_from] = geography_county_[county_from];
        auto state = counties_[county_from];
        auto key_bmp_cost = fmt::format("{}_{}", state, bmp);
        double cost = bmp_cost_[key_bmp_cost];
        total_cost += cost;
    }
    return total_cost;
}
/*
 *
 * 
 * */
double Scenario::normalize_lc(const std::vector<double>& x, 
        std::vector<std::tuple<int, int, int, int, double>>& lc_x,
        std::unordered_map<std::string, double>& amount_minus,
        std::unordered_map<std::string, double>& amount_plus) {
    int counter = lc_begin_;
    amount_minus.clear();
    amount_plus.clear();
    lc_x.clear();
    double total_cost = 0.0;
    //std::vector<std::string> lc_altered_keys;
    for (const auto& key : lc_keys_) {
        std::vector<std::string> bmp_group =  land_conversion_from_bmp_to[key];
        std::vector<std::pair<double, std::string>> grp_tmp;
        std::vector <std::string> key_split;
        double sum = x[counter];
        misc_utilities::split_str(key, '_', key_split);
        auto [lrseg, agency, load_src] = std::make_tuple(std::stoi(key_split[0]), std::stoi(key_split[1]), std::stoi(key_split[2]));
        ++counter;

        for (std::string bmp : bmp_group) {
            grp_tmp.push_back({x[counter], bmp});
            sum += x[counter];
            ++counter;
        }

        double pct_accum = 0.0;
        std::vector<std::pair<double, std::string>> grp_pct_tmp;
        for (auto line : grp_tmp) {
            double pct = line.first;
            std::string to = line.second;
            double norm_pct =  (MAX_PCT_LC_BMP*pct) / sum;
            std::vector <std::string> out_to;
            misc_utilities::split_str(to, '_', out_to);
            auto bmp = std::stoi(out_to[0]);
            auto key_to = fmt::format("{}_{}_{}", key_split[0], key_split[1], out_to[1]);
            sum_alpha(amount_plus, key_to, norm_pct * amount_[key]);
            pct_accum += norm_pct;
            if (norm_pct * amount_[key] > 1.0) {
                double amount = (norm_pct * amount_[key]);
                auto [fips, state, county, geography] = lrseg_dict_[lrseg];
                auto key_bmp_cost = fmt::format("{}_{}", state, bmp);
                double cost = amount * bmp_cost_[key_bmp_cost];
                total_cost += cost;
                lc_x.push_back({std::stoi(key_split[0]), std::stoi(key_split[1]), std::stoi(key_split[2]), bmp, norm_pct * amount_[key]});
            }
        }
        sum_alpha(amount_minus, key, pct_accum * amount_[key]);
    }
    //std::cout<<"lc_x: "<<lc_x.size()<<std::endl;
    return total_cost;
}


double Scenario::normalize_animal(const std::vector<double>& x, std::vector<std::tuple<int, int, int, int, int, double>>& animal_x) {
    size_t counter = animal_begin_;
    double total_cost = 0.0;
    animal_x.clear();
    //std::vector<std::string> lc_altered_keys;
    for (const std::string& key : animal_keys_) {
        std::vector<int> bmp_group =  animal_complete_[key];

        std::vector<std::pair<double, int>> grp_tmp;
        std::vector <std::string> key_split;
        misc_utilities::split_str(key, '_', key_split);
        auto [base_condition, county, load_source, animal_id] = std::make_tuple(std::stoi(key_split[0]), std::stoi(key_split[1]), std::stoi(key_split[2]), std::stoi(key_split[3]));

        double sum = x[counter];
        ++counter; //to take into account the dummy bmp

        for (int bmp : bmp_group) {
            grp_tmp.push_back({x[counter], bmp});
            sum += x[counter];
            ++counter;
        }

        double pct_accum = 0.0;
        //fmt::format("grp_tmp: {}\n", grp_tmp.size());
        for (auto [pct, bmp]: grp_tmp) {
            //double norm_pct =  (double) pct / sum;

            double norm_pct =  (MAX_PCT_ANIMAL_BMP*pct) / sum;
            if (norm_pct * animal_[key] > 1.0) {
                double amount = (norm_pct * animal_[key]);
                auto state = counties_[county];
                std::string key_bmp_cost = fmt::format("{}_{}", state, bmp);
                double cost = amount * bmp_cost_[key_bmp_cost];
                total_cost += cost;
                animal_x.push_back({base_condition, county, load_source, animal_id, bmp, amount});
            }
        }
    }

    //std::cout<<"animal_x: "<<animal_x.size()<<std::endl;
    return total_cost;
}



double Scenario::normalize_manure(const std::vector<double>& x, std::vector<std::tuple<int, int, int, int, int, double>>& manure_x) {
    size_t counter = manure_begin_;
    double total_cost = 0.0;
    manure_x.clear();
    auto bmp = 31; //manure transport
    //std::vector<std::string> lc_altered_keys;
    
    for (const std::string& key : manure_keys_) {
        std::vector<int> neighbors =  manure_all_[key];

        std::vector<std::pair<double, int>> grp_tmp;
        std::vector <std::string> key_split;
        misc_utilities::split_str(key, '_', key_split);
        auto county = std::stoi(key_split[0]);
        auto load_src = std::stoi(key_split[1]);
        auto animal_id = std::stoi(key_split[2]);
        double sum = x[counter];
        ++counter; //to take into account the dummy bmp

        for (int neighbor_to: neighbors) {
            grp_tmp.push_back({x[counter], neighbor_to});
            sum += x[counter];
            ++counter;
        }

        double pct_accum = 0.0;
        for (auto [pct, neighbor_to]: grp_tmp) {

            double norm_pct =  (MAX_PCT_MANURE_BMP*pct) / sum;
            if (norm_pct * manure_dry_lbs_[key] > 1.0) {
                double amount = (norm_pct * manure_dry_lbs_[key]);
                //double moisture = 0.7;
                //amount = amount / (1.0 - moisture); //convert to wet pounds 
                amount = amount / 2000.0; //convert to wet tons
                                          //convert to dry tons
                auto state = counties_[county];
                std::string key_bmp_cost = fmt::format("{}_{}", state, bmp);
                double cost = amount * bmp_cost_[key_bmp_cost];
                total_cost += cost;
                manure_x.push_back({county, neighbor_to, load_src, animal_id, bmp, amount});
            }
        }
    }

    //std::cout<<"animal_x: "<<animal_x.size()<<std::endl;
    return total_cost;
}

std::vector<std::string> Scenario::send_files(const std::string& emo_uuid, const std::vector<std::string>& exec_uuid_vec) {
    std::string emo_str = scenario_data_str_;
    

    RabbitMQClient rabbit(emo_str, emo_uuid);

    for (const auto& exec_uuid : exec_uuid_vec) {
        rabbit.send_signal(exec_uuid);
    }

    auto output_rabbit = rabbit.wait_for_all_data();
    return output_rabbit;
}



size_t Scenario::write_land_json(
        const std::vector<std::tuple<int, int, int, int, double>>& lc_x,
        const std::string& out_filename
) {
    std::unordered_map<std::string, double> land_x_to_json;

    for (const auto& [lrseg, agency, load_src, bmp_idx, amount] : lc_x) {
        std::string key = fmt::format("{}_{}_{}_{}", lrseg, agency, load_src, bmp_idx);
        land_x_to_json[key] = amount;
    }
    json json_obj = land_x_to_json;
    std::ofstream file(out_filename);
    file<<json_obj.dump();
    file.close();
    return lc_x.size();
}

size_t Scenario::write_animal_json(const std::vector<std::tuple<int, int, int, int, int, double>>& animal_x, const std::string& out_filename) {
    std::unordered_map<std::string, double> animal_x_to_json;

    for(auto [base_condition, county, load_src, animal_id, bmp, amount] : animal_x) {
        std::string key = fmt::format("{}_{}_{}_{}_{}", base_condition, county, load_src, animal_id, bmp); 
        animal_x_to_json[key] = amount;
    }
    json json_obj = animal_x_to_json;
    std::ofstream file(out_filename);
    file<<json_obj.dump();
    file.close();
    return animal_x.size();
}


size_t Scenario::write_manure_json(const std::vector<std::tuple<int, int, int, int, int, double>>& manure_x, const std::string& out_filename) {
    std::unordered_map<std::string, double> manure_x_to_json;

    for(auto [county_from, county_to, load_src, animal_id, bmp, amount] : manure_x) {
        std::string key = fmt::format("{}_{}_{}_{}_{}", county_from, county_to, load_src, animal_id, bmp); 
        manure_x_to_json[key] = amount;
    }
    json json_obj = manure_x_to_json;
    std::ofstream file(out_filename);
    file<<json_obj.dump();
    file.close();
    return manure_x.size();
}



int Scenario::write_land(
        const std::vector<std::tuple<int, int, int, int, double>>& lc_x,
        const std::string& out_filename
) {
    if (lc_x.size() == 0) {
        return 0;
    }

    std::shared_ptr<arrow::Schema> schema = arrow::schema ({
                                                                   arrow::field("BmpSubmittedId", arrow::int32()),
                                                                   arrow::field("AgencyId", arrow::int32()),
                                                                   arrow::field("StateUniqueIdentifier", arrow::utf8()), //it can be binary
                                                                   arrow::field("StateId", arrow::int32()),
                                                                   arrow::field("BmpId", arrow::int32()),
                                                                   arrow::field("GeographyId", arrow::int32()),
                                                                   arrow::field("LoadSourceGroupId", arrow::int32()),
                                                                   arrow::field("UnitId", arrow::int32()),
                                                                   arrow::field("Amount", arrow::float64()),
                                                                   arrow::field("IsValid", arrow::boolean()),
                                                                   arrow::field("ErrorMessage", arrow::utf8()),//it can be binary
                                                                   arrow::field("RowIndex", arrow::int32()),
                                                           });
    std::unordered_map<int, double> bmp_sum;
    arrow::Int32Builder bmp_submitted_id, agency_id;
    arrow::StringBuilder state_unique_identifier;
    arrow::Int32Builder state_id, bmp_id, geography_id, load_source_id, unit_id_builder;
    arrow::DoubleBuilder amount_builder;
    arrow::BooleanBuilder is_valid;
    arrow::StringBuilder error_message;
    arrow::Int32Builder row_index;


    std::shared_ptr<arrow::io::FileOutputStream> outfile;


    PARQUET_ASSIGN_OR_THROW(
            outfile,
            arrow::io::FileOutputStream::Open(out_filename));

    parquet::WriterProperties::Builder builder;
    //builder.compression(parquet::Compression::ZSTD);
    builder.version(parquet::ParquetVersion::PARQUET_1_0);

    std::shared_ptr<parquet::schema::GroupNode> my_schema;

    parquet::schema::NodeVector fields;


    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "BmpSubmittedId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "AgencyId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "StateUniqueIdentifier", parquet::Repetition::REQUIRED, parquet::Type::BYTE_ARRAY, parquet::ConvertedType::UTF8
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "StateId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "BmpId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "GeographyId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "LoadSourceGroupId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "UnitId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "Amount", parquet::Repetition::REQUIRED, parquet::Type::DOUBLE
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "IsValid", parquet::Repetition::REQUIRED, parquet::Type::BOOLEAN
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "ErrorMessage", parquet::Repetition::REQUIRED, parquet::Type::BYTE_ARRAY, parquet::ConvertedType::UTF8
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "RowIndex", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));

    my_schema = std::static_pointer_cast<parquet::schema::GroupNode>(
            parquet::schema::GroupNode::Make("schema", parquet::Repetition::REQUIRED, fields));

    parquet::StreamWriter os{
            parquet::ParquetFileWriter::Open(outfile, my_schema, builder.build())};


    int idx = 0;
    int counter = 0;
    for (const auto& entry : lc_x) {
        auto [lrseg, agency, load_src, bmp_idx, amount] = entry;
        auto [fips, state, county, geography] = lrseg_dict_[lrseg];
        int load_src_grp = u_u_group_dict[load_src];
        int unit = 1; //acres
        os<<counter+1<<agency<<fmt::format("SU{}",counter)<<state<<bmp_idx<<geography<<load_src_grp<<unit<<amount<<true<<""<<counter+1<<parquet::EndRow;
        counter++;
    }

    return counter;
}

int Scenario::write_animal ( const std::vector<std::tuple<int, int, int, int, int, double>>& animal_x,
        const std::string& out_filename) {
    if (animal_x.size() == 0) {
        return 0;
    }


    parquet::schema::NodeVector fields;

    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "BmpSubmittedId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "BmpId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "AgencyId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "StateUniqueIdentifier", parquet::Repetition::REQUIRED, parquet::Type::BYTE_ARRAY, parquet::ConvertedType::UTF8
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "StateId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "GeographyId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "AnimalGroupId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "LoadSourceGroupId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "UnitId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "Amount", parquet::Repetition::REQUIRED, parquet::Type::DOUBLE
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "NReductionFraction", parquet::Repetition::REQUIRED, parquet::Type::DOUBLE
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "PReductionFraction", parquet::Repetition::REQUIRED, parquet::Type::DOUBLE
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "IsValid", parquet::Repetition::REQUIRED, parquet::Type::BOOLEAN
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "ErrorMessage", parquet::Repetition::REQUIRED, parquet::Type::BYTE_ARRAY, parquet::ConvertedType::UTF8
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "RowIndex", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    std::shared_ptr<arrow::io::FileOutputStream> outfile;

    PARQUET_ASSIGN_OR_THROW(
            outfile,
            arrow::io::FileOutputStream::Open(out_filename));

    std::shared_ptr<parquet::schema::GroupNode> my_schema = std::static_pointer_cast<parquet::schema::GroupNode>(
            parquet::schema::GroupNode::Make("schema", parquet::Repetition::REQUIRED, fields));

    parquet::WriterProperties::Builder builder;
    //builder.compression(parquet::Compression::ZSTD);
    builder.version(parquet::ParquetVersion::PARQUET_1_0);
    parquet::StreamWriter os{
            parquet::ParquetFileWriter::Open(outfile, my_schema, builder.build())};


    int idx = 0;
    int counter = 0;
    double total_cost = 0.0;
    for(auto [base_condition, county, load_src, animal_id, bmp, amount] : animal_x) {
        auto [geography, geography2_id, fips, county_name, state_abbr] = geography_county_[county];
        auto state = counties_[county];
        //int geography_id = 121; //121: Jefferson
        int unit_id = 13;

        auto key_bmp_cost = fmt::format("{}_{}", state, bmp);
        double cost = bmp_cost_[key_bmp_cost];
        total_cost += cost;
        int agency = 9;
        double nreduction=0.0, preduction=0.0;
        os<<counter+1<<bmp<<agency<<fmt::format("SU{}",counter)<<state<<geography2_id<<animal_id<<u_u_group_dict[load_src]<< unit_id<<amount<<nreduction<<preduction<<true<<""<<counter+1<<parquet::EndRow;
        counter++;
    }

    return counter;
}

int Scenario::write_manure( const std::vector<std::tuple<int, int, int, int, int, double>>& manure_x,
        const std::string& out_filename) {
    if (manure_x.size() == 0) {
        return 0;
    }


    parquet::schema::NodeVector fields;


    //BmpSubmittedId	BmpId	AgencyId	StateUniqueIdentifier	StateId	HasStateReference	CountyIdFrom	CountyIdTo	FipsFrom	FipsTo	AnimalGroupId	LoadSourceGroupId	UnitId	Amount	IsValid	ErrorMessage	RowIndex
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "BmpSubmittedId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "BmpId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "AgencyId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "StateUniqueIdentifier", parquet::Repetition::REQUIRED, parquet::Type::BYTE_ARRAY, parquet::ConvertedType::UTF8
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "StateId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));

    ///////////////////////////////////////
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "HasStateReference", parquet::Repetition::REQUIRED, parquet::Type::BOOLEAN
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "CountyIdFrom", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "CountyIdTo", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "FipsFrom", parquet::Repetition::REQUIRED, parquet::Type::BYTE_ARRAY, parquet::ConvertedType::UTF8
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "FipsTo", parquet::Repetition::REQUIRED, parquet::Type::BYTE_ARRAY, parquet::ConvertedType::UTF8
    ));
    ///////////////////////////////////////

    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "AnimalGroupId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "LoadSourceGroupId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "UnitId", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "Amount", parquet::Repetition::REQUIRED, parquet::Type::DOUBLE
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "IsValid", parquet::Repetition::REQUIRED, parquet::Type::BOOLEAN
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "ErrorMessage", parquet::Repetition::REQUIRED, parquet::Type::BYTE_ARRAY, parquet::ConvertedType::UTF8
    ));
    fields.push_back(parquet::schema::PrimitiveNode::Make(
            "RowIndex", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32
    ));
    std::shared_ptr<arrow::io::FileOutputStream> outfile;

    PARQUET_ASSIGN_OR_THROW(
            outfile,
            arrow::io::FileOutputStream::Open(out_filename));

    std::shared_ptr<parquet::schema::GroupNode> my_schema = std::static_pointer_cast<parquet::schema::GroupNode>(
            parquet::schema::GroupNode::Make("schema", parquet::Repetition::REQUIRED, fields));

    parquet::WriterProperties::Builder builder;
    //builder.compression(parquet::Compression::ZSTD);
    //builder.compression(parquet::Compression::SNAPPY);
    builder.version(parquet::ParquetVersion::PARQUET_1_0);
    parquet::StreamWriter os{
            parquet::ParquetFileWriter::Open(outfile, my_schema, builder.build())};


    int idx = 0;
    int counter = 0;
    double total_cost = 0.0;
    for(auto [county_from, county_to, load_src, animal_id, bmp, amount] : manure_x) {
        auto [geography_from, geography2_id_from, fips_from, county_name_from, state_abbr_from] = geography_county_[county_from];
        auto [geography_to, geography2_id_to, fips_to, county_name_to, state_abbr_to] = geography_county_[county_to];
        auto state_from = counties_[county_from];
        auto state_to = counties_[county_to];
        //int geography_id = 121; //121: Jefferson
        int unit_id = 12; //wet tons

        auto key_bmp_cost= fmt::format("{}_{}", state_from, bmp);
        double cost = bmp_cost_[key_bmp_cost];
        total_cost += cost;
        int agency = 9; //????
        os<<counter+1<<bmp<<agency<<fmt::format("SU{}",counter)<<state_from<<true<<county_from<<county_to<<fips_from<<fips_to<<animal_id<<u_u_group_dict[load_src]<< unit_id<<amount<<true<<""<<counter+1<<parquet::EndRow;

        counter++;
    }

    return counter;
}



std::unordered_map<std::string, double> Scenario::read_manure_nutrients(const std::string& filename) {
    std::unordered_map<std::string, double> manure_dry_lbs;
    std::shared_ptr<arrow::io::ReadableFile> infile;
    PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(filename, arrow::default_memory_pool()));
    
    std::unique_ptr<parquet::arrow::FileReader> reader;
    PARQUET_THROW_NOT_OK(parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));

    std::shared_ptr<arrow::Table> table;
    PARQUET_THROW_NOT_OK(reader->ReadTable(&table));

    // Find the column indices by name
    int lrsegIdIdx = table->schema()->GetFieldIndex("LrsegId");
    int loadSourceIdIdx = table->schema()->GetFieldIndex("LoadSourceId");
    int animalIdIdx = table->schema()->GetFieldIndex("AnimalId");
    int nutrientIdIdx = table->schema()->GetFieldIndex("NutrientId");
    int storedManureDryLbsIdx = table->schema()->GetFieldIndex("StoredManureDryLbs");

    for (int i = 0; i < table->num_rows(); i++) {
        int nutrient_id = std::static_pointer_cast<arrow::Int32Array>(table->column(nutrientIdIdx)->chunk(0))->Value(i);
        if (nutrient_id == 1) {
            int lrseg_id = std::static_pointer_cast<arrow::Int32Array>(table->column(lrsegIdIdx)->chunk(0))->Value(i);
            int load_source_id = std::static_pointer_cast<arrow::Int32Array>(table->column(loadSourceIdIdx)->chunk(0))->Value(i);
            int animal_id = std::static_pointer_cast<arrow::Int32Array>(table->column(animalIdIdx)->chunk(0))->Value(i);
            double stored_manure_dry_lbs = std::static_pointer_cast<arrow::DoubleArray>(table->column(storedManureDryLbsIdx)->chunk(0))->Value(i);

            auto [fips, state, county, geography] = lrseg_dict_[lrseg_id];
            auto county_str = std::to_string(county);
            auto result = std::ranges::find(manure_counties_, county_str);
            if(result != manure_counties_.end() && stored_manure_dry_lbs > 0.0) {
                auto key = fmt::format("{}_{}_{}", county, load_source_id, animal_id);
                manure_dry_lbs[key] += stored_manure_dry_lbs;
                auto neighbors = neighbors_dict_[county_str]; 
                std::sort(neighbors.begin(), neighbors.end());
                manure_all_[key] = neighbors; 
            }
        }
    }

    return manure_dry_lbs;
}

