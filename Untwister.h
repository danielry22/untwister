/*
    Copyright Dan Petro, 2014

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UNTWISTER_H_
#define UNTWISTER_H_

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <climits>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include "ConsoleColors.h"
#include "prngs/PRNGFactory.h"
#include "prngs/PRNG.h"

// Pair of <seed, quality of fit>
typedef std::pair<uint32_t, double> Seed;
typedef std::pair<std::vector<uint32_t>, double> State;

static const std::string VERSION = "0.2.0";
static const uint32_t DEFAULT_DEPTH = 1000;
static const double DEFAULT_MIN_CONFIDENCE = 100.0;

class Untwister
{

public:
    Untwister();
    Untwister(unsigned int observationSize);
    virtual ~Untwister();

    std::vector<Seed> bruteforce(uint32_t lowerBoundSeed, uint32_t upperBoundSeed);

    bool canInferState();
    State inferState();
    uint32_t getStateSize();

    static std::vector<std::string> getSupportedPRNGs();
    void setPRNG(std::string prng);
    void setPRNG(char *prng);
    std::string getPRNG();
    static bool isSupportedPRNG(std::string prng);
    static bool isSupportedPRNG(char* prng);

    void setMinConfidence(double minConfidence);
    double getMinConfidence();
    void setDepth(uint32_t depth);
    uint32_t getDepth();
    void setThreads(unsigned int threads);
    static unsigned int getThreads();
    void addObservedOutput(uint32_t observedOutput);
    std::vector<uint32_t>* getObservedOutputs();
    std::vector<uint32_t>* getStatus();
    std::atomic<bool>* getIsCompleted();
    std::atomic<bool>* getIsRunning();

    std::vector<uint32_t> generateSampleFromSeed(uint32_t seed);
    std::vector<uint32_t> generateSampleFromState();

    const std::string Untwister::getVersion();

private:
    unsigned int m_threads;
    double m_minConfidence;
    uint32_t m_depth;
    std::string m_prng;
    std::atomic<bool> *m_isStarting;
    std::atomic<bool> *m_isRunning;
    std::atomic<bool> *m_isCompleted;
    std::vector<uint32_t> *m_status;
    std::vector<std::vector<Seed>* > *m_answers;
    std::vector<uint32_t> *m_observedOutputs;

    void m_worker(unsigned int id, uint32_t startingSeed, uint32_t endingSeed);
    std::vector<uint32_t> m_divisionOfLabor(uint32_t sizeOfWork);

};

#endif /* UNTWISTER_H_ */
