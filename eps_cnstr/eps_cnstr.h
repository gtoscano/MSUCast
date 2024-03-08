#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <coin-or/IpSmartPtr.hpp>
#include <coin-or/IpIpoptApplication.hpp>
#include "nlp.hpp"

#ifndef EPS_CNSTR_HPP
#define EPS_CNSTR_HPP

// https://dev.to/zigabrencic/parameters-object-in-c-1an9
class Parameters {
public:
    std::string filename_in;
    std::string filename_scenario;
    std::string filename_out;
    double reduction;
    int pollutant_idx;
    std::string log_filename;
    int nsteps;
    bool evaluate_cast;
    Parameters() {
    }
};

class EpsConstraint {
    // https://pdimov.github.io/blog/2020/09/07/named-parameters-in-c20/
    std::string filename_in;
    std::string filename_scenario;
    std::string filename_out;
    double reduction;
    int pollutant_idx;
    std::string log_filename;
    int nsteps;
    bool evaluate_cast;
    //SmartPtr <TNLP> mynlp;
    SmartPtr <EPA_NLP> mynlp;
    SmartPtr <IpoptApplication> app;

    //EPA_NLP *mynlp;
public:
    EpsConstraint(const std::shared_ptr<Parameters> &var);
    bool constr_eval(double, int);
    bool evaluate(double, int);

    std::vector<std::string> send_files(const std::string& scenario_data, const std::string& uuid, const std::vector<std::string>& uuids, const std::string& path);
};

#endif
