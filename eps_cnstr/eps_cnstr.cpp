// Created by: Gregorio Toscano Pulido
#include <iostream>
#include <crossguid/guid.hpp>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>
#include "amqp.h"
#include "eps_cnstr.h"
#include "misc_utilities.h"
#include "fmt/core.h"
#include <filesystem>

namespace fs = std::filesystem;

EpsConstraint::EpsConstraint(const json& base_scenario_json, const json& scenario_json, const std::string& path_out, int pollutant_idx, bool evaluate_cast){ 
    path_out_ = path_out;
    // Check if the directory exists
    if (!fs::exists(path_out)) {
        // Create the directory and any necessary parent directories
        try {
            if (fs::create_directories(path_out)==false) {
                std::cout << "Directory already exists or cannot be created: " << path_out<< std::endl;
            }
        } catch (fs::filesystem_error& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    evaluate_cast_ = evaluate_cast;

    mynlp = new EPA_NLP(base_scenario_json, scenario_json, path_out, pollutant_idx);
    app = IpoptApplicationFactory();
}

bool EpsConstraint::evaluate(double reduction, int current_iteration=0) {
    //app->Options()->SetNumericValue("tol", 1e-8);
    int desired_verbosity_level = 1;
    std::string log_filename = fmt::format("{}/ipopt.out", path_out_);

    mynlp->update_reduction(reduction, current_iteration);
    app->Options()->SetIntegerValue("max_iter", 1000);
    app->Options()->SetStringValue("linear_solver", "ma57");

    app->Options()->SetStringValue("output_file", log_filename.c_str());
    app->Options()->SetIntegerValue("print_level", desired_verbosity_level);
    app->Options()->SetStringValue("hessian_approximation", "limited-memory");

    ApplicationReturnStatus status;
    status = app->Initialize();
    if (status != Solve_Succeeded) {
        std::cout << std::endl << std::endl << "*** Error during initialization!" << std::endl;
        return (int) status;
    }

    status = app->OptimizeTNLP(mynlp);
    //mynlp->save_files(n, x);
    return status;
}


bool EpsConstraint::constr_eval(double reduction, int nsteps, const std::vector<std::string>& uuids, const std::string& parent_uuid_path){
    fmt::print("**************************************** \n");
    fmt::print("In constr_eval\n");
    fmt::print("**************************************** \n");

    auto uuid = mynlp->get_uuid();
    fmt::print("eps-uuid: {}\n", uuid);


    auto scenario_data = mynlp->get_scenario_data();


    double step_size = (double)reduction/nsteps;
    auto base_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", uuid);
    misc_utilities::mkdir(fmt::format("{}/ipopt_tmp", base_path));
    //auto [parent_path_part, parent_uuid_part] = misc_utilities::extract_path_and_id(parent_uuid_path);
    //I want to have a dictionary with a uuid as key, and an integer (i) as value;

    misc_utilities::mkdir(fmt::format("{}/ipopt_tmp", base_path));
    for (int i(0); i< nsteps; ++i) {
        double lower_bound = step_size*(i+1);
        auto result = evaluate(lower_bound, i);
        fmt::print("Result: {}\n", result);

        //copy the files from the parent_uuid_path to the base_path 
        auto [parent_path, parent_uuid] = misc_utilities::extract_path_and_id(parent_uuid_path);
        misc_utilities::copy_prefix_in_to_prefix_out(parent_path, base_path, parent_uuid, uuids[i]);
        //merge eps-constr's impbmpsubmittedland.parquet, impbmpsubmittedland.json and costs.json with parent's

        auto parent_land_path = fmt::format("{}_impbmpsubmittedland.parquet", parent_uuid_path);
        auto current_land_path = fmt::format("{}/ipopt_tmp/{}_{}", base_path, i,"impbmpsubmittedland.parquet");
        auto dst_land_path = fmt::format("{}/{}_impbmpsubmittedland.parquet", base_path, uuids[i]);


        std::vector<std::tuple<int, int, int, int, int, int, double> > parent_land = mynlp->read_land(parent_land_path);
        std::vector<std::tuple<int, int, int, int, int, int, double> >  current_land = mynlp->read_land(current_land_path);
        parent_land.insert(parent_land.end(), current_land.begin(), current_land.end());
        mynlp->write_land_barefoot(parent_land, dst_land_path);

        auto parent_land_json_path = fmt::format("{}_impbmpsubmittedland.json", parent_uuid_path);
        auto current_land_json_path = fmt::format("{}/ipopt_tmp/{}_{}", base_path, i,"impbmpsubmittedland.json");
        auto dst_land_json_path = fmt::format("{}/{}_impbmpsubmittedland.json", base_path, uuids[i]);

        /*
        json parent_land_json = misc_utilities::read_json_file(parent_land_json_path);
        json current_land_json = misc_utilities::read_json_file(current_land_json_path);
        */


        misc_utilities::merge_json_files(current_land_json_path,
                        parent_land_json_path, 
                        dst_land_json_path);

        auto current_cost_path = fmt::format("{}/ipopt_tmp/{}_costs.json", base_path, i);
        auto parent_cost_path = fmt::format("{}_costs.json", parent_uuid_path);
        auto dst_cost_path = fmt::format("{}/{}_costs.json", base_path, uuids[i]);

        auto parent_cost_json = misc_utilities::read_json_file(parent_cost_path);
        auto current_cost_json = misc_utilities::read_json_file(current_cost_path);

        if (current_cost_json.contains("ef_cost")) {
            parent_cost_json["ef_cost"] = current_cost_json["ef_cost"];
        }
                
        misc_utilities::write_json_file(dst_cost_path, parent_cost_json);
        
    }

    if(evaluate_cast_) {
        send_files(scenario_data, uuid, uuids);
    }
    return true;
}


std::vector<std::string>  EpsConstraint::send_files(const std::string& scenario_data, const std::string& uuid, const std::vector<std::string>& uuids) {
    

    RabbitMQClient rabbit(scenario_data, uuid);
    fmt::print("senario_data: {} {}\n", scenario_data, uuid);

    for (const auto& exec_uuid : uuids) {
        rabbit.send_signal(exec_uuid);
    }

    auto output_rabbit = rabbit.wait_for_all_data();
    int i = 0;
    auto base_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", uuid);

    std::unordered_map<std::string, int> uuids_map;

    for (const auto& exec_uuid : uuids) {
        misc_utilities::copy_prefix_in_to_prefix_out(base_path, path_out_, uuids[i], fmt::format("{}",i));
        uuids_map[exec_uuid] = i;
        i++;
    }

    for (const auto& output_str: output_rabbit) {
        std::vector<std::string> output_tmp;
        misc_utilities::split_str(output_str, '_', output_tmp);
        fmt::print("output_str: {}\n", output_str);
        auto i = uuids_map[output_tmp[0]];

        auto src_cost_file = fmt::format("{}/{}_costs.json", base_path, uuids[i]);
        json output_json = misc_utilities::read_json_file(src_cost_file);
        json loads_json = misc_utilities::read_loads(fmt::format("{}/{}_reportloads.parquet", path_out_, i));

        output_json.merge_patch(loads_json);

        output_json["uuid"] = output_tmp[0];

        // Assign default values of 0.0 if they do not exist or are not numbers
        output_json["ef_cost"] = output_json.value("ef_cost", 0.0);
        output_json["lc_cost"] = output_json.value("lc_cost", 0.0);
        output_json["animal_cost"] = output_json.value("animal_cost", 0.0);
        output_json["manure_cost"] = output_json.value("manure_cost", 0.0);

        // Calculate the total cost
        output_json["cost"] = output_json["ef_cost"].get<double>() + output_json["lc_cost"].get<double>() + output_json["animal_cost"].get<double>() + output_json["manure_cost"].get<double>();
        output_json["i"] = i; 

        auto dst_cost_file = fmt::format("{}/{}_costs.json", path_out_, i);
        misc_utilities::write_json_file(dst_cost_file, output_json);
    }


    nlohmann::json j;
    j["output"] = output_rabbit;
    //fmt::print("{}\n", output_rabbit);
    std::string output_path = fmt::format("{}/output_rabbit.json", path_out_);
    std::ofstream out_file(output_path);
    // Check if the file is open
    if (out_file.is_open()) {
        // Write the JSON object to the file
        out_file << j.dump(4); // `dump(4)` converts the JSON object to a string with an indentation of 4 spaces
        // Close the file
        out_file.close();
    } else {
        // If the file couldn't be opened, print an error message
        std::cerr << "Unable to open file" << std::endl;
    }

    return output_rabbit;
}

