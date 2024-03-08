//
// Created by gtoscano on 4/1/23.
//

#ifndef DATA_READER_H
#define DATA_READER_H

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @class DataReader
 * @brief A class to read all required data
 *
 * This class reads the different requiered data by the CBWOP
 */
class DataReader {
public:
    /**
     * Constructor reads an environment variable to set up csvs_path
     */
    DataReader();
    /**
     * Constructor assign the parameter input to csvs_path
     *
     * @param path accepts the path to look for the data
     */
    DataReader(std::string path);
    /**
     * Reads all CSV files.
     */
    void read_all();

    void read_scenario();

    std::string get_scenario_data(int scenario_id);
    std::string get_scenario_data2(int scenario_id);
    void read_lrseg();
    const std::vector<std::vector<int>>& get_lrseg() const;
    int read_bmp_cost(const std::string& filename);
    void read_land_cost();
    void read_animal_cost();
    const std::vector<std::string>& get_bmp_cost_idx(const std::string& profile) const;
    const std::unordered_map<std::string, double>& get_bmp_cost() const;
    double  get_bmp_cost(std::string key) const;

    void read_animal_grp_bmps();
    const std::unordered_map<int, std::vector<int>>& get_animal_grp_bmps() const;
    void read_animal_population();
    const std::unordered_map<std::string, double>& get_animal() const;
    double get_animal(const std::string& key);
    const std::vector<std::string>& get_animal_idx(std::string key) const;

    /**
     * Reads a CSV file containing the load source to load group information
     */
    void read_load_src_to_bmp_list();
    /**
     * Get the dictionary containing the load source to load source group
     * @return a unordered_map<int,int> with the map load source -> load source group
     */

    const std::unordered_map<int, std::vector<int>>& get_load_src_to_bmp_dict() const;
    /**
     * Get the dictionary containing the load source to load source group
     * @return a unordered_map<int,int> with the map load source -> load source group
     */

    const std::vector<int>& get_load_src_to_bmp_list(int load_src) const;
    /**
     * Get the dictionary containing the load source to load source group
     * @return a unordered_map<int,int> with the map load source -> load source group
     */

    const std::unordered_map<std::string, int>& get_lc_bmp_from_to() const;
    /**
     * Get the dictionary containing the load source to load source group
     * @return a unordered_map<int,int> with the map load source -> load source group
     */

    int get_lc_bmp_from_to(std::string lc_bmp_to) const;
    /**
     * Reads a CSV file containing the load source to load group information
     */
    void read_u_to_u_group();
    /**
     * Get the dictionary containing the load source to load source group
     * @return a unordered_map<int,int> with the map load source -> load source group
     */
    const std::unordered_map<int, int>& get_u_u_groups() const;
    /**
     * It provides  the load source group by a given load source. If load source is not
     * in the dictionary it returns -1
     *
     * @param load_source
     * @return the load source group if it is in the dictionary otherwise returns -1
     */
    int get_u_u_group(int load_source) const;
    /**
     * Reads a CSV file containing the lrseg to geography and state information.
     */
    void read_lrseg_to_geography();
    /**
     * Get the dictionary containing the load source to load source group
     *
     * @return a unordered_map<int,int> with the map lrseg -> geography
     */
    const std::unordered_map<int, int>& get_geographies() const;
    /**
     * It provides  the geography of a given lrseg. If lrseg is not
     * in the dictionary it returns -1
     *
     * @param lrseg
     * @return the geography if it is in the dictionary otherwise returns -1
     */
    int get_geography(int lrseg) const;

    void read_geography_county();
    const std::unordered_map<int, std::tuple<int, int, std::string, std::string, std::string> >& get_geography_county() const;
    /**
     * Get the dictionary containing the load source to load source group
     *
     * @return a unordered_map<int,int> with the map lrseg -> state
     */

    const std::unordered_map<int, int>& get_states() const;
    /**
     * It provides  the state of a given lrseg. If lrseg is not
     * in the dictionary it returns -1
     *
     * @param lrseg
     * @return the state if it is in the dictionary otherwise returns -1
     */
    int get_state(int lrseg) const;
    /**
     * Reads a CSV file containing the cost per unit for every land BMP
     */
    int read_bmp_cost2(const std::string& filename);

    /**
     * Reads a CSV file containing the cost per unit for every land BMP
     */
    void read_land_bmp_cost();

    /**
     * Reads a CSV file containing the cost per unit for every land BMP
     */
    void read_animal_bmp_cost();

    /**
     * Get the dictionary containing the cost for all land BMPs.
     *
     * @return a unordered_map<int,unordered_map<int,double>> with the map profile -> bmp -> total cost per unit
     */
    const std::unordered_map<int, std::unordered_map<int,double>>& get_land_bmp_costs() const;
    /**
     * It provides  the cost per unit for  a given bmp in a given profile. If bmp is not
     * in the dictionary it returns -1
     *
     * @param profile the profile cost.
     * @param bmp The BMP
     *
     * @return the BMP's total cost per unit for the bmp in the profile cost otherwise returns -1
     */
    double get_bmp_cost(int profile, int bmp) const;

private:
    std::unordered_map<int, int> u_u_group_dict; ///< A map<int,int>: load source -> load source group.
    std::unordered_map<int, int> s_geography_dict; ///< A map<int,int>: lrseg -> geography.
    std::unordered_map<int, std::tuple<int, int, std::string, std::string, std::string> > geography_county_;
    std::unordered_map<int, int> s_state_dict; ///< A map<int,int>: lrseg -> state.
    std::unordered_map<int, std::unordered_map<int, double>> bmp_cost_dict_; ///< A map<int,<int,double>>: cost_profile -> bmp -> total cost per unit.
    std::unordered_map<std::string, double> bmp_cost_;
    std::unordered_map<int, std::vector<int>> load_src_to_bmp_list;
    std::unordered_map<std::string, int> lc_bmp_from_to;
    std::unordered_map<std::string, double> animal_;
    std::unordered_map<std::string, std::vector<std::string>> animal_idx_;
    std::unordered_map<int, std::vector<int>> animal_grp_bmps_;

    std::unordered_map<std::string, std::vector<std::string>> bmp_cost_idx_;
    std::vector<std::vector<int>> lrseg_;
    std::unordered_map<int, std::string> scenario_data_;
    std::unordered_map<int, std::string> scenario_data2_;
    std::string csvs_path; ///< stores the path where the CSV files are stored.
};

#endif //DATA_READER_H
