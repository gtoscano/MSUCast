#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "pso.h"


namespace {
    
    void save(const std::vector<std::vector<double>>& data, const std::string& filename) {
        std::ofstream out_file(filename);
        
        // Check if the file opened successfully
        if (!out_file) {
            std::cerr << "Failed to open the file.\n";
            return;
        }
        
        for (const auto& row : data) {
            for (const auto& val : row) {
                out_file << val << ' ';
            }
            out_file << '\n';
        }
        
        out_file.close();
    }
}

int main (int argc, char *argv[]) {
    
    int ndim = 1;
    int nparts = 10;
    int nobjs = 2;
    int max_iter = 100;
    double c1 = 1.4;
    double c2 = 1.4;
    double w = 0.7;
    double lb(0.0);
    double ub(1.0);
    std::string input_filename = "prueba.json";
    std::string scenario_filename = "my_prueba.json";
    std::string out_dir = "out";
    PSO pso(nparts, nobjs, max_iter, w, c1, c2, lb, ub, input_filename, scenario_filename, out_dir, false, true, true, true);
    pso.optimize();
    std::vector<std::vector<double>> gbest_x = pso.get_gbest_x();
    std::vector<std::vector<double>> gbest_fx = pso.get_gbest_fx();
    save(gbest_x, "gbest_x.txt");
    save(gbest_fx, "gbest_fx.txt");
    //pso.print();
    //std::vector<double> gbest_x = pso.get_gbest_x();

    return 0;
}
