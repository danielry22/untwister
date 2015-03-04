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

#include "Untwister.h"


Untwister::Untwister(std::string prng)
{
    if (isSupportedPRNG(prng))
    {
        m_isCompleted = new std::atomic<bool>(false);
        m_isRunning = new std::atomic<bool>(false);
        m_isStarting = new std::atomic<bool>(false);
        m_observedOutputs = new std::vector<uint32_t>;
        m_depth = DEFAULT_DEPTH;
        m_minConfidence = DEFAULT_MIN_CONFIDENCE;
        m_threads = std::thread::hardware_concurrency();
        m_prng = prng;
        m_answers = new std::vector<std::vector<Seed>* >(m_threads);
        m_status = new std::vector<uint32_t>(m_threads);
    }
}

Untwister::Untwister(std::string prng, unsigned int observationSize)
{
    if (isSupportedPRNG(prng))
    {
        m_isCompleted = new std::atomic<bool>(false);
        m_isRunning = new std::atomic<bool>(false);
        m_isStarting = new std::atomic<bool>(false);
        m_observedOutputs = new std::vector<uint32_t>(observationSize);
        m_depth = DEFAULT_DEPTH;
        m_minConfidence = DEFAULT_MIN_CONFIDENCE;
        m_threads = std::thread::hardware_concurrency();
        m_prng = prng;
        m_answers = new std::vector<std::vector<Seed>* >(m_threads);
        m_status = new std::vector<uint32_t>(m_threads);
    }
}

Untwister::~Untwister()
{
    delete m_isCompleted;
    delete m_isRunning;
    delete m_observedOutputs;
    delete m_answers;
    delete m_status;
}

/*
 isStarting: The call to bruteforce() has occured but all workers threads have not started yet
  isRunning: All threads have started and it is safe to call getStatus() externally
isCompleted: The operation has completed and all worker threads have joined
*/
std::vector<Seed> Untwister::bruteforce(uint32_t lowerBoundSeed, uint32_t upperBoundSeed)
{
    if (m_isRunning->load(std::memory_order_relaxed) || m_isStarting->exchange(true))
    {
        throw std::runtime_error("Bruteforce is already in progress");
    }

    if (m_isCompleted->load(std::memory_order_relaxed))
    {
        m_isCompleted->store(false, std::memory_order_relaxed);
    }

    std::vector<uint32_t> labor = m_divisionOfLabor(upperBoundSeed - lowerBoundSeed);
    uint32_t startAt = lowerBoundSeed;
    std::vector<std::thread> pool = std::vector<std::thread>(m_threads);

    for (unsigned int id = 0; id < m_threads; ++id)
    {
        uint32_t endAt = startAt + labor.at(id);
        pool[id] = std::thread(&Untwister::m_worker, this, id, startAt, endAt);
        startAt += labor.at(id);
    }

    m_isRunning->store(true, std::memory_order_relaxed);
    m_isStarting->store(false, std::memory_order_relaxed);

    for (unsigned int id = 0; id < pool.size(); ++id)
    {
        pool[id].join();
    }

    if (!m_isCompleted->load(std::memory_order_relaxed))
    {
        m_isCompleted->store(true, std::memory_order_relaxed);
    }

    std::vector<Seed> results = std::vector<Seed>();
    for (unsigned int id = 0; id < m_answers->size(); ++id)
    {
        for (unsigned int index = 0; index < m_answers->at(id)->size(); ++index)
        {
            results.push_back(m_answers->at(id)->at(index));
        }
        delete m_answers->at(id);
    }
    m_isRunning->store(false, std::memory_order_relaxed);
    return results;
}

