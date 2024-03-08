#include <vector>
#include <iostream>
#include <random>
#include <algorithm>
#include <crossguid/guid.hpp>
#include <fmt/core.h>
#include "json.hpp"
#include <fstream>

#include "particle.h"
#include "external_archive.h"

using json = nlohmann::json;

namespace {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    double rand_double(double lower_bound, double upper_bound) {
        return lower_bound + dis(gen) * (upper_bound - lower_bound);
    }
}


Particle::Particle(int dim, int nobjs, double w, double c1, double c2, double lb, double ub) {
    //fmt::print("Particle constructor\n");
    fmt::print("{}\n", dim);
    this->dim = dim;
    this->nobjs= nobjs;
    this->w = w;
    this->c1 = c1;
    this->c2 = c2;

    this->x = std::vector<double>(dim);
    this->v = std::vector<double>(dim);
    this->fx = std::vector<double>(nobjs);
    this->pbest_x = std::vector<double>(dim);
    this->pbest_fx = std::vector<double>(nobjs);
    this->lower_bound = lb; 
    this->upper_bound = ub; 
    this->uuid_ = xg::newGuid().str();
    this->lc_cost_ = 0.0; 
    this->animal_cost_ = 0.0;
    this->manure_cost_ = 0.0;
}


Particle::Particle(const Particle &p) {
    //fmt::print("Particle copy constructor\n");
    this->dim = p.dim;
    this->nobjs= p.nobjs;
    this->w = p.w;
    this->c1 = p.c1;
    this->c2 = p.c2;
    this->x = p.x;
    this->v = p.v;
    this->fx = p.fx;
    this->pbest_x = p.pbest_x;
    this->pbest_fx = p.pbest_fx;
    this->lower_bound = p.lower_bound;
    this->upper_bound = p.upper_bound;
    this->uuid_ = p.uuid_;
    this->lc_x_ = p.lc_x_;
    this->animal_x_ = p.animal_x_;
    this->manure_x_ = p.manure_x_;
    this->lc_cost_ = p.lc_cost_; 
    this->animal_cost_ = p.animal_cost_; 
    this->manure_cost_ = p.manure_cost_; 
    this->amount_plus_ = p.amount_plus_;
    this->amount_minus_ = p.amount_minus_;
}

Particle& Particle::operator=(const Particle &p) {
  // Protect against self-assignment
    if (this == &p) {
        return *this;
    }
    this->dim = p.dim;
    this->nobjs= p.nobjs;
    this->w = p.w;
    this->c1 = p.c1;
    this->c2 = p.c2;
    this->x = p.x;
    this->fx = p.fx;
    this->v = p.v;
    this->pbest_x = p.pbest_x;
    this->pbest_fx = p.pbest_fx;
    this->lower_bound = p.lower_bound;
    this->upper_bound = p.upper_bound;
    this->uuid_ = p.uuid_;
    this->lc_x_ = p.lc_x_;
    this->animal_x_ = p.animal_x_;
    this->manure_x_ = p.manure_x_;
    this->lc_cost_ = p.lc_cost_; 
    this->animal_cost_ = p.animal_cost_; 
    this->manure_cost_ = p.manure_cost_; 
    this->amount_plus_ = p.amount_plus_;
    this->amount_minus_ = p.amount_minus_;
    return *this;
}


void Particle::init() {

    for (int i = 0; i < dim; i++) {
        x[i] = rand_double(lower_bound, upper_bound);
        v[i] = 0.0; 
    }

    //evaluate();
    //pbest_x = x;
    //pbest_fx = fx;
}

void Particle::init(const std::vector<double> &xp ) {

    for (int i = 0; i < dim; i++) {
        x[i] = xp[i]; 
        v[i] = 0.0; 
    }

    //evaluate();
    //pbest_x = x;
    //pbest_fx = fx;
}

void Particle::init_pbest() {
    pbest_x = x;
    pbest_fx = fx;
}

void Particle::update(const std::vector<double> &gbest_x) {

    for (int i = 0; i < dim; i++) {
        // update velocity
        double inertia = w * v[i];
        double cognitive = c1 * rand_double(0.0, 1.0) * (pbest_x[i] - x[i]);
        double social = c2 * rand_double(0.0, 1.0) * (gbest_x[i] - x[i]);
        //v[i] = w * v[i] + c1 * rand_double(0, 1) * (pbest_x[i] - x[i]) + c2 * rand_double(0, 1) * (gbest_x[i] - x[i]);
        v[i] = inertia + cognitive + social;

        // update position
        x[i] = x[i] + v[i];
        // check if x is out of bounds
        if (x[i] < lower_bound) {
            x[i] = lower_bound;
        }
        if (x[i] > upper_bound) {
            x[i] = upper_bound;
        }

    }
    //evaluate();
    //update_pbest();
}


void Particle::store_amount_plus_minus(const std::string& filename) {

    json json_obj;// = json::object();
    json_obj["plus"] = amount_plus_; 
    json_obj["minus"] = amount_minus_;

    std::ofstream file(filename);
    file<<json_obj.dump();
    file.close();
}

void Particle::update_pbest() {
    if (is_dominated(pbest_fx, fx) || !is_dominated(fx, pbest_fx)) {
        pbest_x = x;
        pbest_fx = fx;
    }
}


void Particle::evaluate() {
    double fx0 = 0.0;
    double fx1 = 0.0;

    for (int i = 0; i < dim; i++) {
        fx0 += x[i] * x[i] ;
        fx1 += (x[i] -2.0) * (x[i] -2.0);
    }
    fx[0] = fx0; 
    fx[1] = fx1; 
}

void Particle::set_fx(double fx1, double fx2) {
    fx[0] = fx1; 
    fx[1] = fx2;
}

