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

EpsConstraint::EpsConstraint(const std::shared_ptr<Parameters> &var) {
    filename_in = var->filename_in;
    filename_scenario = var->filename_scenario;
    filename_out = var->filename_out;
    reduction = var->reduction;
    pollutant_idx = var->pollutant_idx;
    log_filename = var->log_filename;
    nsteps = var->nsteps;
    evaluate_cast = var->evaluate_cast;



    mynlp = new EPA_NLP(filename_in, filename_scenario, filename_out, reduction, pollutant_idx);
    app = IpoptApplicationFactory();
}

bool EpsConstraint::evaluate(double reduction, int current_iteration=1) {
    //app->Options()->SetNumericValue("tol", 1e-8);
    int desired_verbosity_level = 1;

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


    double step_size = reduction/nsteps;
    auto base_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", uuid);

    for (int i(1); i<= nsteps; ++i) {
        double lower_bound = step_size*i;
        auto result = evaluate(lower_bound, i);
        auto prefix = "eps_cnstr";
        auto src_file = fmt::format("{}/config/{}_{}_{}",base_path, "eps_cnstr",i,"impbmpsubmittedland.parquet");
        auto dst_file = fmt::format("{}/{}_impbmpsubmittedland.parquet",base_path, uuids_tmp[i-1]);
        misc_utilities::copy_file(src_file, dst_file);
    }

    if(evaluate_cast) {
        send_files(scenario_data, uuid, uuids, base_path);
    }
    return true;
}


std::vector<std::string> EpsConstraint::send_files(const std::string& scenario_data, const std::string& uuid, const std::vector<std::string>& uuids, const std::string& path) {
    

    RabbitMQClient rabbit(scenario_data, uuid);
    fmt::print("senario_data: {} {}\n", scenario_data, uuid);

    for (const auto& exec_uuid : uuids) {
        rabbit.send_signal(exec_uuid);
    }

    auto output_rabbit = rabbit.wait_for_all_data();
    nlohmann::json j;
    j["output"] = output_rabbit;
    std::string output_path = fmt::format("{}/config/output_rabbit.json", path);
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

