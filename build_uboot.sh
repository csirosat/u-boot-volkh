#!/bin/bash

set -e

echo "Building m2s-volkh_config..."
make m2s-volkh_config
make clean
make u-boot.bin u-boot.hex
rsync -av u-boot.bin /srv/tftp/u-boot-volkh.bin
rsync -av u-boot.hex /media/sf_firmware/fpga/horus_volkh/envm/u-boot.hex

echo "Building m2s-fg484-som_config..."
make m2s-fg484-som_config
make clean
make u-boot.bin u-boot.hex
rsync -av u-boot.bin /srv/tftp/u-boot-emcraft.bin
rsync -av u-boot.hex /media/sf_firmware/fpga/horus_emcraft/envm/u-boot.hex

