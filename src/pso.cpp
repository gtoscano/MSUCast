#include <stdio.h>
#include <iostream>


//#include "pso.h"

#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <functional>
#include <numeric>
#include <cmath>
#include <functional> // For std::reference_wrapper
#include <fstream>

#include <range/v3/all.hpp>

#include "external_archive.h"
#include "particle.h"
#include "pso.h"
#include "scenario.h"
#include "misc_utilities.h"

#include <crossguid/guid.hpp>
#include <fmt/core.h>
#include <regex>
#include <filesystem>
#include <boost/algorithm/string.hpp>

#include <nlohmann/json.hpp>

//#include "spdlog/spdlog.h"
//#include "spdlog/sinks/stdout_color_sinks.h"

namespace fs = std::filesystem;

using json = nlohmann::json;


namespace {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::string replace_ending(const std::string& str, const std::string& oldEnding, const std::string& newEnding) {
    if (str.ends_with(oldEnding)) {
        return str.substr(0, str.size() - oldEnding.size()) + newEnding;
    }
    return str;
}
    
    void save(const std::vector<std::vector<double>>& data, const std::string& filename) {
        std::ofstream outFile(filename);
        
        // Check if the file opened successfully
        if (!outFile) {
            std::cerr << "Failed to open the file: "<<filename<<".\n";
            return;
        }
        
        for (const auto& row : data) {
            for (const auto& val : row) {
                outFile << val << ' ';
            }
            outFile << '\n';
        }
        
        outFile.close();
    }

    void save(const std::vector<Particle>& data, const std::string& filename, const std::string& filename_fx) {
        std::ofstream outFile(filename);
        if (!outFile) {
            std::cerr << "Failed to open the file: "<<filename<<".\n";
            return;
        }
        
        for (const auto& row : data) {
            for (const auto& val : row.get_x()) {
                outFile << val << ' ';
            }
            outFile << '\n';
        }
        
        outFile.close();

        std::ofstream out_file_fx(filename_fx);
        if (!out_file_fx) {
            std::cerr << "Failed to open the file: "<<filename_fx<<".\n";
            return;
        }
        
        for (const auto& row : data) {
            for (const auto& val : row.get_fx()) {
                out_file_fx << val << ' ';
            }
            out_file_fx << '\n';
        }
        
        out_file_fx.close();
    }


    std::vector<std::vector<std::tuple<int, int, int, int, double>>> read_scenarios_keyed_json2(std::string filename, const std::unordered_map<std::string, double>& alpha_dict) {
        std::vector<std::vector<std::tuple<int, int, int, int, double>>> scenarios_list;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open the file: "<<filename<<".\n";
            exit(-1);
        }
    
