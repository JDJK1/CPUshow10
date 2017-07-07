# CPUshow10
Magic Eye EM84 for Windows10 and Linux
(hardware: Elektuur 'Magic Eye' (Jan 2010))

Original 'AVR309' driver doesn't install under Windows 10, so searched for other driver..
Driver 'libusb-win32' testtool 'testlibusb-win.exe' sees the 'Magic Eye' directly (tested with v1.2.6.0), so the original CPUshow code is modified for this driver.

For Windows10 a VisualStudio 2015 project is present.
(pm: libusb-win32 driver can be installed by using its inf-wizard.exe tool)

For Linux a Makefile is present.
(pm: uses package 'libusb-dev'
 (eg for RaspberryPi (raspbian-jessie-lite 4.9): 'apt-get -f install', 'apt-get install libusb-dev'))

Directory 'Driver' contains a (seperate) Linux device driver.

Have fun!
JDJK
