#include "wifi_config.h"

// https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-wificonfig/function-wificonfig-forgetallnetworks
// https://docs.microsoft.com/en-us/azure-sphere/network/eap-tls-overview

const char *wifi_config_name      = "AltairWiFi";
const char *wifi_client_cert_name = "WiFiClientCert";
const char *wifi_ca_cert_name     = "WiFiCACert";

enum WIFI_SECURITY
{
	OPEN,
	WPA_PSK,
	WPA_EAP_TLS
};

typedef struct
{
	enum WIFI_SECURITY wifi_security;
	const char *psk;
	const char *ssid;
	const char *security;
	const char *ca_public_cert_pem;
	const char *client_public_cert_pem;
	const char *client_private_key_pem;
	const char *client_private_key_password;
	const char *client_identity;
	int network_id;
} WIFI_CONFIG_T;

// bool is_network_config_stored(uint8_t *wifi_config_name, bool forget);
void execute_wifi_operation(uint8_t *wifi_config);
static void add_wifi_psk(WIFI_CONFIG_T *wifi_config);
static void add_wifi_open(WIFI_CONFIG_T *wifi_config);
static void add_wifi_eap(WIFI_CONFIG_T *wifi);

static enum PANEL_MODE_T previous_panel_mode;
static char display_ip_address[20] = {0};
static char *ip_address_ptr;

DX_TIMER_HANDLER(display_ip_address_handler)
{
#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

	if (*ip_address_ptr)
	{
		gfx_load_character(*ip_address_ptr, retro_click.bitmap);
		gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
		gfx_reverse_panel(retro_click.bitmap);
		gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
		as1115_panel_write(&retro_click);

		ip_address_ptr++;
		dx_timerOneShotSet(&tmr_display_ip_address, &(struct timespec){1, 0});
	}
	else
	{
		panel_mode = previous_panel_mode;
	}
#endif
}
DX_TIMER_HANDLER_END

/// <summary>
/// Check for and load wifi config from disk 5 sectors
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
void wifi_config(void)
{
	// Note this memory is allocated for the Altair memory space but is temporaily repurposed for
	// wifi config as EAP certs can be a max of 8KiB and don't want to allocate that memory on stack or heap
	memset(memory, 0x00, 64 * 1024); // clear memory.
	uint8_t *wifi_json = memory;
	size_t segment_len = 0;

	memset(intercore_disk_block.sector, 0x00, sizeof(intercore_disk_block.sector));
	intercore_disk_block.cached           = false;
	intercore_disk_block.success          = false;
	intercore_disk_block.drive_number     = 5;
	intercore_disk_block.sector_number    = 0;
	intercore_disk_block.disk_ic_msg_type = DISK_IC_READ;

	dx_intercorePublishThenRead(&intercore_sd_card_ctx, &intercore_disk_block, sizeof(intercore_disk_block));

	if (intercore_disk_block.success)
	{
		segment_len = strnlen(intercore_disk_block.sector, sizeof(intercore_disk_block.sector));
		intercore_disk_block.sector_number++;

		// Memory is 64 * 1024. The WiFi config should get nowhere near 60 * 1024 bytes
		while (segment_len == 128 && (wifi_json - memory) < 60 * 1024)
		{
			memcpy(wifi_json, intercore_disk_block.sector, 128);

			wifi_json += 128;
			intercore_disk_block.success = false;

			memset(intercore_disk_block.sector, 0x00, sizeof(intercore_disk_block.sector));

			dx_intercorePublishThenRead(
				&intercore_sd_card_ctx, &intercore_disk_block, sizeof(intercore_disk_block));
			if (!intercore_disk_block.success)
			{
				break;
			}
			segment_len = strnlen(intercore_disk_block.sector, sizeof(intercore_disk_block.sector));
			intercore_disk_block.sector_number++;
		}

		if (segment_len > 0 && segment_len < 128)
		{
			memcpy(wifi_json, intercore_disk_block.sector, segment_len);
			wifi_json += segment_len;
		}

		// Was wifi config data found? If so, it could be a WiFi config JSON string
		if (wifi_json != memory)
		{
			wifi_json = memory;
			execute_wifi_operation(wifi_json);

			// Wipe wifi config info from the SD Card
			for (size_t sector = 0; sector < intercore_disk_block.sector_number; sector++)
			{
				// Now remove the wifi config from disk 5
				memset(intercore_disk_block.sector, 0x00, sizeof(intercore_disk_block.sector));
				intercore_disk_block.success          = false;
				intercore_disk_block.sector_number    = (uint16_t)sector;
				intercore_disk_block.disk_ic_msg_type = DISK_IC_WRITE;

				dx_intercorePublishThenRead(
					&intercore_sd_card_ctx, &intercore_disk_block, sizeof(intercore_disk_block));

				if (intercore_disk_block.success)
				{
					dx_Log_Debug("WiFi Configuration info removed successfully from sector: %d\n", sector);
				}
			}
		}
	}

	memset(&intercore_disk_block, 0x00, sizeof(intercore_disk_block));
}

