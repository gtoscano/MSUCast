#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <coin-or/IpSmartPtr.hpp>
#include <coin-or/IpIpoptApplication.hpp>
#include "nlp.hpp"
#include <nlohmann/json.hpp>

#ifndef EPS_CNSTR_HPP
#define EPS_CNSTR_HPP
using json = nlohmann::json;

class EpsConstraint {
    // https://pdimov.github.io/blog/2020/09/07/named-parameters-in-c20/
    std::string path_out_;
    bool evaluate_cast_;
    //SmartPtr <TNLP> mynlp;
    SmartPtr <EPA_NLP> mynlp;
    SmartPtr <IpoptApplication> app;

    //EPA_NLP *mynlp;
public:

    EpsConstraint(const json& base_scenario_json, const json& scenario_json, const json& uuids_json, const std::string& path_out, int pollutant_idx, bool evaluate_cast);
    bool constr_eval(double, int);
    bool evaluate(double, int);

    std::vector<std::string> send_files(const std::string& scenario_data, const std::string& uuid, const std::vector<std::string>& uuids);
};

#endif
