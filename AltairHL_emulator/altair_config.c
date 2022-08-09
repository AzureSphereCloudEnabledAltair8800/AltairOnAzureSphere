/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "altair_config.h"

void parse_altair_cmd_line_arguments(int argc, char *argv[], ALTAIR_CONFIG_T *altair_config)
{
    int option                                  = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "ScopeID", .has_arg = required_argument, .flag = NULL, .val = 's'},
        {.name = "OwmKey", .has_arg = required_argument, .flag = NULL, .val = 'o'},
        {.name = "CopyUrl", .has_arg = required_argument, .flag = NULL, .val = 'u'},
        {.name = "NetworkInterface", .has_arg = required_argument, .flag = NULL, .val = 'n'},
    };

    altair_config->user_config.connectionType = DX_CONNECTION_TYPE_NOT_DEFINED;

    // Loop over all of the options.
    while ((option = getopt_long(argc, argv, "s:o:u:n:", cmdLineOptions, NULL)) != -1)
    {
        // Check if arguments are missing. Every option requires an argument.
        if (optarg != NULL && optarg[0] == '-')
        {
            Log_Debug("WARNING: Option %c requires an argument\n", option);
            continue;
        }
        switch (option)
        {
            case 's':
                altair_config->user_config.idScope        = optarg;
                altair_config->user_config.connectionType = DX_CONNECTION_TYPE_DPS;
                break;
            case 'o':
                altair_config->open_weather_map_api_key = optarg;
                break;
            case 'n':
                altair_config->network_interface = optarg;
                break;
            default:
                // Unknown options are ignored.
                break;
        }
    }
}