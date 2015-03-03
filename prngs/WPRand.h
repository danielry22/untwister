/*
 * WPRand.h
 *
 *  Created on: Mar 3, 2015
 *      Author: moloch
 */

#ifndef WPRAND_H_
#define WPRAND_H_

#include <random>
#include "PRNG.h"
#include "crypto/md5.h"
#include "crypto/sha1.h"

static const std::string WPRAND = "WPRand";
static const uint32_t WPRAND_STATE_SIZE = 624;

class WPRand: public PRNG
{
public:
    WPRand();
    virtual ~WPRand();

    const std::string getName(void);
    void seed(uint32_t value);
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

private:
    uint32_t seedValue;
};

#endif /* WPRAND_H_ */
