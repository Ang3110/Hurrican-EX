/**
 * This implements logging to a file and cout.
 *
 * (c) 2002 Jörg M. Winterstein
 * (c) 2017 Stefan Schmidt
 **/

#include "Logdatei.hpp"
#include <iostream>
#include <filesystem>

void Logdatei::OpenFile(const std::string &filename) {
    file.open(std::filesystem::u8path(filename));
    if (!file)
        std::cerr << "Unable to open log file (" << filename << ")!" << std::endl;
}

Logdatei::~Logdatei() {
    flush();
}

/**
 * Puts the internal buffer into the output-streams, flushes them and empties the buffer
 **/
void Logdatei::flush() {
    // print to both outputs
    std::cout << this->str();
    if (file)
        file << this->str() << std::flush;

    // flush both streams to ensure they are up-to-date
    std::cout << std::flush;

    // empty internal buffer
    this->str("");
}
