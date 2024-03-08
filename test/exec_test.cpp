
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <unordered_map>
#include <filesystem>

#include <random>
#include <iomanip>
#include <chrono>
#include <ostream>
#include <stdexcept>
#include <fmt/core.h>

#include <crossguid/guid.hpp>

#include "execute.h"

int main (int argc, char *argv[]) {
    std::string emo_uuid = xg::newGuid().str();
    fmt::print("emo_uuid: {}\n", emo_uuid);
    Execute execute;
    execute.set_files(emo_uuid, "front/0_reportloads.csv");
    execute.execute(emo_uuid, 0.30, 8, 10);
    execute.get_files(emo_uuid, "ipopt");
    return 0;
}
