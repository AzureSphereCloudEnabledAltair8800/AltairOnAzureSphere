# Burn altair_burn_disk_image.bin image to sd card with dd or balenaEtcher
# sudo dd if=altair_burn_disk_image.bin of=/dev/sda bs=512

import profile
import sys
import getopt
import json
import yaml
from os.path import exists

disk_number = 5
disk_offset = 3000


wifi_profile = None
wifi_config = None


def write_altair_image(data, altair_image):
    segment = 0
    start = 0
    length = len(data)
    sector_number = 0
    total_len = 0

    while start < length:
        seg_data = data[start:start + 128]

        print(seg_data)
        total_len += len(seg_data)

        seek = ((disk_offset * disk_number) + sector_number) * 512

        print(seek)

        # Clear wifi config sector
        altair_image.seek(seek)
        altair_image.write(bytearray(137))

        altair_image.seek(seek)
        altair_image.write(bytearray(seg_data, 'utf-8'))

        start += 128
        sector_number += 1

    print(total_len)

    seek = ((disk_offset * disk_number) + sector_number) * 512
    # Write an empty sector after wifi config sectors
    altair_image.seek(seek)
    altair_image.write(bytearray(137))


def main(argv):
    config = {}
    config.update({'op': "add"})

    try:
        opts, args = getopt.getopt(
            argv, "p:", ["profile="])
    except getopt.GetoptError:
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-p", "--profile"):
            wifi_profile = arg

    print(wifi_profile)

    if not exists(wifi_profile):
        print("YAML WiFi profile not found")
        return

    with open(wifi_profile, 'r') as file:
        wifi_config = yaml.safe_load(file)

    if wifi_config.get("security") == "wpa_eap_tls":
        with open(wifi_config.get('ca_cert_pem_filename'), 'r') as file:
            wifi_config.update({'ca_cert_pem': file.read()})
        with open(wifi_config.get('client_cert_pem_filename'), 'r') as file:
            wifi_config.update({'client_cert_pem': file.read()})

    if wifi_config.get('ssid') is None or wifi_config.get('op') is None or wifi_config.get('security') is None:
        print("usage -s YOUR_WIFI_SSID -p YOUR_WIFI_PSK -o OPERATION=add or remove --security=open or wpa_psk or wpa_eap_tls")
        return

    with open('altair_burn_disk_image.bin', 'rb') as altair_image:

        data = altair_image.read()

        with open('altair_burn_disk_image_wifi.bin', 'wb') as altair_image_wifi:

            altair_image_wifi.write(data)

            data = json.dumps(wifi_config)
            print(data)
            print(len(data))

            # Write WiFi config

            write_altair_image(data, altair_image_wifi)


if __name__ == "__main__":
    main(sys.argv[1:])
