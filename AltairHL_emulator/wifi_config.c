#include "wifi_config.h"

// https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-wificonfig/function-wificonfig-forgetallnetworks

const char *configName = "Altair_WiFi";

// bool is_network_config_stored(uint8_t *configName, bool forget);
void execute_wifi_operation(uint8_t *wifi_config);

void wifi_config(void)
{
	memset(intercore_disk_block.sector, 0x00, sizeof(intercore_disk_block.sector));
	intercore_disk_block.cached           = false;
	intercore_disk_block.success          = false;
	intercore_disk_block.drive_number     = 5;
	intercore_disk_block.sector_number    = 0;
	intercore_disk_block.disk_ic_msg_type = DISK_IC_READ;

	dx_intercorePublishThenRead(&intercore_sd_card_ctx, &intercore_disk_block, sizeof(intercore_disk_block));

	if (intercore_disk_block.success)
	{
		dx_Log_Debug(intercore_disk_block.sector);

		// Is there a string at sector 0. If so, it could be a WiFi config JSON string
		if (strnlen(intercore_disk_block.sector, sizeof(intercore_disk_block.sector)) > 0)
		{
			execute_wifi_operation(intercore_disk_block.sector);
		}
	}
}

void execute_wifi_operation(uint8_t *wifi_config)
{
	const char *operation = NULL;
	const char *psk       = NULL;
	const char *ssid      = NULL;
	int network_id        = -1;

	JSON_Value *rootProperties = json_parse_string(wifi_config);
	if (rootProperties == NULL)
	{
		goto cleanup;
	}

	JSON_Object *rootObject = json_value_get_object(rootProperties);
	if (rootObject == NULL)
	{
		goto cleanup;
	}

	if (json_object_has_value_of_type(rootObject, "ssid", JSONString))
	{
		ssid = json_object_get_string(rootObject, "ssid");
	}

	if (json_object_has_value_of_type(rootObject, "psk", JSONString))
	{
		psk = json_object_get_string(rootObject, "psk");
	}

	if (json_object_has_value_of_type(rootObject, "op", JSONString))
	{
		operation = json_object_get_string(rootObject, "op");
	}

	if (dx_isStringNullOrEmpty(ssid) || dx_isStringNullOrEmpty(psk) || dx_isStringNullOrEmpty(operation) ||
		strnlen(ssid, WIFICONFIG_SSID_MAX_LENGTH) > WIFICONFIG_SSID_MAX_LENGTH ||
		strnlen(psk, WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE) > WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE)
	{
		goto cleanup;
	}

	if (WifiConfig_ForgetAllNetworks() != -1 && strncmp(operation, "add", 3) == 0)
	{
		if (WifiConfig_ForgetAllNetworks() != -1)
		{
			if ((network_id = WifiConfig_AddNetwork()) != -1)
			{
				if (WifiConfig_SetSecurityType(network_id, WifiConfig_Security_Wpa2_Psk) != -1)
				{
					if (WifiConfig_SetConfigName(network_id, configName) != -1)
					{
						if (WifiConfig_SetPSK(network_id, psk, 11) != -1)
						{
							if (WifiConfig_SetSSID(network_id, ssid, 3) != -1)
							{
								if (WifiConfig_SetNetworkEnabled(network_id, true) != -1)
								{
									if (WifiConfig_PersistConfig() != -1)
									{
										dx_Log_Debug("WiFi Configured\n");

										// Now remove the wifi config from disk 5 sector 0

										memset(intercore_disk_block.sector, 0x00,
											sizeof(intercore_disk_block.sector));
										intercore_disk_block.cached           = false;
										intercore_disk_block.success          = false;
										intercore_disk_block.drive_number     = 5;
										intercore_disk_block.sector_number    = 0;
										intercore_disk_block.disk_ic_msg_type = DISK_IC_WRITE;

										dx_intercorePublishThenRead(&intercore_sd_card_ctx,
											&intercore_disk_block, sizeof(intercore_disk_block));

										if (intercore_disk_block.success)
										{
											dx_Log_Debug("WiFi Configuration info removed sucessfully\n");
										}
									}
								};
							}
						}
					}
				}
			}
		}
	}

cleanup:

	if (rootProperties != NULL)
	{
		json_value_free(rootProperties);
	}
}

// bool is_network_config_stored(uint8_t *configName, bool forget)
// {
// 	int network_id;

// 	if ((network_id = WifiConfig_GetNetworkIdByConfigName(configName)) != -1)
// 	{
// 		if (forget)
// 		{
// 			return WifiConfig_ForgetNetworkById(network_id) != 0;
// 		}
// 		else
// 		{
// 			return true; // network config exists
// 		}
// 	}
// 	return false; // network config doesn't exist
// }