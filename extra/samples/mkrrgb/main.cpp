/*
 *  extra/stduart/main.cpp
 *  Copyright 2021 ItJustWorksTM
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#ifndef SMCE_RESOURCES_DIR
#    error "SMCE_RESOURCES_DIR is not set"
#endif

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <thread>
#include <utility>
#include <SMCE/Board.hpp>
#include <SMCE/BoardConf.hpp>
#include <SMCE/BoardView.hpp>
#include <SMCE/Sketch.hpp>
#include <SMCE/SketchConf.hpp>
#include <SMCE/Toolchain.hpp>

using namespace std::literals;

void print_help(const char* argv0) {
    std::cout << "Usage: " << argv0 << " <fully-qualified-board-name> <path-to-sketch>" << std::endl;
}

int main(int argc, char** argv) {
    if (argc == 2 && (argv[1] == "-h"sv || argv[1] == "--help"sv)) {
        print_help(argv[0]);
        return EXIT_SUCCESS;
    } else if (argc != 3) {
        print_help(argv[0]);
        return EXIT_FAILURE;
    }

    // Create the toolchain
    smce::Toolchain toolchain{SMCE_RESOURCES_DIR};
    if (const auto ec = toolchain.check_suitable_environment()) {
        std::cerr << "Error: " << ec.message() << std::endl;
        return EXIT_FAILURE;
    }

    // Create the sketch, and declare that it requires the WiFi and MQTT Arduino libraries during preprocessing
    // clang-format off
    smce::Sketch sketch{argv[2], {
          .fqbn = argv[1],
          .legacy_preproc_libs = { {"ArduinoLibrary"} }
    }};
    // // clang-format on

    std::cout << "Compiling..." << std::endl;
    // Compile the sketch on the toolchain
    if (const auto ec = toolchain.compile(sketch)) {
        std::cerr << "Error: " << ec.message() << std::endl;
        auto [_, log] = toolchain.build_log();
        if (!log.empty())
            std::cerr << log << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Done" << std::endl;

    smce::Board board; // Create the virtual Arduino board
    board.attach_sketch(sketch);
    // clang-format off
    smce::BoardConfig board_conf{
        .frame_buffers = { {} }
    };

    board.configure(std::move(board_conf));
    // clang-format on

    // Power-on the board
    if (!board.start()) {
        std::cerr << "Error: Board failed to start sketch" << std::endl;
        return EXIT_FAILURE;
    };

    std::this_thread::sleep_for(1000ms);

    auto board_view = board.view();
    auto fbuf = board_view.frame_buffers[0];

    std::byte target[12 * 7 * 3]; // target
    fbuf.read_rgb888(target);

    for (int y = 0; y <= 6; y++) {
        for (int x = 0; x <= 11; x++) {
            if (target[(y * 12 + x) * 3] == (std::byte)0) {
                std::cout << '-';
            } else {
                std::cout << '@';
            }
        }
        std::cout << std::endl;
    }

    board.stop(); // Power-off the board
}