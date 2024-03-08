#ifndef EXTERNAL_ARCHIVE_H
#define EXTERNAL_ARCHIVE_H
#include <vector>
#include <string>
#include "particle.h"


bool is_dominated(const std::vector<double>& a, const std::vector<double>& b);

bool is_non_dominated(const std::vector<double>& a, const std::vector<double>& b);

void update_non_dominated_solutions(
    std::vector<std::vector<double>>& solutions_x, 
    const std::vector<double>& new_solution_x, 
    std::vector<std::vector<double>>& solutions_fx, 
    const std::vector<double>& new_solution_fx);
void update_non_dominated_solutions(
    std::vector<Particle>& archive, 
    const Particle& new_solution_x);

#endif
