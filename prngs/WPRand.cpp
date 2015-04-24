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

const std::string getName(void) {
    return WPRAND;
}

void seed(uint32_t value) {
    m_seedValue = value;
}

uint32_t getSeed(void) {
    return m_seedValue;
}

uint32_t random(void) {

    if (m_rndValue.length() < MIN_RND_LENGTH) {
        // $rnd_value = md5( uniqid(microtime() . mt_rand(), true ) . $seed );
        std::string uid = m_php_uniqid();
        std::string md = MD5(uid + m_seedValue).rawdigest();
        sha1();
        sha1();
    }
    // Take the first 8 digits for our value
    // $value = substr($rnd_value, 0, 8);
    // intval(value)
    return std::abs(value);

}

uint32_t getStateSize(void) {
    return WPRAND_STATE_SIZE;
}

void setState(std::vector<uint32_t>) {

}

std::vector<uint32_t> getState(void) {

}

