#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <unordered_map>
#include <filesystem>

#include <random>
#include <iomanip>
#include <chrono>
#include <ostream>
#include <stdexcept>
#include <fmt/core.h>

#include <crossguid/guid.hpp>


#include "json.hpp"
#include "misc_utilities.h"
#include "execute.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace my_execute{

    struct CommandResult {
        std::string output;
        int exitstatus;
        friend std::ostream &operator<<(std::ostream &os, const CommandResult &result) {
            os << "command exitstatus: " << result.exitstatus << " output: " << result.output;
            return os;
        }
        bool operator==(const CommandResult &rhs) const {
            return output == rhs.output &&
                   exitstatus == rhs.exitstatus;
        }
        bool operator!=(const CommandResult &rhs) const {
            return !(rhs == *this);
        }
    };

    class Command {
    public:
        /**
             * Execute system command and get STDOUT result.
             * Regular system() only gives back exit status, this gives back output as well.
             * @param command system command to execute
             * @return commandResult containing STDOUT (not stderr) output & exitstatus
             * of command. Empty if command failed (or has no output). If you want stderr,
             * use shell redirection (2&>1).
             */
        static CommandResult exec(const std::string &command) {
            int exitcode = 0;
            std::array<char, 1048576> buffer {};
            std::string result;
            FILE *pipe = popen(command.c_str(), "r");
            if (pipe == nullptr) {
                throw std::runtime_error("popen() failed!");
            }
            try {
                std::size_t bytesread;
                while ((bytesread = std::fread(buffer.data(), sizeof(buffer.at(0)), sizeof(buffer), pipe)) != 0) {
                    result += std::string(buffer.data(), bytesread);
                }
            } catch (...) {
                pclose(pipe);
                throw;
            }
            exitcode = WEXITSTATUS(pclose(pipe));
            return CommandResult{result, exitcode};
        }
    };
}


void Execute::set_files(const std::string& emo_uuid, const std::string& report_loads_filename) {
    std::string emo_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", emo_uuid);
    std::string dir_path = fmt::format("{}/config/", emo_path);
    misc_utilities::mkdir(dir_path);
    std::string filename_src = fmt::format("{}", report_loads_filename);
    auto filename_dst = fmt::format("{}/reportloads.csv", dir_path);
    misc_utilities::copy_file(filename_src, filename_dst); 
    filename_dst = fmt::format("/opt/opt4cast/input/{}_reportloads.csv", emo_uuid);
    misc_utilities::copy_file(filename_src, filename_dst); 

}

void Execute::get_files(const std::string& emo_uuid, const std::string& path_to) {
    std::string emo_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", emo_uuid);
    std::string dir_path = fmt::format("{}/config/", emo_path);
    misc_utilities::copy_full_directory(dir_path, path_to);
}

bool Execute::process_file(const std::string& source_path, const std::string& destination_path, float initial_cost) {
    std::ifstream infile(source_path);
    std::ofstream outfile(destination_path);

    // Check if the files are open successfully
    if (!infile.is_open() || !outfile.is_open()) {
        std::cerr << "Error opening files: " << source_path<< " "<< destination_path<<std::endl;
        return false;
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);

        float first_number, second_number;
        char comma;
        if (iss >> first_number >> comma >> second_number && comma == ',') {
            first_number += initial_cost;
            outfile << first_number << " " << second_number << std::endl;
        } else {
            std::cerr << "Error reading line: " << line << std::endl;
        }
    }

    infile.close();
    outfile.close();
    return true;
}


void Execute::update_output(const std::string& emo_uuid, double initial_cost) {
    std::string emo_path = fmt::format("/opt/opt4cast/output/nsga3/{}/", emo_uuid);
    std::string dir_path = fmt::format("{}/config/", emo_path);
    std::string filename_src = fmt::format("{}/ipopt_funcs.txt", dir_path);
    std::string filename_dst = fmt::format("{}/pfront_ef.txt", dir_path);
    process_file(filename_src, filename_dst, initial_cost);
}


void Execute::execute(const std::string& emo_uuid, double ipopt_reduction, int cost_profile_idx, int ipopt_popsize ) {
    /*
void Execute::execute(const std::string& emo_uuid, 
        const std::string& path,
        double ipopt_reduction, //0.30
        int cost_profile_idx, //state_id
        int ipopt_popsize //10
        ) {
        */
    int limit_alpha = 1;
    int ipopt = 1;  //1
    int pollutant_idx = 0;//0 
    std::string env_var = "OPT4CAST_EPS_CNSTR_PATH";
    //std::string EPS_CNSTR_PATH = misc_utilities::get_env_var(env_var, "/home/gtoscano/django/api4opt4/optimization/eps_cnstr/build/eps_cnstr");
    std::string EPS_CNSTR_PATH = misc_utilities::get_env_var(env_var, "/home/gtoscano/projects/MSUCast/build/eps_cnstr/eps_cnstr");
    auto report_loads="/opt/opt4cast/output/nsga3/592e98d5-2d52-4d25-99cb-76f88a6d4e09/config/reportloads_processed.json";
    auto scenario = "/opt/opt4cast/output/nsga3/592e98d5-2d52-4d25-99cb-76f88a6d4e09/config/scenario.json";
    auto uuids = "/opt/opt4cast/output/nsga3/592e98d5-2d52-4d25-99cb-76f88a6d4e09/config/uuids.json";
    auto front = "/opt/opt4cast/output/nsga3/592e98d5-2d52-4d25-99cb-76f88a6d4e09/front";
    //0 0.7 20
    std::string exec_string = fmt::format("{} {} {} {} {} {} {} {}",
                                          EPS_CNSTR_PATH, emo_uuid, ipopt_reduction, pollutant_idx, ipopt, limit_alpha, cost_profile_idx, ipopt_popsize);
    fmt::print("exec_string: {}\n", exec_string);
    using namespace my_execute;
    CommandResult nullbyteCommand = Command::exec(exec_string); // NOLINT(bugprone-string-literal-with-embedded-nul)
    std::ofstream ofile("/tmp/filename2.txt");
    ofile<<exec_string<<std::endl;
    ofile << "Output using fread: " << nullbyteCommand << std::endl;
    ofile.close();
}

