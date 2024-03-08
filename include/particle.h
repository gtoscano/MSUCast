#ifndef PARTICLE_H
#define PARTICLE_H

#include <iostream>
#include <vector>
#include "scenario.h"

class Particle {
public:
    Particle(int dim, int nobjs, double w, double c1, double c2, double lb, double ub);
    Particle() = default;
    Particle(const Particle &p);
    ~Particle() = default;
    Particle& operator=(const Particle &p);
    void init();
    void init(const std::vector<double> &xp );
    void update(const std::vector<double> &gbest_x);
    void evaluate();
    const std::vector<double>& get_x() const { return x; }
    const std::vector<double>& get_pbest() const { return pbest_x; }
    const std::vector<double>& get_fx() const { return fx; }
    void set_fx(double fx1, double fx2); 
    void set_uuid(const std::string& uuid) { uuid_ = uuid; }
    const std::string& get_uuid() const { return uuid_; }
    void init_pbest();
    void update_pbest();
    const std::vector<std::tuple<int, int, int, int, double>> get_lc_x() const { return lc_x_; }
    const std::vector<std::tuple<int, int, int, int, int, double>> get_animal_x() const { return animal_x_; }
    const std::vector<std::tuple<int, int, int, int, int, double>> get_manure_x() const { return manure_x_; }
    void set_lc_x(const std::vector<std::tuple<int, int, int, int, double>>& lc_x) { lc_x_ = lc_x; }
    void set_animal_x(const std::vector<std::tuple<int, int, int, int, int, double>>& animal_x) { animal_x_ = animal_x; }
    void set_manure_x(const std::vector<std::tuple<int, int, int, int, int, double>>& manure_x) { manure_x_ = manure_x; }
    void set_lc_cost(double lc_cost) { lc_cost_ = lc_cost; }
    const double get_lc_cost() const { return lc_cost_; }
    void set_animal_cost(double animal_cost) { animal_cost_ = animal_cost; }
    void set_manure_cost(double manure_cost) { manure_cost_ = manure_cost; }
    const double get_animal_cost() const { return animal_cost_; }
    const double get_manure_cost() const { return manure_cost_; }
    void set_amount_plus(const std::unordered_map<std::string, double>& amount_plus) { amount_plus_ = amount_plus; }
    const std::unordered_map<std::string, double>& get_amount_plus() const { return amount_plus_; }
    void set_amount_minus(const std::unordered_map<std::string, double>& amount_minus) { amount_minus_ = amount_minus; }
    const std::unordered_map<std::string, double>& get_amount_minus() const { return amount_minus_; }
    void store_amount_plus_minus(const std::string& filename);

private:
    int dim;
    int nobjs;
    std::vector<double> x;
    std::vector<double> fx;
    std::vector<double> v;
    std::string uuid_;
    double w, c1, c2;
    std::vector<double> pbest_x;
    std::vector<double> pbest_fx;
    double lower_bound;
    double upper_bound;
    std::vector<std::tuple<int, int, int, int, double>> lc_x_;
    std::vector<std::tuple<int, int, int, int, int, double>> animal_x_;
    std::vector<std::tuple<int, int, int, int, int, double>> manure_x_;

    std::unordered_map<std::string, double> amount_minus_;
    std::unordered_map<std::string, double> amount_plus_;
    double lc_cost_;
    double animal_cost_;
    double manure_cost_;
};
#endif
