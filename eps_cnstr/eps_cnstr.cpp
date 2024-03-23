// Created by: Gregorio Toscano Pulido
#include <iostream>
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

EpsConstraint::EpsConstraint(const json& base_scenario_json, const json& scenario_json, const json& uuids_json, const std::string& path_out, int pollutant_idx, bool evaluate_cast){ 
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

    mynlp = new EPA_NLP(base_scenario_json, scenario_json, uuids_json, path_out, pollutant_idx);
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

bool EpsConstraint::constr_eval(double reduction, int nsteps){

    auto uuid = mynlp->get_uuid();
    auto uuids_tmp  = mynlp->get_uuids();
    auto scenario_data = mynlp->get_scenario_data();
    std::vector<std::string> uuids(uuids_tmp.begin(), uuids_tmp.begin()+nsteps);


    double step_size = (double)reduction/nsteps;
    auto base_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", uuid);

    for (int i(0); i< nsteps; ++i) {
        double lower_bound = step_size*(i+1);
        auto result = evaluate(lower_bound, i);
        auto src_file = fmt::format("{}/{}_{}",path_out_, i,"impbmpsubmittedland.parquet");
        auto dst_file = fmt::format("{}/{}_impbmpsubmittedland.parquet",base_path, uuids_tmp[i]);
        misc_utilities::copy_file(src_file, dst_file);
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
    for (const auto& exec_uuid : uuids) {
        auto src_file = fmt::format("{}/{}_reportloads.parquet",base_path, exec_uuid);
        auto dst_file = fmt::format("{}/{}_reportloads.parquet",path_out_, i);
        misc_utilities::copy_file(src_file, dst_file);
        i++;
    }
    nlohmann::json j;
    j["output"] = output_rabbit;
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

