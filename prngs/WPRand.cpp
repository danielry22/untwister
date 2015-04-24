/*
 * WPRand.cpp
 *
 *  Created on: April 24, 2015
 *      Author: moloch
 */

#include "WPRand.h"

WPRand::WPRand()
{
    PHP_mt19937 *m_php_mt = new PHP_mt19937();
}

WPRand::~WPRand()
{

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
        std::string uid = m_phpUniqid();
        std::string step1 = MD5(uid + m_seedValue).rawdigest();
        std::string step2 = SHA1().rawdigest(step1);
        m_rndValue = SHA1().rawdigest();
    }

    // Take the first 8 digits for our value
    std::string sample = m_rndValue.substr(0, 8);
    m_rndValue = m_rndValue.substr(8, m_rndValue.length());
    uint32_t value = atoi(sample.c_str());
    return std::abs(value);
}

/*
    This is a C++ Implementation of PHP's uniqid() - https://php.net/manual/en/function.uniqid.php

    $m = microtime(true); // time as a float
    sprintf("%8x%05x\n", floor($m), ($m-floor($m)) * 1000000);
 */
std::string m_phpUniqid()
{
    float mtime = microtime();
    char buf[14];
    sprintf(buf, "%8x%05x", std::floor(mtime), (mtime - std::floor(mtime)) * 1000000);
    return std::string(buf);
}

uint32_t getStateSize(void)
{
    return WPRAND_STATE_SIZE;
}

void setState(std::vector<uint32_t>)
{

}

std::vector<uint32_t> getState(void)
{

}