        json json_obj = json::parse(file);
        for (const auto &scenario_list : json_obj){
            std::vector<std::tuple<int, int, int, int, double>> parcel_list;
            for(const auto& parcel : scenario_list){
                std::vector<std::string> result_vec;
                auto key = parcel["name"].get<std::string>();
                boost::split(result_vec, key, boost::is_any_of("_"));
                auto lrseg = result_vec[0];
                auto agency = result_vec[1];
                auto load_src = result_vec[2];
                auto bmp = result_vec[3];
                double alpha = 0.0;
                try {
                    alpha = 1.0;//alpha_dict.at(fmt::format("{}_{}_{}", lrseg, agency, load_src));
                }
                catch (const std::exception& e) {
                    std::cerr << "Key not found: " << fmt::format("{}_{}_{}", lrseg, agency, load_src) << '\n';
                    for (const auto& [key, val] : alpha_dict) {
                        std::cerr << key << '\n';
                    }
                    std::cerr << e.what() << '\n';
                    exit(-1);
                }

                auto amount = parcel["amount"].get<double>() * alpha;

                parcel_list.emplace_back(std::stoi(lrseg), std::stoi(agency), std::stoi(load_src), std::stoi(bmp), amount);
            }
            scenarios_list.emplace_back(parcel_list);
        }
        return scenarios_list;
    }
    std::vector<std::vector<std::tuple<int, int, int, int, double>>> read_scenarios_keyed_json(std::string filename, const std::unordered_map<std::string, double>& alpha_dict) {
        std::vector<std::vector<std::tuple<int, int, int, int, double>>> scenarios_list;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open the file: "<<filename<<".\n";
            exit(-1);
        }
    
        json json_obj = json::parse(file);
        for (const auto &scenario_list : json_obj){
            std::vector<std::tuple<int, int, int, int, double>> parcel_list;
            for(const auto& parcel : scenario_list){
                std::vector<std::string> result_vec;
                auto key = parcel["name"].get<std::string>();
                boost::split(result_vec, key, boost::is_any_of("_"));
                auto lrseg = result_vec[0];
                auto agency = result_vec[1];
                auto load_src = result_vec[2];
                auto bmp = result_vec[3];
                double alpha = 0.0;
                try {
                    alpha = alpha_dict.at(fmt::format("{}_{}_{}", lrseg, agency, load_src));
                }
                catch (const std::exception& e) {
                    std::cerr << "Key not found: " << fmt::format("{}_{}_{}", lrseg, agency, load_src) << '\n';
                    for (const auto& [key, val] : alpha_dict) {
                        std::cerr << key << '\n';
                    }
                    std::cerr << e.what() << '\n';
                    exit(-1);
                }

                auto amount = parcel["amount"].get<double>() * alpha;

                parcel_list.emplace_back(std::stoi(lrseg), std::stoi(agency), std::stoi(load_src), std::stoi(bmp), amount);
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

}


PSO::PSO(int nparts, int nobjs, int max_iter, double w, double c1, double c2, double lb, double ub, const std::string& input_filename, const std::string& scenario_filename, const std::string& out_dir, bool is_ef_enabled, bool is_lc_enabled, bool is_animal_enabled, bool is_manure_enabled ) {
    out_dir_= out_dir;
    is_ef_enabled_ = is_ef_enabled;
    is_lc_enabled_ = is_lc_enabled;
    is_animal_enabled_ = is_animal_enabled;
    is_manure_enabled_ = is_manure_enabled;
    ef_size_ = 0;
    lc_size_ = 0;
    animal_size_ = 0;
    manure_size_ = 0;
    init_cast(input_filename, scenario_filename);
    input_filename_ = input_filename;
    scenario_filename_ = scenario_filename;
    this->nparts = nparts;
    this->nobjs= nobjs;
    this->max_iter = max_iter;
    this->w = w;
    this->c1 = c1;
    this->c2 = c2;
    this->lower_bound = lb; 
    this->upper_bound = ub; 
    //logger_ = spdlog::stdout_color_mt("PSO");
}
PSO::PSO(const PSO &p) {
    this->dim = p.dim;
    this->nparts = p.nparts;
    this->nobjs= p.nobjs;
    this->max_iter = p.max_iter;
    this->w = p.w;
    this->c1 = p.c1;
    this->c2 = p.c2;
    this->particles = p.particles;
    this->gbest_x = p.gbest_x;
    this->gbest_fx = p.gbest_fx;
    this->lower_bound = p.lower_bound;
    this->upper_bound = p.upper_bound;

    this->is_ef_enabled_ = p.is_ef_enabled_;
    this->is_lc_enabled_ = p.is_lc_enabled_;
    this->is_animal_enabled_ = p.is_animal_enabled_;
    this->is_manure_enabled_ = p.is_manure_enabled_;
    this->input_filename_ = p.input_filename_;
    this->out_dir_ = p.out_dir_;
    this->emo_uuid_ = p.emo_uuid_;
    this->lc_size_ = p.lc_size_;
    this->animal_size_ = p.animal_size_;
    this->manure_size_ = p.manure_size_;
    this->exec_uuid_log_ = p.exec_uuid_log_;
    this->scenario_ = p.scenario_;
    this->execute = p.execute;
    this->gbest_ = p.gbest_;
    //this->logger_ = p.logger_;
}

PSO& PSO::operator=(const PSO &p) {

  // Protect against self-assignment
    if (this == &p) {
        return *this;
    }

    this->dim = p.dim;
    this->nparts = p.nparts;
    this->nobjs= p.nobjs;
    this->max_iter = p.max_iter;
    this->w = p.w;
    this->c1 = p.c1;
    this->c2 = p.c2;
    this->particles = p.particles;
    this->gbest_x = p.gbest_x;
    this->gbest_fx = p.gbest_fx;
    this->lower_bound = p.lower_bound;
    this->upper_bound = p.upper_bound;


    return *this;
}

PSO::~PSO() {
    delete_tmp_files();


}
void PSO::delete_tmp_files(){
    int counter = 0;
    std::string directory = fmt::format("/opt/opt4cast/output/nsga3/{}", emo_uuid_);
    for (const auto& exec_uuid_vec : exec_uuid_log_) {
        for (const auto& exec_uuid: exec_uuid_vec) {
            auto list_files = misc_utilities::find_files(directory, exec_uuid);
            for (const auto &file : list_files) {
                auto full_path = fmt::format("{}/{}", directory, file);
                try {
                    if (fs::exists(full_path)) {
                        fs::remove(full_path);
                        //std::cout << "\t\tDeleted: " << full_path << std::endl;
                    } else {
                        //logger_->error("File not found: {}", full_path);
                        std::cout << "\t\tFile not found: " << full_path << std::endl;
                    }
                } catch (const std::exception &e) {
                    std::cerr << "\tError deleting full_path " << full_path << ": " << e.what() << std::endl;
                }
            }
        }
    }
}


void PSO::init_cast(const std::string& input_filename, const std::string& scenario_filename) {
    emo_uuid_ = xg::newGuid().str();
    fmt::print("emo_uuid: {}\n", emo_uuid_);
    std::string emo_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", emo_uuid_);
    std::unordered_map<std::string, std::tuple<int, double, double , double, double>> generation_fx;//key: UID, tuple: [idx, Nitrogen, Phosphorus, Sediments]
    std::unordered_map<std::string, int> generation_uuid_idx;
    misc_utilities::mkdir(emo_path);

    scenario_.init(input_filename, scenario_filename, is_ef_enabled_, is_lc_enabled_, is_animal_enabled_, is_manure_enabled_);
    lc_size_ = scenario_.get_lc_size();
    fmt::print("lc_size: {}\n", lc_size_);
    animal_size_ = scenario_.get_animal_size();
    fmt::print("animal_size: {}\n", animal_size_);
    manure_size_ = scenario_.get_manure_size();
    fmt::print("manure_size: {}\n", manure_size_);
    dim = lc_size_ + animal_size_ + manure_size_;
    fmt::print("dim: {}\n", dim);
    nobjs = 2;
}

void PSO::init() {
    particles.reserve(nparts);
    for (int i = 0; i < nparts; i++) {
        particles.emplace_back(dim, nobjs, w, c1, c2, lower_bound, upper_bound);

        //std::vector<double > x(lc_size + animal_size);
        auto x = particles[i].get_x();

        scenario_.initialize_vector(x);
        //particles[i].init();
        particles[i].init(x);
    }
    evaluate();
    for (int i = 0; i < nparts; i++) {
        particles[i].init_pbest();
    }
    update_gbest();
}

void PSO::optimize() {
    init();

    for (int i = 0; i < max_iter; i++) {
        fmt::print(" =================================================================\n                      iteration: {}\n=================================================================\n", i);
        for (int j = 0; j < nparts; j++) {
            std::uniform_int_distribution<> dis(0, gbest_.size() - 1);
            int index = dis(gen);
            const auto& curr_gbest = gbest_[index].get_x();
            particles[j].update(curr_gbest);
        }
        evaluate();
        update_pbest();
        update_gbest();
    }

    //exec_ipopt();

    exec_ipopt_all_sols();
    //evaluate_ipopt_sols();
}


void PSO::update_pbest() {
    for (int j = 0; j < nparts; j++) {
        particles[j].update_pbest();
    }

}

std::vector<std::string> PSO::generate_n_uuids(int n) {
    std::vector<std::string> uuids;
    for (int i = 0; i < n; i++) {
        uuids.push_back(xg::newGuid().str());
    }
    //json uuids_json;
    //uuids_json["uuids"] = uuids;
    return uuids;
}

void PSO::copy_parquet_files_for_ipopt(const std::string& path, const std::string& parent_uuid, const std::vector<std::string>& uuids) {
    for (const auto& uuid : uuids) {
        /*
        auto land_src = fmt::format("{}/{}_impbmpsubmittedland.parquet", path, parent_uuid);
        if(fs::exists(land_src)){
            auto land_dst = fmt::format("{}/{}_impbmpsubmittedland.parquet", path, uuid);
            misc_utilities::copy_file(land_src, land_dst);
        }
        */

        auto animal_src = fmt::format("{}/{}_impbmpsubmittedanimal.parquet", path, parent_uuid);
        if(fs::exists(animal_src)){
            auto animal_dst = fmt::format("{}/{}_impbmpsubmittedanimal.parquet", path, uuid);
            misc_utilities::copy_file(animal_src, animal_dst);
        }
        auto manure_src = fmt::format("{}/{}_impbmpsubmittedmanuretransport.parquet", path, parent_uuid);
        if (fs::exists(manure_src)) {
            auto manure_dst = fmt::format("{}/{}_impbmpsubmittedmanuretransport.parquet", path, uuid);
            misc_utilities::copy_file(manure_src, manure_dst);
        }
    }
} 

void PSO::exec_ipopt_all_sols(){
    Execute execute;
    int min_idx = 0;
    int max_idx = 0; 
    int mid_idx;
    int ipopt_popsize = 6;
    std::vector<double> values;

    for (const auto& particle : gbest_) {
        values.emplace_back(particle.get_fx()[0]);
    }

    // Step 1: Initialize a vector with indices 0 to values.size() - 1
    std::vector<size_t> indices(values.size());
    std::iota(indices.begin(), indices.end(), 0);

    // Step 2: Sort the indices based on comparing values from the values vector
    std::sort(indices.begin(), indices.end(),
        [&values](size_t i1, size_t i2) { return values[i1] < values[i2]; });
    //for (const auto& particle : gbest_) {
    min_idx = indices[0];
    max_idx = indices[indices.size() - 1];
    mid_idx = indices[indices.size() / 2];
    std::vector<int> idx_vec = {min_idx, mid_idx, max_idx};
    std::string path = fmt::format("/opt/opt4cast/output/nsga3/{}", emo_uuid_);
    json scenario_json = misc_utilities::read_json_file(fmt::format("{}/scenario.json", path));

   for (const auto& idx : idx_vec) { 
        auto lc_cost = gbest_[idx].get_lc_cost();
        auto animal_cost = gbest_[idx].get_animal_cost();
        auto manure_cost = gbest_[idx].get_manure_cost();
        auto parent_uuid = gbest_[idx].get_uuid();
        //execute.set_files(emo_uuid_, in_file);
        //execute.execute(emo_uuid_, 0.50, 6, 20);

        fmt::print("Particle Selected Cost: {}\n", gbest_[idx].get_fx()[0]);
        execute.update_output(emo_uuid_, gbest_[idx].get_fx()[0]);
        fmt::print("======================== best_lc_cost_: {}\n", lc_cost);
        fmt::print("======================== best_animal_cost_: {}\n", animal_cost);
        fmt::print("======================== best_manure_cost_: {}\n", manure_cost);
        std::string postfix;
        if (idx == min_idx) postfix = "min";
        else if (idx == max_idx) postfix = "max";
        else postfix = "median";
        std::string out_path = fmt::format("{}/ipopt_results-all-sols-{}", path, postfix);

        auto uuids = generate_n_uuids(ipopt_popsize);
        int sinfo = 0;
        std::string report_loads_path = fmt::format("{}/{}_reportloads.csv", path, parent_uuid);
        std::string output_path_prefix = fmt::format("{}/{}", path, parent_uuid);

        execute.get_json_scenario( sinfo, report_loads_path, output_path_prefix);

        copy_parquet_files_for_ipopt(path, parent_uuid, uuids);

        json base_scenario_json = misc_utilities::read_json_file(fmt::format("{}/reportloads_processed.json", path));
        json uuids_json;
        uuids_json["uuids"] = uuids;
        int pollutant_idx = 0;
        double ipopt_reduction = 0.50;
        int ipopt_popsize = 10;
        //evaluate_ipopt_sols(sub_dir, ipopt_uuid, animal_cost, manure_cost);
        //EpsConstraint eps_constr(base_scenario_json, scenario_json, uuids_json, path_out, pollutant_idx, evaluate_cast);
    }
}


void PSO::evaluate_ipopt_sols(const std::string& sub_dir, const std::string& ipopt_uuid, double animal_cost, double manure_cost) {
    
    std::vector<std::string> exec_uuid_vec;
    std::unordered_map<std::string, double> total_cost_map;
    //get_parent_solution

    std::string emo_path = fmt::format("/opt/opt4cast/output/nsga3/{}", emo_uuid_);
    auto ipopt_in_filename = fmt::format("{}/{}_impbmpsubmittedland.json", emo_path, ipopt_uuid);
    fmt::print("ipopt_in_filename: {}\n", ipopt_in_filename);
    std::vector<std::tuple<int, int, int, int, double>> parent_list;
    if(is_lc_enabled_){
        parent_list = read_scenario_json(ipopt_in_filename);
    }
    //get_ipopt_solutions
    std::string filename_ipopt_out = fmt::format("{}/config/ipopt.json", emo_path);
    std::string filename_ipopt_out2 = fmt::format("{}/config/ipopt2.json", emo_path);
    fmt::print("filename_ipopt_out: {}\n", filename_ipopt_out);
    auto alpha_dict = scenario_.get_alpha();
    auto ipopt_lists = read_scenarios_keyed_json2(filename_ipopt_out2, alpha_dict);

    for (const auto& parcel_list : ipopt_lists) {

        std::vector<std::tuple<int, int, int, int, double>> combined;
        if(is_lc_enabled_){
            combined = parent_list;
        }
        combined.insert(combined.end(), parcel_list.begin(), parcel_list.end());

        std::string exec_uuid = xg::newGuid().str();
        exec_uuid_vec.emplace_back(exec_uuid);
        auto land_filename = fmt::format("{}/{}_impbmpsubmittedland.parquet", emo_path, exec_uuid);
        scenario_.write_land(combined, land_filename);
        scenario_.write_land_json(combined, replace_ending(land_filename, ".parquet", ".json"));

        if(is_animal_enabled_){
            auto animal_dst = fmt::format("{}/{}_impbmpsubmittedanimal.parquet", emo_path, exec_uuid);
            misc_utilities::copy_file(fmt::format("{}/{}_impbmpsubmittedanimal.parquet", emo_path, ipopt_uuid), animal_dst );
        }

        if(is_manure_enabled_){
            auto manure_dst = fmt::format("{}/{}_impbmpsubmittedmanuretransport.parquet", emo_path, exec_uuid);
            misc_utilities::copy_file(fmt::format("{}/{}_impbmpsubmittedmanuretransport.parquet", emo_path, ipopt_uuid), manure_dst );
        }


        total_cost_map[exec_uuid] = scenario_.compute_cost(combined) + animal_cost + manure_cost;
    }

    auto results = scenario_.send_files(emo_uuid_, exec_uuid_vec);

    //ipopt_results
    auto dir_path = fmt::format("{}/config/{}", emo_path, sub_dir);
    misc_utilities::mkdir(dir_path);
    misc_utilities::copy_file(fmt::format("{}/config/ipopt.json", emo_path), fmt::format("{}/ipopt.json", dir_path));
    misc_utilities::copy_file(fmt::format("{}/config/ipopt2.json", emo_path), fmt::format("{}/ipopt2.json", dir_path));
    
    std::vector<std::vector<double>> result_fx;
    auto counter = 0;
    for (const auto& result : results) {
        std::vector<std::string> result_vec;
        misc_utilities::split_str(result, '_', result_vec);
        auto exec_uuid = result_vec[0];
        result_fx.push_back({total_cost_map[exec_uuid], std::stod(result_vec[1])});

        std::regex pattern (exec_uuid);
        auto str_replacement = std::to_string(counter);
        auto found_files =  misc_utilities::find_files(emo_path, exec_uuid);
        for (const auto& filename : found_files) {
            fmt::print("filename: {}\n", filename);
            auto filename_dst = std::regex_replace(filename, pattern, str_replacement);
            misc_utilities::copy_file(fmt::format("{}/{}", emo_path, filename), fmt::format("{}/{}", dir_path, filename_dst));
        }
        counter++;
    } 

    for(const auto& result :result_fx) {
        fmt::print("ipopt solution : [{}, {}]\n", result[0], result[1]);
    }
    exec_uuid_log_.push_back(exec_uuid_vec);
    save(result_fx, fmt::format("{}/config/{}/pareto_front_ipopt.txt", emo_path, sub_dir));
    fmt::print("end\n");
}


//std::vector<Particle> gbest_;
void PSO::exec_ipopt(){
    Execute execute;
    Particle particle_selected;
    bool flag = true;
    for (const auto& particle : gbest_) {
        //select particle with lowest  particle.get_fx()[1]
        if (flag || particle_selected.get_fx()[0] > particle.get_fx()[0]) {
            particle_selected = particle;
            flag = false;
        }
    }
    auto ipopt_uuid = particle_selected.get_uuid();
    fmt::print("ipopt_uuid: {}\n", ipopt_uuid);
    auto in_file = fmt::format("/opt/opt4cast/output/nsga3/{}/{}_reportloads.csv", emo_uuid_, ipopt_uuid);

    auto lc_cost = particle_selected.get_lc_cost();
    auto animal_cost = particle_selected.get_animal_cost();
    auto manure_cost = particle_selected.get_manure_cost();
    execute.set_files(emo_uuid_, in_file);
    execute.execute(emo_uuid_, 0.50, 6, 20);
    std::string exec_str = "/home/gtoscano/projects/MSUCast/build/eps_cnstr/eps_cnstr";

    std::string  in_path = "/opt/opt4cast/output/nsga3/592e98d5-2d52-4d25-99cb-76f88a6d4e09/config/reportloads_processed.json"; 
    std::string out_path = "/opt/opt4cast/output/nsga3/592e98d5-2d52-4d25-99cb-76f88a6d4e09/config/scenario.json";
    std::string uuids = "/opt/opt4cast/output/nsga3/592e98d5-2d52-4d25-99cb-76f88a6d4e09/config/uuids.json";
    "/opt/opt4cast/output/nsga3/592e98d5-2d52-4d25-99cb-76f88a6d4e09/front";
    int pollutant_idx = 0;
    double ipopt_reduction = 0.7;
    int ipopt_popsize = 20;
    execute.execute_local(
        in_path,
        out_path,
        pollutant_idx, //0
        ipopt_reduction, //0.30
        ipopt_popsize //10
        ); 
    fmt::print("Particle Selected Cost: {}\n", particle_selected.get_fx()[0]);
    execute.update_output(emo_uuid_, particle_selected.get_fx()[0]);
    fmt::print("======================== best_lc_cost_: {}\n", lc_cost);
    fmt::print("======================== best_animal_cost_: {}\n", animal_cost);
    fmt::print("======================== best_manure_cost_: {}\n", manure_cost);
    std::string sub_dir = "ipopt_results";
    evaluate_ipopt_sols(sub_dir, ipopt_uuid, animal_cost, manure_cost);

}

/*
void PSO::update_gbest() {
    for (int j = 0; j < nparts; j++) {
        const auto& new_solution_x = particles[j].get_x();
        const auto& new_solution_fx = particles[j].get_fx();
        update_non_dominated_solutions( gbest_x, new_solution_x, gbest_fx, new_solution_fx);
    } 
}
*/

void PSO::update_gbest() {
    for (int j = 0; j < nparts; j++) {
        update_non_dominated_solutions(gbest_, particles[j]);
    } 
}

void PSO::print() {
    std::cout << "gbest_fx: ";
    for(auto& row : gbest_fx) {
        for(auto& val : row) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }

    std::cout << "gbest_x: ";
    for(auto& row : gbest_x) {
        for(auto& val : row) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
    std::cout<<"--------------------------\n";
}

void PSO::evaluate() {
    std::vector<std::string> exec_uuid_vec;
    std::vector<double> total_cost_vec(nparts, 0.0);
    std::unordered_map<std::string, int> generation_uuid_idx;
    std::string emo_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", emo_uuid_);
    for (int i = 0; i < nparts; i++) {
        std::vector<std::tuple<int, int, int, int, double>> lc_x;
        std::vector<std::tuple<int, int, int, int, int, double>> animal_x;
        std::vector<std::tuple<int, int, int, int, int, double>> manure_x;
        std::unordered_map<std::string, double> amount_minus;
        std::unordered_map<std::string, double> amount_plus;
        double total_cost = 0.0;

        const auto& x = particles[i].get_x();
        std::string exec_uuid = xg::newGuid().str();
        particles[i].set_uuid(exec_uuid);
        bool flag = true;
        if(is_ef_enabled_){
            //total_cost += scenario_.normalize_ef(x, ef_x);
            //particles[i].set_ef_x(lc_x);
        }
        
        if(is_lc_enabled_){
            double lc_cost  = scenario_.normalize_lc(x, lc_x, amount_minus, amount_plus);
            particles[i].set_amount_minus(amount_minus);
            particles[i].set_amount_plus(amount_plus);
            particles[i].set_lc_cost(lc_cost);
            //fmt::print("lc_cost: {}\n", lc_cost);
            total_cost += lc_cost;
            particles[i].set_lc_x(lc_x);
            //fmt::print("exec_uuid: {}\n", exec_uuid);  
            auto land_filename = fmt::format("{}/{}_impbmpsubmittedland.parquet", emo_path, exec_uuid);
            scenario_.write_land(lc_x, land_filename);
            if (!std::filesystem::exists(land_filename)) {
                total_cost = 9999999999999.99;
                particles[i].set_lc_cost(lc_cost);
                particles[i].set_fx(total_cost, total_cost);
                flag = false;
                //continue;
            }

            scenario_.write_land_json(lc_x, replace_ending(land_filename, ".parquet", ".json"));
        }
        if(is_animal_enabled_){
            auto animal_cost = scenario_.normalize_animal(x, animal_x); 
            //fmt::print("animal_cost: {}\n", animal_cost);
            particles[i].set_animal_cost(animal_cost);
            total_cost += animal_cost;

            particles[i].set_animal_x(animal_x);
            auto animal_filename = fmt::format("{}/{}_impbmpsubmittedanimal.parquet", emo_path, exec_uuid);
            scenario_.write_animal(animal_x, animal_filename);
            if (!std::filesystem::exists(animal_filename)) {
                total_cost = 9999999999999.99;
                particles[i].set_animal_cost(animal_cost);
                particles[i].set_fx(total_cost, total_cost);
                flag = false;
                //continue;
            }
            scenario_.write_animal_json(animal_x, replace_ending(animal_filename, ".parquet", ".json"));
        }
        if(is_manure_enabled_){
            auto manure_cost = scenario_.normalize_manure(x, manure_x); 
            //fmt::print("manure_cost: {}\n", manure_cost);
            particles[i].set_manure_cost(manure_cost);
            total_cost += manure_cost;

            particles[i].set_manure_x(manure_x);
            auto manure_filename = fmt::format("{}/{}_impbmpsubmittedmanuretransport.parquet", emo_path, exec_uuid);
            scenario_.write_manure(manure_x, manure_filename);
            if (!std::filesystem::exists(manure_filename)) {
                total_cost = 9999999999999.99;
                particles[i].set_manure_cost(manure_cost);
                particles[i].set_fx(total_cost, total_cost);
                flag = false;
                //continue;
            }
            scenario_.write_manure_json(manure_x, replace_ending(manure_filename, ".parquet", ".json"));
        }

        if(flag){
            generation_uuid_idx[exec_uuid] = i;
            total_cost_vec[i] = total_cost;
            exec_uuid_vec.push_back(exec_uuid);
        }
    }

    //send files and wait for them
    auto results = scenario_.send_files(emo_uuid_, exec_uuid_vec);

    //for (int i = 0; i < nparts; i++) {
    for (auto const& key : results) {
        std::vector<std::string> result_vec;
        misc_utilities::split_str(key, '_', result_vec);
        auto stored_idx = generation_uuid_idx[result_vec[0]];
        particles[stored_idx].set_fx(total_cost_vec[stored_idx], std::stod(result_vec[1]));
    } 

    for (int i = 0; i < nparts; i++) {
        const auto& new_solution_fx = particles[i].get_fx();
        if (new_solution_fx[1] >= 9999999999999.0) {
            fmt::print("New solution fx[{}]: [{}, {}]\n", particles[i].get_uuid(), new_solution_fx[0], new_solution_fx[1]);

        }
        else {
            fmt::print("new_solution_fx[{}]: [{}, {}]\n", i, new_solution_fx[0], new_solution_fx[1]);
        }
    }
    exec_uuid_log_.push_back(exec_uuid_vec);
}

void PSO::save_gbest(std::string out_dir) {
    //create directory: out_dir
    //misc_utilities::mkdir(out_dir);

    auto pfront_path = fmt::format("{}/front", out_dir);
    std::string emo_path = fmt::format("/opt/opt4cast/output/nsga3/{}", emo_uuid_);
    int counter = 0;
    misc_utilities::mkdir(pfront_path);
    for(const auto& particle : gbest_) {
        const auto& uuid = particle.get_uuid();
        std::regex pattern (uuid);
        std::string str_replacement = std::to_string(counter);

        auto found_files =  misc_utilities::find_files(emo_path, uuid);
         
        for (const auto& filename : found_files) {
            //copy_file(file, fmt::format("{}/{}", pfront_path, );
            auto filename_dst = std::regex_replace(filename, pattern, str_replacement);
            misc_utilities::copy_file(fmt::format("{}/{}", emo_path, filename), fmt::format("{}/{}", pfront_path, filename_dst));
        }

        counter++;
    }
    auto x_filename = fmt::format("{}/pareto_set.txt", pfront_path);
    auto fx_filename = fmt::format("{}/pareto_front.txt", pfront_path);
    save(gbest_, x_filename, fx_filename);

    execute.get_files(emo_uuid_, fmt::format("{}/ipopt", out_dir));
    misc_utilities::copy_file(fmt::format("{}/ipopt/pfront_ef.txt", out_dir), fmt::format("{}/front/pfront_ef.txt", out_dir));

}




