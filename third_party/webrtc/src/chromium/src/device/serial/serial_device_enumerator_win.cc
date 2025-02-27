// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/serial/serial_device_enumerator_win.h"

#include <windows.h>

#include <ntddser.h>
#include <setupapi.h>

#include "base/memory/scoped_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/re2/re2/re2.h"

namespace device {

namespace {

// Searches the specified device info for a property with the specified key,
// assigns the result to value, and returns whether the operation was
// successful.
bool GetProperty(HDEVINFO dev_info,
                 SP_DEVINFO_DATA dev_info_data,
                 const int key,
                 std::string* value) {
  // We don't know how much space the property's value will take up, so we call
  // the property retrieval function once to fetch the size of the required
  // value buffer, then again once we've allocated a sufficiently large buffer.
  DWORD buffer_size = 0;
  SetupDiGetDeviceRegistryProperty(dev_info, &dev_info_data, key, nullptr,
                                   nullptr, buffer_size, &buffer_size);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    return false;

  scoped_ptr<wchar_t[]> buffer(new wchar_t[buffer_size]);
  if (!SetupDiGetDeviceRegistryProperty(dev_info, &dev_info_data, key, nullptr,
                                        reinterpret_cast<PBYTE>(buffer.get()),
                                        buffer_size, nullptr))
    return false;

  *value = base::WideToUTF8(buffer.get());
  return true;
}

// Searches for the COM port in the device's friendly name, assigns its value to
// com_port, and returns whether the operation was successful.
bool GetCOMPort(const std::string friendly_name, std::string* com_port) {
  return RE2::PartialMatch(friendly_name, ".* \\((COM[0-9]+)\\)", com_port);
}

// Searches for the display name in the device's friendly name, assigns its
// value to display_name, and returns whether the operation was successful.
bool GetDisplayName(const std::string friendly_name,
                    std::string* display_name) {
  return RE2::PartialMatch(friendly_name, "(.*) \\(COM[0-9]+\\)", display_name);
}

// Searches for the vendor ID in the device's hardware ID, assigns its value to
// vendor_id, and returns whether the operation was successful.
bool GetVendorID(const std::string hardware_id, uint32_t* vendor_id) {
  std::string vendor_id_str;
  return RE2::PartialMatch(hardware_id, "VID_([0-9]+)", &vendor_id_str) &&
         base::HexStringToUInt(vendor_id_str, vendor_id);
}

// Searches for the product ID in the device's product ID, assigns its value to
// product_id, and returns whether the operation was successful.
bool GetProductID(const std::string hardware_id, uint32_t* product_id) {
  std::string product_id_str;
  return RE2::PartialMatch(hardware_id, "PID_([0-9]+)", &product_id_str) &&
         base::HexStringToUInt(product_id_str, product_id);
}

}  // namespace

// static
scoped_ptr<SerialDeviceEnumerator> SerialDeviceEnumerator::Create() {
  return scoped_ptr<SerialDeviceEnumerator>(new SerialDeviceEnumeratorWin());
}

SerialDeviceEnumeratorWin::SerialDeviceEnumeratorWin() {}

SerialDeviceEnumeratorWin::~SerialDeviceEnumeratorWin() {}

mojo::Array<serial::DeviceInfoPtr> SerialDeviceEnumeratorWin::GetDevices() {
  mojo::Array<serial::DeviceInfoPtr> devices(0);

  // Make a device interface query to find all serial devices.
  HDEVINFO dev_info =
      SetupDiGetClassDevs(&GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR, 0, 0,
                          DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (dev_info == INVALID_HANDLE_VALUE)
    return devices.Pass();

  SP_DEVINFO_DATA dev_info_data;
  dev_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
  for (DWORD i = 0; SetupDiEnumDeviceInfo(dev_info, i, &dev_info_data); i++) {
    std::string friendly_name, com_port;
    // SPDRP_FRIENDLYNAME looks like "USB_SERIAL_PORT (COM3)".
    if (!GetProperty(dev_info, dev_info_data, SPDRP_FRIENDLYNAME,
                     &friendly_name) ||
        !GetCOMPort(friendly_name, &com_port))
      // In Windows, the COM port is the path used to uniquely identify the
      // serial device. If the COM can't be found, ignore the device.
      continue;

    serial::DeviceInfoPtr info(serial::DeviceInfo::New());
    info->path = com_port;

    std::string display_name;
    if (GetDisplayName(friendly_name, &display_name))
      info->display_name = display_name;

    std::string hardware_id;
    // SPDRP_HARDWAREID looks like "FTDIBUS\COMPORT&VID_0403&PID_6001".
    if (GetProperty(dev_info, dev_info_data, SPDRP_HARDWAREID, &hardware_id)) {
      uint32_t vendor_id, product_id;
      if (GetVendorID(hardware_id, &vendor_id)) {
        info->has_vendor_id = true;
        info->vendor_id = vendor_id;
      }
      if (GetProductID(hardware_id, &product_id)) {
        info->has_product_id = true;
        info->product_id = product_id;
      }
    }

    devices.push_back(info.Pass());
  }

  SetupDiDestroyDeviceInfoList(dev_info);
  return devices.Pass();
}

}  // namespace device
