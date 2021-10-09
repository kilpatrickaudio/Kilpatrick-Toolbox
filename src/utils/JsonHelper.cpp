/*
 * Kilpatrick Aduio JSON Helper
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
 #include "../plugin.hpp"
#include "JsonHelper.h"

// looks up an array and copies len items to dest
// if the array is not found or is wrong len, returns -1
int jsonHelperLoadBuf(json_t *j, const char *key, uint8_t *dest, int len) {
    int i;
    json_t *ja = json_object_get(j, key);
    if(ja == NULL || (int)json_array_size(ja) != len) {
        return -1;
    }
    for(i = 0; i < (int)json_array_size(ja); i ++) {
        dest[i] = json_integer_value(json_array_get(ja, i));
    }
    return 0;
}

// save data from an array into a json array object and add it to j
// returns -1 on error
int jsonHelperSaveBuf(json_t *j, const char *key, uint8_t *src, int len) {
    int i;
    json_t *ja = json_array();
    for(i = 0; i < len; i ++) {
        json_array_append_new(ja, json_integer(src[i]));
    }
    json_object_set_new(j, key, ja);
    return 0;
}

// looks up an int value
// if the value is not found, returns -1
int jsonHelperLoadInt(json_t *j, const char *key, int *val) {
    json_t *ja = json_object_get(j, key);
    if(ja == NULL || !json_is_integer(ja)) {
        return -1;
    }
    *val = json_integer_value(ja);
    return 0;
}

// saves an inv value into json array object and add it to j
// returns -1 on error
int jsonHelperSaveInt(json_t *j, const char *key, int val) {
    return json_object_set_new(j, key, json_integer(val));
}
