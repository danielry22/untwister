/*
 * Python.h
 *
 *  Created on: Feb 24, 2015
 *      Author: moloch
 *
 */

#ifndef PYTHON_H_
#define PYTHON_H_

#include <vector>

class Python: public PRNG
{
public:

    const std::string getName(void);
    void seed(uint32_t);
    uint32_t getSeed(void);
    uint32_t random(void);
    uint32_t getStateSize(void);
    void setState(std::vector<uint32_t>);
    std::vector<uint32_t> getState(void);
    void setEvidence(std::vector<uint32_t>);
    std::vector<uint32_t> predictForward(uint32_t);
    std::vector<uint32_t> predictBackward(uint32_t);
    void tune(std::vector<uint32_t>, std::vector<uint32_t>);
    bool reverseToSeed(uint32_t *, uint32_t);

    virtual ~Python(){};

protected:
    std::vector<uint32_t> m_state;
    std::vector<uint32_t> m_evidence;

};

#endif /* PYTHON_H_ */
