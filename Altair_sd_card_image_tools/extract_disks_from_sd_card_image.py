# First extract SD card to a altair_extracted_sd_card_image.bin' image
# with sudo dd of=altair_extracted_sd_card_image.bin if=/dev/sda count=12000 bs=512; sync
# change image ownership sudo chown $USER altair_extracted_sd_card_image.bin


import os

disks = ['azsphere_cpm63k.dsk', 'bdsc-v1.60.dsk', 'escape.dsk', 'blank.dsk']

disk_number = 0
disk_offset = 3000


for disk in disks:
    filename = disk

    with open('altair_extracted_sd_card_image.bin', 'rb') as altair_image:

        with open(filename, "wb") as disk_image:

            for sector_number in range(0, 77*32):
                seek = ((disk_offset * disk_number) + sector_number) * 512
                altair_image.seek(seek)
                sector_data = altair_image.read(137)
                if not sector_data:
                    break
                print(sector_number)
                disk_image.write(sector_data)

    disk_number = disk_number + 1