void Execute::execute_local(
        const std::string& in_path,
        const std::string& out_path,
        int pollutant_idx, //0
        double ipopt_reduction, //0.30
        int ipopt_popsize //10
        ) {
    std::string env_var = "OPT4CAST_EPS_CNSTR_PATH";
    std::string EPS_CNSTR_PATH = misc_utilities::get_env_var("OPT4CAST_RUN_EPS_CNSTR_PATH", "/home/gtoscano/projects/MSUCast/build/eps_cnstr/eps_cnstr");
    // const std::string& emo_uuid, 
    // std::string path = fmt::format("/opt/opt4cast/output/nsga3/{}/config/", emo_uuid);
    std::string reportloads_json_path = fmt::format("{}/reportloads_processed.json", in_path);
    std::string scenario_json_path = fmt::format("{}/scenario.json", in_path);;;
    std::string uuids_json_path = fmt::format("{}/uuids.json", in_path);;;
    std::string exec_string = fmt::format("{} {} {} {} {} {} {} {} {}", 
                EPS_CNSTR_PATH, 
                reportloads_json_path, 
                scenario_json_path, 
                out_path, 
                pollutant_idx,
                ipopt_reduction, 
                ipopt_popsize,
                1,
                uuids_json_path 
                );

    fmt::print("exec_string: {}\n", exec_string);
    using namespace my_execute;
    CommandResult nullbyteCommand = Command::exec(exec_string); // NOLINT(bugprone-string-literal-with-embedded-nul)
    std::ofstream ofile(fmt::format("{}/filename2.txt", out_path));
    ofile<<exec_string<<std::endl;
    ofile << "Output using fread: " << nullbyteCommand << std::endl;
    ofile.close();
}

void Execute::execute_new(
        const std::string& base_scenario,
        const std::string& scenario,
        const std::string& out_path,
        int pollutant_idx, //0
        double ipopt_reduction, //0.30
        int nsteps,
        int evaluate_cast,
        std::string original_base_scenario
        ) {
    std::string env_var = "OPT4CAST_EPS_CNSTR_PATH";
    std::string EPS_CNSTR_PATH = misc_utilities::get_env_var("OPT4CAST_RUN_EPS_CNSTR_PATH", "/home/gtoscano/projects/MSUCast/build/eps_cnstr/eps_cnstr");
    // const std::string& emo_uuid, 
    // std::string path = fmt::format("/opt/opt4cast/output/nsga3/{}/config/", emo_uuid);
    std::string exec_string = fmt::format("{} {} {} {} {} {} {} {} {}", 
                EPS_CNSTR_PATH, 
                base_scenario,
                scenario,
                out_path,
                pollutant_idx,
                ipopt_reduction,
                nsteps,
                evaluate_cast,
                original_base_scenario
                );

    fmt::print("exec_string: {}\n", exec_string);
    using namespace my_execute;
    CommandResult nullbyteCommand = Command::exec(exec_string); // NOLINT(bugprone-string-literal-with-embedded-nul)
    std::ofstream ofile(fmt::format("{}/filename2.txt", out_path));
    ofile<<exec_string<<std::endl;
    ofile << "Output using fread: " << nullbyteCommand << std::endl;
    ofile.close();
}
void Execute::get_json_scenario(
        size_t sinfo,
        const std::string& report_loads_path,
        const std::string& output_path_prefix
        ) {

    std::string OPT4CAST_MAKE_SCENARIO_FILE_PATH = misc_utilities::get_env_var("OPT4CAST_MAKE_SCENARIO_FILE_PATH", "/home/gtoscano/projects/CastEvaluation/build/test/scenario_test");
    
    OPT4CAST_MAKE_SCENARIO_FILE_PATH = "/home/gtoscano/projects/CastEvaluation/build/test/scenario_test";

    std::string exec_string = fmt::format("{} {} {} {}", 
                OPT4CAST_MAKE_SCENARIO_FILE_PATH, 
                sinfo, 
                report_loads_path, 
                fmt::format("{}_reportloads_processed.json", output_path_prefix)
                );

    fmt::print("exec_string: {}\n", exec_string);
    using namespace my_execute;
    CommandResult nullbyteCommand = Command::exec(exec_string); // NOLINT(bugprone-string-literal-with-embedded-nul)
    std::ofstream ofile(fmt::format("{}_log_get_json_scenario.txt", output_path_prefix));
    ofile<<exec_string<<std::endl;
    ofile << "Output using fread: " << nullbyteCommand << std::endl;
    ofile.close();
}