/* This method is execute as a seperate thread */
void Untwister::m_worker(unsigned int id, uint32_t startingSeed, uint32_t endingSeed)
{

    /* Each thread must have a local factory unless you like mutexes and/or segfaults */
    PRNGFactory factory;
    PRNG *generator = factory.getInstance(m_prng);
    m_answers->at(id) = new std::vector<Seed>();
    m_status->at(id) = 0;

    for(uint32_t seedIndex = startingSeed; seedIndex <= endingSeed; ++seedIndex)
    {

        if(m_isCompleted->load(std::memory_order_relaxed))
        {
            break;  // Some other thread found the seed
        }

        generator->seed(seedIndex);
        uint32_t matchesFound = 0;
        for(uint32_t index = 0; index < m_depth; index++)
        {
            uint32_t nextRand = generator->random();
            uint32_t observed = m_observedOutputs->at(matchesFound);

            if(observed == nextRand)
            {
                matchesFound++;
                if(matchesFound == m_observedOutputs->size())
                {
                    break;  // This seed is a winner if we get to the end
                }
            }
        }

        m_status->at(id) = seedIndex - startingSeed;
        double confidence = ((double) matchesFound / (double) m_observedOutputs->size()) * 100.0;
        if(m_minConfidence <= confidence)
        {
            Seed seed = Seed(seedIndex, confidence);
            m_answers->at(id)->push_back(seed);
        }

        if(matchesFound == m_observedOutputs->size())
        {
            m_isCompleted->store(true, std::memory_order_relaxed);
        }
    }
    delete generator;
}

bool Untwister::canInferState()
{
    return getStateSize() <= m_observedOutputs->size() ? true:false;
}

/*
    This is the "smarter" method of breaking RNGs. We use consecutive integers
    to infer information about the internal state of the RNG. Using this
    method, however, we won't typically recover an actual seed value.
    But the effect is the same.
*/
State Untwister::inferState()
{
    if (!canInferState())
    {
        throw std::runtime_error("Invalid state size, cannot infer state");
    }

    PRNGFactory factory;
    PRNG *generator = factory.getInstance(m_prng);
    uint32_t stateSize = generator->getStateSize();

    double highscore = 0.0;

    /* Guaranteed from the above to loop at least one time */
    std::vector<double> scores;
    std::vector<uint32_t> best_state;

    for(uint32_t index = 0; index < (m_observedOutputs->size() - stateSize); ++index)
    {
        std::vector<uint32_t>::const_iterator first = m_observedOutputs->begin() + index;
        std::vector<uint32_t>::const_iterator last = m_observedOutputs->begin() + index + stateSize;
        std::vector<uint32_t> state(first, last);

        /* Make predictions based on the state */
        std::vector<uint32_t> evidenceForward
            ((std::vector<uint32_t>::const_iterator)m_observedOutputs->begin(), first);
        std::vector<uint32_t> evidenceBackward
            (last+1, (std::vector<uint32_t>::const_iterator)m_observedOutputs->end());
        generator->setState(state);

        /* Provide additional evidence for tuning on PRNGs that require it */
        generator->setEvidence((*m_observedOutputs));
        generator->tune(evidenceForward, evidenceBackward);

        std::vector<uint32_t> predictions_forward =
            generator->predictForward(((m_observedOutputs->size() - stateSize) - index));

        std::vector<uint32_t> predictions_backward = generator->predictBackward(index);

        /* Test the prediction against the rest of the observed data */
        /* Forward */
        uint32_t matchesFound = 0;
        uint32_t index_pred = 0;
        uint32_t index_obs = index + stateSize;
        while(index_obs < m_observedOutputs->size() && index_pred < predictions_forward.size())
        {
            if(m_observedOutputs->at(index_obs) == predictions_forward[index_pred])
            {
                matchesFound++;
                index_obs++;
            }
            index_pred++;
        }

        /* Backward */
        index_pred = 0;
        index_obs = index;
        while(index_obs > 0 && index_pred < predictions_backward.size())
        {
            if(m_observedOutputs->at(index_obs) == predictions_backward[index_pred])
            {
                matchesFound++;
                index_obs--;
            }
            index_pred++;
        }

        /* If we get a perfect guess, then try reversing out the seed, and exit */
        if(matchesFound == (m_observedOutputs->size() - stateSize))
        {
            uint32_t outSeed = 0;
            if(generator->reverseToSeed(&outSeed, 10000))
            {
                /* We win! */
                // std::cout << SUCCESS << "Found seed " << outSeed << std::endl;
            }
            else
            {
                std::vector<uint32_t> state = generator->getState();
            }
        }

        double score = (double)(matchesFound*100) / (double)(m_observedOutputs->size() - stateSize);
        scores.push_back(score);
        if(score > highscore)
        {
            best_state = generator->getState();
        }
    }

    /* TODO: Analyze scores */
    State finalState;
    finalState.first = best_state;
    finalState.second = highscore;

    delete generator;
    return finalState;
}

