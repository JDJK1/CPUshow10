# Magiceye_driver
Magic Eye EM84 device driver for Linux
(hardware: Elektuur 'Magic Eye' (Jan 2010))
(tested with RaspberryPi (raspbian-jessie-lite 4.9) and Debian7)

Creates device driver 'magiceye_driver' with interfaces 'pwm1' and 'start'

Raspberry Pi build instructions:
 sudo apt-get update
 sudo apt-get -f install
 sudo apt-get install raspberrypi-kernel-headers
 make

Debian: remove driver of IgorPlugUSB receiver:
 rmmod lirc_igorplugusb
 mv -v /lib/modules/$(uname -r)/kernel/drivers/staging/media/lirc/lirc_igorplugusb.ko /root

Installing driver:
 sudo insmod magiceye_driver.ko

Removing driver:
 sudo rmmod magiceye_driver.ko

Verification:
 cd /sys/bus/usb/drivers
 echo 0 | sudo tee magiceye_driver/1*/pwm1
 (cat magiceye_driver/1*/pwm1 shows 0)
 echo 100 | sudo tee magiceye_driver/1*/pwm1
 (cat magiceye_driver/1*/pwm1 shows 100)
 
 echo 10 80 | sudo tee magiceye_driver/1*/start
  (cat magiceye_driver/1*/start shows 10 80)

 echo | sudo tee magiceye_driver/1*/start
  (cat magiceye_driver/1*/start shows 0 100)

Have fun!
JDJK
