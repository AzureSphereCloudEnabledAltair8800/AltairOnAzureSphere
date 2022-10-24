/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "altair_config.h"
#include "environment_types.h"

// /*************************************************************
// * Description
// *    Get the current weather
// * Parameter
// *    location_info * - contains country code, city code
// *                      can determine degC/F from country code.
// * Return
// *    char *: current weather description
// *************************************************************/
// TODO: allow for location customization.
void update_owm(ENVIRONMENT_TELEMETRY *environment);
void init_open_weather_map_api_key(ALTAIR_CONFIG_T *altair_config, ENVIRONMENT_TELEMETRY *environment);