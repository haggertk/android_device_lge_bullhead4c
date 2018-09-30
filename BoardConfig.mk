#
# Copyright (C) 2018 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# inherit from bullhead
include device/lge/bullhead/BoardConfig.mk

# Assert
TARGET_OTA_ASSERT_DEVICE := bullhead,bullhead4c

# Kernel (only use the Cortex-A53 cores at boot)
BOARD_KERNEL_CMDLINE := $(subst boot_cpus=0-5,boot_cpus=0-3,$(BOARD_KERNEL_CMDLINE))
BOARD_KERNEL_CMDLINE += maxcpus=4

# Power
TARGET_POWERHAL_VARIANT := bullhead4c

# Sensors
NANOHUB_SENSORHAL_NAME_OVERRIDE := sensors.bullhead