std::vector<uint32_t> Untwister::generateSampleFromSeed(uint32_t seed)
{
    PRNGFactory factory;
    PRNG *generator = factory.getInstance(m_prng);
    generator->seed(seed);
    PRNG *distance_gen = factory.getInstance(m_prng);
    distance_gen->seed(time(NULL));
    uint32_t distance = distance_gen->random() % (m_depth - 10);

    // Burn a bunch of random numbers
    for (uint32_t index = 0; index < distance; ++index)
    {
        generator->random();
    }

    std::vector<uint32_t> results(10);
    for (unsigned int index = 0; index < 10; ++index)
    {
        results.push_back(generator->random());
    }
    delete generator;
    delete distance_gen;
    return results;
}

std::vector<uint32_t> Untwister::generateSampleFromState()
{
    PRNGFactory factory;
    PRNG *generator = factory.getInstance(m_prng);
    generator->setState((*m_observedOutputs));

    std::vector<uint32_t> results(m_depth);
    for (unsigned int index = 0; index < m_depth; ++index)
    {
        results.push_back(generator->random());
    }
    delete generator;
    return results;
}

/* Divide X work among Y number of threads, and evenly distribute remainders */
std::vector<uint32_t> Untwister::m_divisionOfLabor(uint32_t sizeOfWork)
{
    uint32_t work = sizeOfWork / m_threads;
    uint32_t leftover = sizeOfWork % m_threads;
    std::vector<uint32_t> labor(m_threads);
    for(uint32_t index = 0; index < m_threads; ++index)
    {
        if (0 < leftover)
        {
            labor[index] = work + 1;
            --leftover;
        }
        else
        {
            labor[index] = work;
        }
    }
    return labor;
}

void Untwister::addObservedOutput(uint32_t observedOutput)
{
    m_observedOutputs->push_back(observedOutput);
}

std::vector<uint32_t>* Untwister::getObservedOutputs()
{
    return m_observedOutputs;
}

std::vector<uint32_t>* Untwister::getStatus()
{
    if (m_isRunning->load(std::memory_order_relaxed))
    {
        return m_status;
    }
    throw std::runtime_error("Bruteforce is not running, no status");
}

static std::vector<std::string>* Untwister::getSupportedPRNGs()
{
    PRNGFactory factory;
    return factory.getNames();
}


static void Untwister::setPRNG(std::string prng)
{
    if (isSupportedPRNG(prng))
    {
        this->m_prng = prng;
    }
    else
    {
        throw std::runtime_error("Unsupported PRNG");
    }
}

static void Untwister::setPRNG(char *prng)
{
    setPRNG(std::string(prng));
}

std::string Untwister::getPRNG()
{
    return m_prng;
}

bool Untwister::isSupportedPRNG(char *prng)
{
    return isSupportedPRNG(std::string(prng));
}

bool Untwister::isSupportedPRNG(std::string prng)
{
    std::vector<std::string> names = getSupportedPRNGs();
    return std::find(names.begin(), names.end(), prng) != names.end() ? true:false;
}

uint32_t Untwister::getStateSize()
{
    PRNGFactory factory;
    PRNG *generator = factory.getInstance(m_prng);
    uint32_t stateSize = generator->getStateSize();
    delete generator;
    return stateSize;
}

void Untwister::setThreads(unsigned int threads)
{
    if (!m_isRunning->load(std::memory_order_relaxed))
    {
        this->m_threads = threads;
        delete m_answers;
        delete m_status;
        m_answers = new std::vector<std::vector<Seed>* >(threads);
        m_status = new std::vector<uint32_t>(threads);
    }
}

static unsigned int Untwister::getThreads()
{
    return m_threads;
}

void Untwister::setDepth(unsigned int depth)
{
    this->m_depth = depth;
}

unsigned int Untwister::getDepth()
{
    return m_depth;
}

void Untwister::setMinConfidence(double minConfidence)
{
    this->m_minConfidence = minConfidence;
}

double Untwister::getMinConfidence()
{
    return m_minConfidence;
}

std::atomic<bool>* Untwister::getIsCompleted()
{
    return m_isCompleted;
}

std::atomic<bool>* Untwister::getIsRunning()
{
    return m_isRunning;
}

const std::string Untwister::getVersion()
{
    return VERSION;
}
