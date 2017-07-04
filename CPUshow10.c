#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "cpuusage.h"
#ifdef LINUX
  #include <signal.h>
  #include <usb.h>
#else
  #include <windows.h>
  #include "lusb0_usb.h"
#endif

const char* cAvrDevString = "AVR309:USB to UART protocol converter (simple)";
const unsigned short cVendorUSBID = 0x03eb; // Atmel
const unsigned short cDeviceUSBID = 0x21FE; // RS232 converter AT90S2313
const unsigned short cDeviceVersion = 2;    // v0.02
const int cAVRConfig = 1; // Must be 1 for AVR309
const int cAVRIntf = 0;   // Must be 0 for AVR309 (usb_interface_descriptor.bInterfaceNumber)
const int cSetOutDataPort = 5;

static volatile bool gTerminate = false;

#ifdef LINUX
  void SignalHandler(int sig)
  {
    gTerminate = true;
  }

  #define SetConsoleCtrlHandler(handler, add) (SIG_ERR != signal(SIGINT, handler))

  #define Sleep(ms) usleep(ms * 1000)
#else
  BOOL WINAPI SignalHandler(DWORD signal)
  {
    gTerminate = (signal == CTRL_C_EVENT);
    return true;
  }
#endif

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

  for (struct usb_bus* bus = usb_busses; bus && (avrDevice == NULL); bus = bus->next)
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
          else
          {
            perror("error: Could not read usb descriptor");
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

int main(int argc, char** argv)
{
  printf("Start CPUmeter.....!\n");
  printf("argc= %i\n", argc);

  if (!SetConsoleCtrlHandler(SignalHandler, true))
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
          fflush(stdout);
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
            fflush(stdout);
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
