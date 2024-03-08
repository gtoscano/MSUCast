
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "external_archive.h"

// ... (insert your function implementations here)
void print_vector(const std::vector<double> &v) {
    for (const auto& val : v) {
        std::cout << val << " ";
    }
}

int main() {
    // Initialize with some non-dominated solutions
    auto schafer_f1 = [](const std::vector<std::vector<double>>& solutions_x) {
        std::vector<std::vector<double>> solutions_fx(solutions_x.size());
        for (size_t i = 0; i < solutions_x.size(); ++i) {
            double sum1 = 0.0, sum2 = 0.0;
            for (const double& x : solutions_x[i]) {
                sum1 += x * x;
                sum2 += (x - 2.0) * (x - 2.0);
            }
            solutions_fx[i] = {sum1, sum2};
        }
        return solutions_fx;
    };

    std::vector<std::vector<double>> solutions_x;
    std::vector<std::vector<double>> solutions_fx;

    std::vector<std::vector<double>> new_solutions_x = {
        {1.0, 4.0},
        {2.0, 3.0},
        {4.0, 1.0},
        {1.0, 5.0},
        {0.0, 3.0},
        {5.0, 0.0},
        {3.0, 2.0},
        {2.9, 2.2},
        {0.0, 0.0}
    };
    std::cout<<"Is dominated: (false) "<< is_dominated({1.1, 4.1}, {1.1, 5.1}) <<"\n";
    std::cout<<"Is dominated: (false)"<< is_dominated({1.0, 4.0}, {2.0, 4.0}) <<"\n";
    std::cout<<"Is dominated: (false)"<< is_dominated({1.0, 4.0}, {1.0, 4.0}) <<"\n";
    std::cout<<"Is dominated: (false)"<< is_dominated({1.0, 4.0}, {0.0, 5.0}) <<"\n";
    std::cout<<"Is dominated: (false)"<< is_dominated({1.0, 4.0}, {2.0, 5.0}) <<"\n";
    std::cout<<"Is dominated: (true) "<< is_dominated({1.0, 5.0}, {1.0, 4.0}) <<"\n";
    std::cout<<"Is dominated: (true) "<< is_dominated({1.0, 5.0}, {0.0, 5.0}) <<"\n";
    std::cout<<"Is dominated: (true) "<< is_dominated({1.0, 5.0}, {0.0, 3.0}) <<"\n";

    /*
     * fx
    17 5 -> dominated by 13 1 
    13 1 -> new no dominated
    17 5 -> dominated by 13 1 
    26 10 -> dominated by 13 1
    9 5  -> new no dominated
    25 13  -> dominated by 13 1
    13 1 -> equals to 13 1
    12.5 6.25 -> new no dominated
*/

    auto new_solutions_fx = new_solutions_x; 
    
    // Print the initial solutions
    std::cout << "Initial solutions to add:\n";
    for(const auto& solution_x : new_solutions_x) {
        print_vector(solution_x);
        std::cout << "\n";
    }
      

    // Print the initial solutions' objectives
    std::cout << "Initial solutions' objectives:\n";
    for(const auto& solution_fx : new_solutions_fx) {
        print_vector(solution_fx);
        std::cout << "\n";
    }
    
    // Update non-dominated solutions
    for (size_t i = 0; i < new_solutions_x.size(); ++i) {
        const auto& new_solution_x = new_solutions_x[i];
        const auto& new_solution_fx = new_solutions_fx[i];

        std::cout << "New solution to consider (x): ";
        print_vector(new_solution_x);
        std::cout << " fX: ";
        print_vector(new_solution_fx);
        std::cout << "\n";

        update_non_dominated_solutions(solutions_x, new_solution_x, solutions_fx, new_solution_fx);

        std::cout << "New non-dominated solutions are:\n";
        std::cout<<solutions_x.size()<<std::endl;
        for (size_t j = 0; j < solutions_x.size(); ++j) {
            std::cout << "x: ";
            print_vector(solutions_x[j]);
            std::cout << " fx: ";
            print_vector(solutions_fx[j]);
            std::cout << "\n";
        }
    }

    // Print the final non-dominated solutions
    std::cout << "Final non-dominated solutions are:\n";
    for (const auto& solution : solutions_x) {
        print_vector(solution);
    }

    std::cout<<"\n";
    std::cout << "Final non-dominated solutions' objectives are:\n";
    for (const auto& solution_fx : solutions_fx) {
        print_vector(solution_fx);
    }

    return 0;
}

