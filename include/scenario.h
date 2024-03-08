// Created by: Gregorio Toscano

#ifndef SCENARIO_H
#define SCENARIO_H
#include <vector>
#include <string>
#include <unordered_map>

class Scenario {
    public:
        Scenario();
        void load(const std::string& filename, const std::string& filename_scenario);
        void load_neighbors(const std::string& filename);
        void compute_efficiency_keys();
        void compute_lc_keys();
        void compute_animal_keys();
        void compute_manure_keys();
        size_t compute_efficiency_size();
        size_t compute_lc_size();
        size_t compute_animal_size();
        size_t compute_manure_size();
        int compute_ef();
        size_t get_lc_size() {
            return lc_size_;
        }
        size_t get_animal_size() {
            return animal_size_;
        }
        size_t get_manure_size() {
            return manure_size_;
        }
        void initialize_vector(std::vector<double>& x);
        void init(const std::string& filename, const std::string& scenario_filename, bool is_ef_enabled, bool is_lc_enabled, bool is_animal_enabled , bool is_manure_enabled);
        size_t get_nvars() {
            return nvars_;
        }
        size_t get_lc_begin() {
            return lc_begin_;
        }
        size_t get_animal_begin() {
            return animal_begin_;
        }
        size_t get_manure_begin() {
            return manure_begin_;
        }

        double normalize_lc(const std::vector<double>& x, 
            std::vector<std::tuple<int, int, int, int, double>>& lc_x,
            std::unordered_map<std::string, double>& amount_minus,
            std::unordered_map<std::string, double>& amount_plus);

        double normalize_animal(const std::vector<double>& x, std::vector<std::tuple<int, int, int, int, int, double>>& animal_x); 
        double normalize_manure(const std::vector<double>& x, std::vector<std::tuple<int, int, int, int, int, double>>& manure_x); 
        int write_land(const std::vector<std::tuple<int, int, int, int, double>>& lc_x,
        const std::string& out_filename);

        int write_animal ( const std::vector<std::tuple<int, int, int, int, int, double>>& animal_x, const std::string& out_filename);
        int write_manure ( const std::vector<std::tuple<int, int, int, int, int, double>>& manure_x, const std::string& out_filename);
        std::vector<std::string> send_files(const std::string& emo_uuid, const std::vector<std::string>& exec_uuid_vec);
        size_t write_land_json( const std::vector<std::tuple<int, int, int, int, double>>& lc_x, const std::string& out_filename);
        size_t write_animal_json(const std::vector<std::tuple<int, int, int, int, int, double>>& animal_x , const std::string& out_filename);
        size_t write_manure_json(const std::vector<std::tuple<int, int, int, int, int, double>>& manure_x , const std::string& out_filename);

        double get_alpha(std::string key) {return amount_[key];}
        const std::unordered_map<std::string, double> get_alpha() const {return amount_;}

        double compute_cost(const std::vector<std::tuple<int, int, int, int, double>>& parcel);

        double compute_cost_animal(const std::vector<std::tuple<int, int, int, int, int, double>>& parcel);
        double compute_cost_manure(const std::vector<std::tuple<int, int, int, int, int, double>>& parcel);
        std::unordered_map<std::string, double> read_manure_nutrients(const std::string& filename);

    private:
        size_t ef_size_;
        size_t lc_size_;
        size_t animal_size_;
        size_t manure_size_;

        size_t nvars_; 
        size_t ef_begin_;
        size_t lc_begin_;
        size_t animal_begin_;
        size_t manure_begin_;

        size_t ef_end_;

        bool is_ef_enabled;
        bool is_lc_enabled;
        bool is_animal_enabled;
        bool is_manure_enabled;

        std::string csvs_path;
        std::string scenario_data_str_;

        std::vector<std::string> ef_keys_;
        std::vector<std::string> lc_keys_;
        std::vector<std::string> animal_keys_;
        std::vector<std::string> manure_keys_;

        std::unordered_map<std::string, double> amount_;
        std::unordered_map<int, int> counties_;

        std::unordered_map<std::string, std::vector<std::vector<int>>> efficiency_;
        double alpha_plus_minus(const std::string& key, double original_amount, 
            const std::unordered_map<std::string, double>& amount_minus,
            const std::unordered_map<std::string, double>& amount_plus);
        void load_alpha(std::vector<double>& my_alpha, 
                const std::unordered_map<std::string, double>& amount_minus,
                const std::unordered_map<std::string, double>& amount_plus);
        double alpha_minus(const std::string& key, double original_amount, const std::unordered_map<std::string, double>& amount_minus );

        std::unordered_map<std::string, std::vector<std::string>> land_conversion_from_bmp_to;
        std::unordered_map<std::string, double> bmp_cost_;
        std::unordered_map<std::string, std::vector<int>> animal_complete_;
        std::unordered_map<std::string, double> animal_;
        std::unordered_map<int, std::tuple<int, int, int, int>> lrseg_dict_;
        std::unordered_map<int, int> u_u_group_dict; ///< A map<int,int>: load source -> load source group.
        std::unordered_map<int, std::tuple<int, int, std::string, std::string, std::string> > geography_county_;
        std::unordered_map<int, double> pct_by_valid_load_;
        /*
        double alpha_minus(const std::string& key, double original_amount);
        double alpha_plus_minus(const std::string& key, double original_amount);
        void compute_lc(); 
        void load_alpha(std::vector<double>& my_alpha); 
        */
        void sum_alpha(std::unordered_map<std::string, double>& am,  const std::string& key, double acreage);
        std::vector<std::string> valid_lc_bmps_;
        //double inject_lc_x();
        //
        std::unordered_map<std::string, std::vector<int>> neighbors_dict_;

        std::unordered_map<std::string, std::vector<double>> eta_dict_;
        std::unordered_map<std::string, std::vector<double>> phi_dict_;
        std::unordered_map<std::string, std::vector<std::vector<double>>> ef_x_;
        
        //std::unordered_map<std::string, std::vector<int>> lrseg_;

        std::vector<double> sum_load_invalid_;
        std::vector<double> sum_load_valid_;
        double normalize_efficiency(const std::vector<double>& x, 
    std::unordered_map<std::string, std::vector<std::vector<double>>> ef_x
        ); 
        double best_land_cost_;
        double best_animal_cost_;
        double best_manure_cost_;

        int load_to_opt_;
        void compute_eta();
        std::vector<std::string> manure_counties_;
        std::unordered_map<std::string, std::vector<int>> manure_all_; 
        std::unordered_map<std::string, double> manure_dry_lbs_;

};
#endif


