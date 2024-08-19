#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <fmt/core.h>

class Execute {
    public:
        Execute() = default;
        
        void set_files(const std::string& emo_uuid, const std::string& report_loads_filename);
        void get_files(const std::string& emo_uuid, const std::string& path_to);
        void execute(const std::string& emo_uuid, double ipopt_reduction, int cost_profile_idx, int ipopt_popsize ); 

        void execute_local(
            const std::string& in_path,
            const std::string& out_path,
            int pollutant_idx, //0
            double ipopt_reduction, //0.30
            int ipopt_popsize //10
        );
        void execute_new(
            const std::string& base_scenario,
            const std::string& scenario,
            const std::string& out_path,
            int pollutant_idx, //0
            double ipopt_reduction, //0.30
            int nsteps,
            int evaluate_cast,
            std::string original_base_scenario
        ); 

        void get_json_scenario( size_t sinfo, const std::string& report_loads_path, const std::string& output_path_prefix);
        void update_output(const std::string& emo_uuid, double initial_cost);

        bool process_file(const std::string& source_path, const std::string& destination_path, float initial_cost);
};

