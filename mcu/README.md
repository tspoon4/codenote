# MCU

ESP32 flash incorrect MD5
* Check strapping pins are not connected
* Write flash: sudo python3 esptool.py write_flash_status --non-volatile 0
