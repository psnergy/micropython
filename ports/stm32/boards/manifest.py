include("$(MPY_DIR)/extmod/uasyncio/manifest.py")
freeze("$(MPY_DIR)/drivers/dht", "dht.py")
freeze("$(MPY_DIR)/drivers/display", ("lcd160cr.py", "lcd160cr_test.py"))
freeze("$(MPY_DIR)/drivers/onewire", "onewire.py")
freeze("../../../frozen", "main.py")
freeze("../../../frozen", "config.py")
freeze("../../../frozen", "fram.py")
freeze("../../../frozen", "ssc.py")
freeze("../../../frozen", "crc8.py")
freeze("../../../frozen", "nextion.py")
freeze("../../../frozen", "adc.py")
