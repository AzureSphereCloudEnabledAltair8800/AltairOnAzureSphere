# Burn altair_burn_disk_image.bin image to sd card with dd or balenaEtcher
# sudo dd if=altair_burn_disk_image.bin of=/dev/sda bs=512

import sys
import getopt
import json

disk_number = 5
disk_offset = 3000
sector_number = 0

def write_altair_image(data, altair_image):
    seek = ((disk_offset * disk_number) + sector_number) * 512
    altair_image.seek(seek)
    altair_image.write(data)
    print(data)


def main(argv):
    config = {}
    config.update({'op': "add"})

    try:
        opts, args = getopt.getopt(argv, "s:p:o", ["ssid=", "psk=", "op="])
    except getopt.GetoptError:
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-s", "--ssid"):
            config.update({'ssid': arg})
        elif opt in ("-p", "--psk"):
            config.update({'psk': arg})
        elif opt in ("-o", "--op"):
            config.update({'op': arg})

    if config.get('ssid') is None or config.get('psk') is None or config.get('op') is None:
        print("usage -s YOUR_WIFI_SSID -p YOUR_WIFI_PSK -o OPERATION=add or remove")
        return

    with open('altair_burn_disk_image.bin', 'rb') as altair_image:

        data = altair_image.read()

        with open('altair_burn_disk_image_wifi.bin', 'wb') as altair_image_wifi:

            altair_image_wifi.write(data)

            # Clear wifi config sector
            sector = bytearray(137)
            write_altair_image(sector, altair_image_wifi)

            data = json.dumps(config)

            # Write WiFi config
            sector = bytearray(data, 'utf-8')
            write_altair_image(sector, altair_image_wifi)


if __name__ == "__main__":
    main(sys.argv[1:])
