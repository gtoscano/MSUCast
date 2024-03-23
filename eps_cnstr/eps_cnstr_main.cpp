// Author: Gregorio Toscano Pulido 
//epa exec_id 1.0-target_reduction target_pollutant algorithm_var 1
//epa 45 0.90 0 0 1

//[MAIN]

#include "IpIpoptApplication.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <fmt/core.h>
#include <coin-or/IpSmartPtr.hpp>
#include <unordered_map>
#include <memory>
#include <misc_utilities.h>
#include <nlp.hpp>
#include <eps_cnstr.h>
#include <nlohmann/json.hpp>
#include <stdexcept> // For std::runtime_error

using namespace Ipopt;
using json = nlohmann::json;
json read_json_file(const std::string& filename) {
    // Open the JSON file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open the base scenario file: " << filename << std::endl;
        exit(-1);
    }

    // Parse the JSON file directly into a nlohmann::json object
    json json_obj;
    try {
        json_obj = json::parse(file);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON file: " + filename + "; error: " + e.what());
    }
    return json_obj;
}

int main(
        int argc,
        char **argv
) {
    //        1             2           3  4  5   6
    //run Jefferson.json   output.json  0 0.9 5 
    //run input_base_scenario.json input_scenario.json output.json pollutant_id  target_reduction nsteps 
    std::string filename_in;
    std::string filename_scenario;
    std::string filename_uuids;
    std::string path_out;
    filename_in = argv[1];
    json base_scenario_json = read_json_file(filename_in);

    filename_scenario = argv[2];
    json scenario_json = read_json_file(filename_scenario);

    filename_uuids = argv[3];
    json uuids_json = read_json_file(filename_uuids);


    path_out = argv[4];
    int pollutant_idx = atoi(argv[5]); // 0
    double reduction= 1.0 - atof(argv[6]); // 0.9
    int nsteps = atoi(argv[7]); //5
    
    bool evaluate_cast = true;

    if (argc > 9) {
        evaluate_cast = ( atoi(argv[8]) == 1)? true : false;
    }

    fmt::print("filename_scenario: {}\n", filename_scenario);


    int option = 1;
    /*
    if (option == 0) { //ipopt Opt3
        EpsConstraint eps_constr(var);
        auto r = eps_constr.evaluate(var->reduction);
        //std::cout<<r<<std::endl;

        //std::vector<double> init_x;
        //eps_constr.get_final_x(init_x);
        //eps_constr.set_init_x(init_x);

        //r = eps_constr.evaluate(0.7);
        //std::cout<<r<<std::endl;
    }
    */
    if (option == 1) { //ipopt Opt3
        EpsConstraint eps_constr(base_scenario_json, scenario_json, uuids_json, path_out, pollutant_idx, evaluate_cast);
        eps_constr.constr_eval(reduction, nsteps);
    }
    /*
    if (option == 6) { //ipopt Opt3
        SmartPtr <TNLP> mynlp = new EPA_NLP(filename_in, filename_scenario, filename_out, reduction, pollutant_idx);
        SmartPtr <IpoptApplication> app = IpoptApplicationFactory();

        //app->Options()->SetNumericValue("tol", 1e-8);
        app->Options()->SetIntegerValue("max_iter", 10000);
        app->Options()->SetStringValue("linear_solver", "ma57");

        //app->Options()->SetStringValue("mu_strategy", "adaptive");
        app->Options()->SetStringValue("output_file", log_filename.c_str());
        app->Options()->SetStringValue("hessian_approximation", "limited-memory");

        ApplicationReturnStatus status;
        status = app->Initialize();
        if (status != Solve_Succeeded) {
            std::cout << std::endl << std::endl << "*** Error during initialization!" << std::endl;
            return (int) status;
        }

        // Ask Ipopt to solve the problem
        status = app->OptimizeTNLP(mynlp);
        std::ofstream file(log_filename, std::ios_base::app);

        if (status == Solve_Succeeded) {
            std::cout << std::endl << std::endl << "*** The problem solved!" << std::endl;
            file << "Solved\n";
        } else {
            std::cout << std::endl << std::endl << "*** The problem FAILED!" << std::endl;
            file << "Failed\n";
        }
        file.close();

    } else if (option == 2) { //random Opt1
        EPA_NLP *mynlp = new EPA_NLP(filename_in, filename_scenario, filename_out, reduction, pollutant_idx);
        Index n;
        Index m;
        mynlp->get_n_and_m(n, m);

        Number *x = new Number[n];
        Number obj_value;

        mynlp->random_x(n, m, x);
        //get_info(n, m);
        mynlp->my_get_starting_point(x);
        for(int i(0); i<n; i++){
            if(x[i]>0.1)
                std::cout<<i<<":"<<x[i]<<" ";
        }
        std::cout<<std::endl;
        mynlp->eval_f(n, x, true, obj_value);
        mynlp->write_files(n, x, m, obj_value);

        std::ofstream file(log_filename, std::ios_base::app);

        file << "Random Test\n";
        file.close();
        delete x;

    }*/
    return 0;
}

