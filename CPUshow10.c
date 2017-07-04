#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "lusb0_usb.h"

const char* cAvrDevString = "AVR309:USB to UART protocol converter (simple)";
const unsigned short cVendorUSBID = 0x03eb; // Atmel
const unsigned short cDeviceUSBID = 0x21FE; // RS232 converter AT90S2313
const unsigned short cDeviceVersion = 2;    // v0.02
const int cAVRConfig = 1; // Must be 1 for AVR309
const int cAVRIntf = 0;   // Must be 0 for AVR309 (usb_interface_descriptor.bInterfaceNumber)
const int cSetOutDataPort = 5;

static volatile BOOL gTerminate = FALSE;

BOOL WINAPI ConsoleHandler(DWORD signal)
{
  gTerminate = (signal == CTRL_C_EVENT);
  return TRUE;
}

struct usb_dev_handle* GetAVRDevice(void)
{
  usb_dev_handle* avrDevice = NULL;

  const int STRING_MAX_SIZE = 256;
  char* product = malloc(STRING_MAX_SIZE);
  if (!product)
  {
    printf("Error: failed allocating memory!\n");
    return NULL;
  }

  for (struct usb_bus* bus = usb_get_busses(); bus && (avrDevice == NULL); bus = bus->next)
  {
    for (struct usb_device* dev = bus->devices; dev && (avrDevice == NULL); dev = dev->next)
    {
      usb_dev_handle* udev = usb_open(dev);
      if (udev)
      {
        memset(product, 0, STRING_MAX_SIZE);

        if (dev->descriptor.iProduct)
        {
          if (usb_get_string_simple(udev, dev->descriptor.iProduct, product, STRING_MAX_SIZE - 1) > 0)
          {
            if ((dev->descriptor.idVendor == cVendorUSBID) &&
              (dev->descriptor.idProduct == cDeviceUSBID) &&
              (dev->descriptor.bcdDevice == cDeviceVersion) &&
              (strcmp(cAvrDevString, product) == 0))
            {
              avrDevice = udev;
              // AVR309 has only one interface with InterfaceNumber cAVRIntf, so no need to find interface
            }
          }
        }

        printf("Found usb id %04X:%04X (\"%s\")\n",
          dev->descriptor.idVendor, dev->descriptor.idProduct, product);

        if (avrDevice == NULL)
        {
          usb_close(udev);
        }
      }
    }
  }

  free(product);

  return avrDevice;
}

// Do AVR309 vendor device requests (USB_ENDPOINT_IN | USB_TYPE_VENDOR)
// request number cSetOutDataPort
#define SetOutDataPort(aAvrDevice, aDataOutByte, aOutBuf) \
  usb_control_msg(aAvrDevice, USB_ENDPOINT_IN | USB_TYPE_VENDOR, \
    cSetOutDataPort, aDataOutByte, 0, aOutBuf, 1, 250)

//----------------------------------------------------------------------------------------------------------------
// cpuusage(void)
// ==============
// source: http://en.literateprograms.org/cpu_usage_(c,_windows_xp)
// Return a CHAR value in the range 0 - 100 representing actual CPU usage in percent.
//----------------------------------------------------------------------------------------------------------------
static char cpuusage()
{
  FILETIME              ft_sys_idle;
  FILETIME              ft_sys_kernel;
  FILETIME              ft_sys_user;

  ULARGE_INTEGER        ul_sys_idle;
  ULARGE_INTEGER        ul_sys_kernel;
  ULARGE_INTEGER        ul_sys_user;

  static ULARGE_INTEGER	ul_sys_idle_old;
  static ULARGE_INTEGER ul_sys_kernel_old;
  static ULARGE_INTEGER ul_sys_user_old;

  CHAR usage = 0;

  GetSystemTimes(&ft_sys_idle, /* System idle time */
    &ft_sys_kernel, /* system kernel time */
    &ft_sys_user); /* System user time */

  CopyMemory(&ul_sys_idle, &ft_sys_idle, sizeof(FILETIME)); // Could been optimized away...
  CopyMemory(&ul_sys_kernel, &ft_sys_kernel, sizeof(FILETIME)); // Could been optimized away...
  CopyMemory(&ul_sys_user, &ft_sys_user, sizeof(FILETIME)); // Could been optimized away...

  usage = (char)
  (
   (
    (
     (
      (ul_sys_kernel.QuadPart - ul_sys_kernel_old.QuadPart) +
      (ul_sys_user.QuadPart - ul_sys_user_old.QuadPart)
     )
     -
     (ul_sys_idle.QuadPart - ul_sys_idle_old.QuadPart)
    )
    * 100
   )
   /
   (
    (ul_sys_kernel.QuadPart - ul_sys_kernel_old.QuadPart) +
    (ul_sys_user.QuadPart - ul_sys_user_old.QuadPart)
   )
  );

  ul_sys_idle_old.QuadPart = ul_sys_idle.QuadPart;
  ul_sys_user_old.QuadPart = ul_sys_user.QuadPart;
  ul_sys_kernel_old.QuadPart = ul_sys_kernel.QuadPart;

  return usage;
}

