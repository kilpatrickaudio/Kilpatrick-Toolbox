/*
 * Kilpatrick Aduio JSON Helper
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include "../plugin.hpp"

// looks up an array and copies len items to dest
// if the array is not found or is wrong len, returns -1
int jsonHelperLoadBuf(json_t *j, const char *key, uint8_t *dest, int len);

// save data from an array into a json array object and add it to j
// returns -1 on error
int jsonHelperSaveBuf(json_t *j, const char *key, uint8_t *src, int len);

// looks up an int value
// if the value is not found, returns -1
int jsonHelperLoadInt(json_t *j, const char *key, int *val);

// saves an inv value into json array object and add it to j
// returns -1 on error
int jsonHelperSaveInt(json_t *j, const char *key, int val);

#endif