static bool reset_network(WIFI_CONFIG_T *wifi_configuration)
{
	int result, retry = 0;
	// Wait for the WiFi to be ready

	result = WifiConfig_ForgetAllNetworks();
	while (result == -1 && errno == EAGAIN && retry++ < 10)
	{
		nanosleep(&(struct timespec){0, 250 * ONE_MS}, NULL);
		result = WifiConfig_ForgetAllNetworks();
	}

	if (result == 0)
	{
		if ((wifi_configuration->network_id = WifiConfig_AddNetwork()) != -1)
		{
			result = WifiConfig_SetConfigName(wifi_configuration->network_id, wifi_config_name);
		}
		else
		{
			result = wifi_configuration->network_id;
		}
	}

	return result == 0;
}

void execute_wifi_operation(uint8_t *wifi_config)
{
#define GET_VALUE(key)                                                     \
	if (json_object_has_value_of_type(rootObject, #key, JSONString))       \
	{                                                                      \
		wifi_configuration.key = json_object_get_string(rootObject, #key); \
	}                                                                      \
	else                                                                   \
	{                                                                      \
		wifi_configuration.key = NULL;                                     \
	}

	WIFI_CONFIG_T wifi_configuration;

	memset(&wifi_configuration, 0x00, sizeof(wifi_configuration));
	wifi_configuration.network_id = -1;

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

	GET_VALUE(security);
	GET_VALUE(ssid);
	GET_VALUE(psk);
	GET_VALUE(ca_public_cert_pem);
	GET_VALUE(client_identity);
	GET_VALUE(client_public_cert_pem);
	GET_VALUE(client_private_key_pem);
	GET_VALUE(client_private_key_password);

	if (wifi_configuration.security)
	{
		// note the default is open if key value not found
		if (!strncmp(wifi_configuration.security, "open", strlen(wifi_configuration.security)))
		{
			wifi_configuration.wifi_security = OPEN;
		}
		else if (!strncmp(wifi_configuration.security, "wpa_psk", strlen(wifi_configuration.security)))
		{
			wifi_configuration.wifi_security = WPA_PSK;
		}
		else if (!strncmp(wifi_configuration.security, "wpa_eap_tls", strlen(wifi_configuration.security)))
		{
			wifi_configuration.wifi_security = WPA_EAP_TLS;
		}
	}

	switch (wifi_configuration.wifi_security)
	{
		case OPEN:
			add_wifi_open(&wifi_configuration);
			break;
		case WPA_PSK:
			add_wifi_psk(&wifi_configuration);
			break;
		case WPA_EAP_TLS:
			add_wifi_eap(&wifi_configuration);
			break;
		default:
			break;
	}

cleanup:

	if (rootProperties != NULL)
	{
		json_value_free(rootProperties);
	}
}

static void add_wifi_psk(WIFI_CONFIG_T *wifi)
{

	if (!wifi->ssid || !wifi->psk)
	{
		return;
	}

	if (reset_network(wifi))
	{
		if (WifiConfig_SetSecurityType(wifi->network_id, WifiConfig_Security_Wpa2_Psk) == -1)
		{
			Log_Debug("Set security type %s: errno=%d (%s)\n", "EAP", errno, strerror(errno));
		}
		else if (WifiConfig_SetPSK(wifi->network_id, wifi->psk, 11) == -1)
		{
		}
		else if (WifiConfig_SetSSID(wifi->network_id, wifi->ssid, strlen(wifi->ssid)) == -1)
		{
			Log_Debug("Set SSID %s: errno=%d (%s)\n", wifi->ssid, errno, strerror(errno));
		}
		else if (WifiConfig_SetNetworkEnabled(wifi->network_id, true) == -1)
		{
			Log_Debug("Set Enable network: errno=%d (%s)\n", errno, strerror(errno));
		}
		else if (WifiConfig_PersistConfig() == -1)
		{
			Log_Debug("Persist config: errno=%d (%s)\n", errno, strerror(errno));
		}
		else
		{
			dx_Log_Debug("WiFi Configured\n");
		}
	}
}

static void add_wifi_open(WIFI_CONFIG_T *wifi)
{
	if (!wifi->ssid)
	{
		return;
	}

	if (reset_network(wifi))
	{
		if (WifiConfig_SetSecurityType(wifi->network_id, WifiConfig_Security_Open) == -1)
		{
			Log_Debug("Set security type %s: errno=%d (%s)\n", "EAP", errno, strerror(errno));
		}
		else if (WifiConfig_SetSSID(wifi->network_id, wifi->ssid, strlen(wifi->ssid)) == -1)
		{
			Log_Debug("Set SSID %s: errno=%d (%s)\n", wifi->ssid, errno, strerror(errno));
		}
		else if (WifiConfig_SetNetworkEnabled(wifi->network_id, true) == -1)
		{
			Log_Debug("Set Enable network: errno=%d (%s)\n", errno, strerror(errno));
		}
		else if (WifiConfig_PersistConfig() == -1)
		{
			Log_Debug("Persist config: errno=%d (%s)\n", errno, strerror(errno));
		}
		else
		{
			dx_Log_Debug("WiFi Configured\n");
		}
	}
}

static bool install_certs(WIFI_CONFIG_T *wifi)
{
	if (CertStore_InstallRootCACertificate(
			wifi_ca_cert_name, wifi->ca_public_cert_pem, strlen(wifi->ca_public_cert_pem)) == -1)
	{
		Log_Debug("CA cert install failed. Errno: %d\n", errno);
	}
	else if (CertStore_InstallClientCertificate(wifi_client_cert_name, wifi->client_public_cert_pem,
				 strlen(wifi->client_public_cert_pem), wifi->client_private_key_pem,
				 strlen(wifi->client_private_key_pem), wifi->client_private_key_password) == -1)
	{
		Log_Debug("Client cert install failed. Errno: %d %s\n", errno, strerror(errno));
	}
	else
	{
		return true;
	}

	return false;
}

static void add_wifi_eap(WIFI_CONFIG_T *wifi)
{
	if (!wifi->ssid || !wifi->ca_public_cert_pem || !wifi->client_public_cert_pem)
	{
		return;
	}

	if (!install_certs(wifi))
	{
		Log_Debug("Install certs failed network failed\n");
	}
	else if (!reset_network(wifi))
	{
		Log_Debug("Reset network failed\n");
	}
	else if (WifiConfig_SetSecurityType(wifi->network_id, WifiConfig_Security_Wpa2_EAP_TLS) == -1)
	{
		Log_Debug("Set security type %s: errno=%d (%s)\n", "EAP", errno, strerror(errno));
	}
	else if (WifiConfig_SetClientIdentity(wifi->network_id, wifi->client_identity) == -1)
	{
		Log_Debug("Set client identity %s: errno=%d (%s)\n", wifi->client_identity, errno, strerror(errno));
	}
	else if (WifiConfig_SetRootCACertStoreIdentifier(wifi->network_id, wifi_ca_cert_name) == -1)
	{
		Log_Debug("Set CA cert %s: errno=%d (%s)\n", wifi_config_name, errno, strerror(errno));
	}
	else if (WifiConfig_SetClientCertStoreIdentifier(wifi->network_id, wifi_client_cert_name) == -1)
	{
		Log_Debug("Set Client cert %s: errno=%d (%s)\n", wifi_client_cert_name, errno, strerror(errno));
	}
	else if (WifiConfig_SetSSID(wifi->network_id, wifi->ssid, strlen(wifi->ssid)) == -1)
	{
		Log_Debug("Set SSID %s: errno=%d (%s)\n", wifi->ssid, errno, strerror(errno));
	}
	else if (WifiConfig_SetNetworkEnabled(wifi->network_id, true) == -1)
	{
		Log_Debug("Set Enable network: errno=%d (%s)\n", errno, strerror(errno));
	}
	else if (WifiConfig_PersistConfig() == -1)
	{
		Log_Debug("Persist config: errno=%d (%s)\n", errno, strerror(errno));
	}
	else
	{
		dx_Log_Debug("EAP-TLS WiFi successfully configured\n");
	}
}

void getIP(void)
{
	struct ifaddrs *addr_list;
	struct ifaddrs *it;
	int n;

	strncpy(display_ip_address, "NOT CONNECTED", sizeof(display_ip_address));
	ip_address_ptr = display_ip_address;

	if (getifaddrs(&addr_list) < 0)
	{
		Log_Debug("***** GETIFADDRS failed! ***\n");
	}
	else
	{
		for (it = addr_list, n = 0; it != NULL; it = it->ifa_next, ++n)
		{
			if (it->ifa_addr == NULL)
			{
				continue;
			}
			if (strncmp(it->ifa_name, default_network_interface, strlen(default_network_interface)) == 0)
			{
				if (it->ifa_addr->sa_family == AF_INET)
				{
					struct sockaddr_in *addr = (struct sockaddr_in *)it->ifa_addr;
					char *ip_address         = inet_ntoa(addr->sin_addr);
					strncpy(display_ip_address, ip_address, sizeof(display_ip_address));
				}
			}
		}
	}

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
	previous_panel_mode = panel_mode;
	panel_mode          = PANEL_FONT_MODE;
	dx_timerOneShotSet(&tmr_display_ip_address, &(struct timespec){1, 0});
#else
	Log_Debug("**** wlan0 IP: %s ***\n", display_ip_address);
#endif

	freeifaddrs(addr_list);
}