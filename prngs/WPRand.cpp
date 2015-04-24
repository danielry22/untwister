/*
 * WPRand.cpp
 *
 *  Created on: April 24, 2015
 *      Author: moloch
 */

#include "WPRand.h"

WPRand::WPRand()
{
    PHP_mt19937 *m_phpMt = new PHP_mt19937();
}

WPRand::~WPRand()
{
    delete m_phpMt;
}

const std::string getName(void)
{
    return WPRAND;
}

void seed(uint32_t value)
{
    m_seedValue = value;
}

uint32_t getSeed(void)
{
    return m_seedValue;
}

uint32_t random(void)
{
    if (m_rndValue.length() < MIN_RND_LENGTH)
    {
        // $rnd_value = md5( uniqid(microtime() . mt_rand(), true ) . $seed );
        std::string uid = m_phpUniqid(microtime() + m_phpMt->random());

        std::string step1 = MD5(uid + m_seedValue).rawdigest();

        std::string step2 = SHA1(step1).rawdigest();

        /* HASH IT AGAIN! */
        m_rndValue = SHA1(step2).rawdigest();
    }

    // Take the first 8 digits for our value
    std::string sample = m_rndValue.substr(0, 8);
    m_rndValue = m_rndValue.substr(8, m_rndValue.length());
    uint32_t value = atoi(sample.c_str());
    return std::abs(value);
}

/*
    This is a C++ Implementation of PHP's uniqid()
    https://php.net/manual/en/function.uniqid.php

    $m = microtime(true); // time as a float
    sprintf("%8x%05x\n", floor($m), ($m - floor($m)) * 1000000);
 */
std::string m_phpUniqid(std::string entropy)
{
    float mtime = (float) gettimeofday();
    char buf[14];
    sprintf(buf, "%8x%05x", std::floor(mtime), (mtime - std::floor(mtime)) * 1000000);
    return std::string(buf);
}

uint32_t getStateSize(void)
{
    return WPRAND_STATE_SIZE;
}

void setState(std::vector<uint32_t> inState)
{
    m_phpMt->setState(inState);
}

std::vector<uint32_t> getState(void)
{
    return m_phpMt->getState();
}

