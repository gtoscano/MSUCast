//
// Created by gtoscano on 4/1/23.
//

#include "data_reader.h"
#include "misc_utilities.h"

#include <string>
#include <unordered_map>

#include <fmt/core.h>

#include "csv.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;


DataReader::DataReader() {
    std::string msu_cbpo_path = misc_utilities::get_env_var("MSU_CBPO_PATH", "/opt/opt4cast");
    csvs_path = fmt::format("{}/csvs",msu_cbpo_path);
}

DataReader::DataReader(std::string path) {
    csvs_path = path;
}

void DataReader::read_all() {
    read_u_to_u_group();
    read_lrseg_to_geography();
    read_load_src_to_bmp_list();
    read_animal_population();
    read_animal_grp_bmps();
    read_land_cost();
    read_animal_cost();
    read_land_bmp_cost();
    read_animal_bmp_cost();
    read_lrseg();
    read_scenario();
    read_geography_county();
}


void DataReader::read_scenario() {
    int counter = 0;

    auto filename = fmt::format("{}/TblScenario.csv", csvs_path);

    csv::CSVReader reader2(filename);
    for(auto& row: reader2) {
        try {

            //"ScenarioId","ScenarioName","ScenarioDescription","SourceDataRevisionId",
            // "BaseConditionId","ScenarioTypeId","BackoutScenarioId","NeienProgressRunId",
            // "PointSourceDataSetId","AtmDepDataSetId","ClimateChangeDataSetId",
            // "SoilPDataSetId","BaseLoadId","CostProfileId","CreateDate","UserId",
            // "ScenarioStatusId","IsEditable","IsCbpoScenario","Notes","IsPublicScenario",
            // "IsRestrictedByState","LastModifiedDate"
            auto ScenarioId = row["ScenarioId"].get<int>();
            json scenario_data;

            scenario_data["AtmDepDataSetId"] = row["AtmDepDataSetId"].get<int>();
            scenario_data["BackoutScenarioId"] = row["BackoutScenarioId"].get<int>();
            scenario_data["BaseConditionId"] = row["BaseConditionId"].get<int>();
            scenario_data["BaseLoadId"] = row["BaseLoadId"].get<int>();
            scenario_data["CostProfileId"] = row["CostProfileId"].get<int>();
            scenario_data["ClimateChangeDataSetId"] = row["ClimateChangeDataSetId"].get<int>();
            scenario_data["HistoricalCropNeedScenario"]= 6608;
            scenario_data["PointSourceDataSetId"] = row["PointSourceDataSetId"].get<int>();
            scenario_data["ScenarioTypeId"] = row["ScenarioTypeId"].get<int>();
            scenario_data["SoilPDataSetId"] = row["SoilPDataSetId"].get<int>();
            scenario_data["SourceDataRevisionId"]= row["SourceDataRevisionId"].get<int>();
            scenario_data_[ScenarioId] = scenario_data.dump();

            std::string scenario_name = "";
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
            std::string scenario_data2 = fmt::format("{}_{}_{}_{}_{}_{}_{}_{}_{}_{}_{}_{}",scenario_name,\
                                      atm_dep_data_set, back_out_scenario, base_condition,\
                                      base_load, cost_profile, climate_change_data_set,\
                                      historical_crop_need_scenario, point_source_data_set,\
                                      scenario_type, soil_p_data_set, source_data_revision);

            scenario_data2_[ScenarioId] = scenario_data2;
        } catch (std::exception &e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
        counter++;
    }
    fmt::print("Land River Segument Read {}\n", counter);
}

std::string DataReader::get_scenario_data(int scenario_id) {
    return scenario_data_.at(scenario_id);
}

std::string DataReader::get_scenario_data2(int scenario_id) {
    return scenario_data2_.at(scenario_id);
}

void DataReader::read_lrseg() {
    int counter = 0;

    auto filename = fmt::format("{}/TblLandRiverSegment.csv", csvs_path);

    csv::CSVReader reader2(filename);
    for(auto& row: reader2) {
        try {
            //"LrsegId","LandRiverSegment","LandSegmentGeographyId",
            // "LandSegment","RiverSegment","FIPS","StateId","CountyId",
            // "HgmrId","OutOfCBWS","AboveRIM","TotalAcres",
            // "TotalAcresIncludingTidalWetlands"

            int lrseg_id = row["LrsegId"].get<int>();
            int geography = row["LandSegmentGeographyId"].get<int>();
            int fips = row["FIPS"].get<int>();
            int state = row["StateId"].get<int>();
            int county = row["CountyId"].get<int>();
            lrseg_.push_back({lrseg_id, fips, state, county});
        } catch (std::exception &e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
        counter++;
    }
    fmt::print("Land River Segument Read {}\n", counter);
}

const std::vector<std::vector<int>>& DataReader::get_lrseg() const {
    return lrseg_;
}


void DataReader::read_geography_county() {
    int counter = 0;

    auto filename = fmt::format("{}/TblGeographyCounty.csv", csvs_path);

    csv::CSVReader reader(filename);
    for(auto& row: reader) {
        try {
            //GeographyId,CountyId,FIPS,CountyName,StateAbbreviation

            int county = row["CountyId"].get<int>();
            int geography = row["GeographyId"].get<int>();
            int geography2 = row["GeographyType2Id"].get<int>();
            std::string fips = row["FIPS"].get<std::string>();
            std::string county_name = row["CountyName"].get<std::string>();
            std::string state = row["StateAbbreviation"].get<std::string>();
            geography_county_[county] = {geography, geography2, fips, county_name, state};
        } catch (std::exception &e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
        counter++;
    }
    fmt::print("Geography County Read {}\n", counter);
}

const std::unordered_map<int, std::tuple<int, int, std::string, std::string, std::string> >& DataReader::get_geography_county() const {
    return geography_county_;
}

int DataReader::read_bmp_cost(const std::string& filename) {
    int counter = 0;
    csv::CSVReader reader2(filename);
    for(auto& row: reader2) {
        try {
            //"CostBmpSubmittedId","CostProfileId","SectorId","BmpId",
            // "LifespanYears","Capital","OandM","Opportunity","Notes",
            // "TotalCostPerUnit"

            int profile = row["CostProfileId"].get<int>();
            int bmp = row["BmpId"].get<int>();
            double cost = row["TotalCostPerUnit"].get<double>();
            auto key = fmt::format("{}_{}", profile, bmp);
            bmp_cost_[key] = cost;
            auto profile_str = std::to_string(profile);
            if (bmp_cost_idx_.contains(profile_str)) {
                auto tmp_lst =  bmp_cost_idx_.at(profile_str);
                tmp_lst.push_back(key);
                bmp_cost_idx_[profile_str] = tmp_lst;
            }
            else {
                bmp_cost_idx_[profile_str] = {key};
            }

        } catch (std::exception &e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
        counter++;
    }
    fmt::print("Bmp Cost read {}\n", counter);
    return counter;
}

void DataReader::read_land_cost() {
    auto filename = fmt::format("{}/TblCostBmpLand.csv", csvs_path);
    read_bmp_cost(filename);
}
void DataReader::read_animal_cost() {
    //CostBmpSubmittedId,CostProfileId,SectorId,BmpId,
    // AnimalId,LifespanYears,Capital,OandM,Opportunity,
    // Notes,TotalCostPerUnit
    auto filename = fmt::format("{}/TblCostBmpAnimal-reduced.csv", csvs_path);
    read_bmp_cost(filename);
}

double  DataReader::get_bmp_cost(std::string key) const {
    auto cost = bmp_cost_.at(key);
    return cost;
}

const std::unordered_map<std::string, double>& DataReader::get_bmp_cost() const {
    return bmp_cost_;
}

const std::vector<std::string>& DataReader::get_bmp_cost_idx(const std::string& profile) const {
    return bmp_cost_idx_.at(profile);
}

void DataReader::read_animal_grp_bmps() {
    int counter = 0;

    auto filename = fmt::format("{}/TblAnimalGrpBmp.csv", csvs_path);

    csv::CSVReader reader2(filename);
    for(auto& row: reader2) {
        try {
            int animal_grp = row["AnimalGrp"].get<int>();
            int bmp = row["Bmp"].get<int>();

            if (animal_grp_bmps_.contains(animal_grp)) {
                auto tmp_lst =  animal_grp_bmps_.at(animal_grp);
                tmp_lst.push_back(bmp);
                animal_grp_bmps_[animal_grp] = tmp_lst;
            }
            else {
                animal_grp_bmps_[animal_grp] = {bmp};
            }

        } catch (std::exception &e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
        counter++;
    }
    fmt::print("Animal Grp Bmp read {}\n", counter);
}

const std::unordered_map<int, std::vector<int>>& DataReader::get_animal_grp_bmps() const {
    return animal_grp_bmps_;
}

void DataReader::read_animal_population() {
    int counter = 0;

    auto filename = fmt::format("{}/TblAnimalPopulation-filtered.csv", csvs_path);

    csv::CSVReader reader2(filename);
    for(auto& row: reader2) {
        try {
            int base_condition = row["BaseConditionId"].get<int>();
            int county = row["CountyId"].get<int>();
            int load_source = row["LoadSourceId"].get<int>();
            int animal_id = row["AnimalId"].get<int>();
            double animal_units = row["AnimalUnits"].get<double>();

            auto key = fmt::format("{}_{}_{}_{}", base_condition, county, load_source, animal_id);
            auto key_base_county = fmt::format("{}_{}", base_condition, county);
            animal_[key] = animal_units;

            if (animal_idx_.contains(key_base_county)) {
                auto tmp_lst = animal_idx_.at(key_base_county);
                tmp_lst.push_back(key);
                animal_idx_[key_base_county] = tmp_lst;
            }
            else {
                animal_idx_[key_base_county] = {key};
            }

        } catch (std::exception &e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
        counter++;
    }
    fmt::print("Animal read {}\n", counter);
}

const std::unordered_map<std::string, double>& DataReader::get_animal() const {
    return animal_;
}

double DataReader::get_animal(const std::string& key) {
    return animal_.at(key);
}

const std::vector<std::string>& DataReader::get_animal_idx(std::string key) const {
    return animal_idx_.at(key);
}

void DataReader::read_load_src_to_bmp_list() {
    int counter = 0;

    auto filename = fmt::format("{}/TblBmpLoadSourceFromTo.csv", csvs_path);

    csv::CSVReader reader2(filename);
    for(auto& row: reader2) {
        try {
            int bmp_id = row["BmpId"].get<int>();
            int load_src_from = row["FromLoadSourceId"].get<int>();
            int load_src_to = row["ToLoadSourceId"].get<int>();
            auto key = fmt::format("{}_{}", bmp_id, load_src_from);
            lc_bmp_from_to[key] = load_src_to;
            if (load_src_to_bmp_list.contains(load_src_from)) {
                auto tmp_list =  load_src_to_bmp_list[load_src_from];
                tmp_list.push_back(bmp_id);
                load_src_to_bmp_list[load_src_from] = tmp_list;
            }
            else {
                load_src_to_bmp_list[load_src_from] = {bmp_id};
            }

        } catch (std::exception &e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
        counter++;
    }
    fmt::print("Load sources to load sources read {}\n", counter);
}
/* for livestock everything can apply except 69 (pultred, 203)
 * For poultry we can apply everying.
 * Livestoc, animalgroup = 20, and poultry animalgroup 17 */

const std::unordered_map<int, std::vector<int>>& DataReader::get_load_src_to_bmp_dict() const {
    return load_src_to_bmp_list;
}

const std::vector<int>& DataReader::get_load_src_to_bmp_list(int load_src) const {
    return load_src_to_bmp_list.at(load_src);
}

const std::unordered_map<std::string, int>& DataReader::get_lc_bmp_from_to() const {
    return lc_bmp_from_to;
}

int  DataReader::get_lc_bmp_from_to(std::string lc_bmp_to) const {
    return lc_bmp_from_to.at(lc_bmp_to);
}


void DataReader::read_u_to_u_group() {
    int counter = 0;

    auto filename = fmt::format("{}/load_src_to_load_src_grp.csv", csvs_path);
    csv::CSVReader reader2(filename);
    for(auto& row: reader2) {
        try {
            int load_src = row["LoadSrcId"].get<int>();
            int load_src_grp= row["LoadSrcGrpId"].get<int>();
            u_u_group_dict[load_src] = load_src_grp;
        } catch (std::exception &e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
        counter++;
    }
    fmt::print("Load sources to load sources read {}\n", counter);
}


const std::unordered_map<int, int>& DataReader::get_u_u_groups() const{
    return u_u_group_dict;
}

int DataReader::get_u_u_group(int load_source) const {
    if (u_u_group_dict.contains(load_source)) {
        int ret_val = u_u_group_dict.at(load_source);
        return ret_val;
    }
    else
        return -1;
}

void DataReader::read_lrseg_to_geography() {
    auto filename = fmt::format("{}/{}", csvs_path, "lrseg_geo.csv");
    csv::CSVReader reader(filename);
    int counter =0;
    for(auto& row: reader) {
        try {
            int lrseg = row["LrsegId"].get<int>();
            int geography = row["GeographyId"].get<int>();
            int state = row["StateId"].get<int>();
            s_geography_dict[lrseg] = geography;
            s_state_dict[lrseg] = state;
        } catch (std::exception &e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
        counter++;
    }
    fmt::print("Geographies read counter {}\n", counter);
}

const std::unordered_map<int, int>& DataReader::get_geographies() const {
    return s_geography_dict;
}

int DataReader::get_geography(int lrseg) const {
    if (s_geography_dict.contains(lrseg))
        return s_geography_dict.at(lrseg);
    else
        return -1;
}

const std::unordered_map<int, int>& DataReader::get_states() const {
    return s_state_dict;

}

int DataReader::get_state(int lrseg) const {
    if (s_state_dict.contains(lrseg))
        return s_state_dict.at(lrseg);
    else
        return -1;
}


int DataReader::read_bmp_cost2(const std::string& filename) {
    int counter = 0;
    csv::CSVReader reader(filename);

    for(auto& row: reader) {
        try {

            int profile = row["CostProfileId"].get<int>();
            int bmp = row["BmpId"].get<int>();
            double cost = row["TotalCostPerUnit"].get<double>();
            bmp_cost_dict_[profile][bmp]=cost;
            ++counter;
        } catch (std::exception& e) {
            // Catch any exceptions that derive from std::exception
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            // Catch any other exceptions
            std::cerr << "Caught unknown exception." << std::endl;
        }
    }
    return counter;
}

void DataReader::read_land_bmp_cost() {
    auto filename = fmt::format("{}/TblCostBmpLand.csv", csvs_path);
    read_bmp_cost2(filename);
}

void DataReader::read_animal_bmp_cost() {
    auto filename = fmt::format("{}/TblCostBmpAnimal-reduced.csv", csvs_path);
    read_bmp_cost2(filename);
}
const std::unordered_map<int, std::unordered_map<int, double>>& DataReader::get_land_bmp_costs() const{
    return bmp_cost_dict_;
}

double DataReader::get_bmp_cost(int profile, int bmp) const {
    if (bmp_cost_dict_.contains(profile)){
        if (bmp_cost_dict_.at(profile).contains(bmp)) {
            return bmp_cost_dict_.at(profile).at(bmp);
        }
    }
    return -1;
}