int main(int argc, char** argv)
{
  printf("Start CPUmeter.....!\n");
  printf("argc= %i\n", argc);

  if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE))
  {
    printf("ERROR: Could not set control handler\n");
    return 1;
  }

  // Initialize the library.
  usb_init();

  // Find all busses.
  usb_find_busses();

  // Find all connected devices.
  usb_find_devices();

  usb_dev_handle* avrDevice = GetAVRDevice();
  if (avrDevice != NULL)
  {
    if (usb_set_configuration(avrDevice, cAVRConfig) < 0)
    {
      printf("error: setting config #%d failed\n", cAVRConfig);
    }
    else if (usb_claim_interface(avrDevice, cAVRIntf) < 0)
    {
      printf("error: claiming interface #%d failed\n", cAVRIntf);
    }
    else
    {
      int PWMmin, PWMmax, par1;
      PWMmin = 0;
      PWMmax = 100;
      char OutBuf[1];

      if (argc == 2)
      {
        par1 = atoi(argv[1]);
        if (par1 < 0)
        {
          par1 = -par1;
          if (par1 > 127) par1 = 127;
        }
        else
        {
          if (par1 > 127) par1 = 127;
          par1 += 0x80;
        }
        SetOutDataPort(avrDevice, par1, OutBuf);
        printf("Set PWM(out1) to %i\n", par1);
      }
      else
      {
        if (argc == 3)
        {
          PWMmin = atoi(argv[1]);
          PWMmax = atoi(argv[2]);
        }

        if (PWMmin < 0) PWMmin = 0;
        else if (PWMmin > 127) PWMmin = 127;

        if (PWMmax < 0) PWMmax = 0;
        else if (PWMmax > 127) PWMmax = 127;

        printf("PWMmin= %i, PWMmax= %i\n", PWMmin, PWMmax);

        SetOutDataPort(avrDevice, PWMmin + 0x80, OutBuf); // Set PortA

        printf("Softstart of heater and high voltage...\n");
        for (int pwm = 3; !gTerminate && (pwm < 35); pwm++)
        {
          SetOutDataPort(avrDevice, pwm, OutBuf);

          printf("SMPS PWM: %2d\r", pwm);
          Sleep(200);
        }
        if (!gTerminate)
        {
          printf("\nSoftstart completed...\n");

          double Xfiltered = 0;

          while (!gTerminate)
          {
            int X = cpuusage();

            Xfiltered = 0.8*Xfiltered + 0.2*X;
            int P = (int)(PWMmin + (Xfiltered / 100.0) * (PWMmax - PWMmin));
            if (P < 0) P = 0;
            else if (P > 127) P = 127;
            printf("CPU Usage: %3d   send %3d to USB  \r", X, P);
            SetOutDataPort(avrDevice, P + 0x80, OutBuf);
            Sleep(100);
          }
        }
      }

      usb_release_interface(avrDevice, cAVRIntf);
    }

    usb_close(avrDevice);
  }

  printf("End of program\n");
  return 0;
}
