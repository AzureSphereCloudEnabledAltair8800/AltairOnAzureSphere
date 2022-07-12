# sudo dd if=altair_burn_disk_image.bin of=/dev/sda bs=512; sync

import os

disks = ['azsphere_cpm63k.dsk', 'bdsc-v1.60.dsk', 'blank.dsk', 'blank.dsk']

disk_number = 0
disk_offset = 3000
sector_number = 0

altair_image = open('altair_burn_disk_image.bin', 'wb')


def write_altair_image(data):
    seek = ((disk_offset * disk_number) + sector_number) * 512
    altair_image.seek(seek)
    altair_image.write(data)


for disk in disks:
    # filename = "../AltairHL_emulator/Disks/" + disk
    filename = disk

    with open(filename, 'rb') as altair_disk:

        while True:
            sector = altair_disk.read(137)
            if not sector:
                break

            write_altair_image(sector)
            sector_number = sector_number + 1

    disk_number = disk_number + 1
    sector_number = 0


# Clear wifi config sector
disk_number = 5
sector_number = 0
sector = bytearray(137)
write_altair_image(sector)

altair_image.close()
