# Our's goes first do that our copy-files wins
$(call inherit-product, device/lge/bullhead4c/device.mk)

# Inherit device configuration
$(call inherit-product, device/lge/bullhead/lineage.mk)

## Device identifier. This must come after all inclusions
PRODUCT_NAME := lineage_bullhead4c
PRODUCT_DEVICE := bullhead4c
