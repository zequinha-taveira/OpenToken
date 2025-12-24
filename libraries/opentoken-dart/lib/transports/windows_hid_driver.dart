import 'dart:ffi';

import 'package:win32/win32.dart';

// -----------------------------------------------------------------------------
// FFI DEFINITIONS & DRIVER WRAPPERS
// -----------------------------------------------------------------------------

final class HIDD_ATTRIBUTES extends Struct {
  @Uint32()
  external int Size;

  @Uint16()
  external int VendorID;

  @Uint16()
  external int ProductID;

  @Uint16()
  external int VersionNumber;
}

typedef HidD_GetAttributes_Native = Int32 Function(
    IntPtr HidDeviceObject, Pointer<HIDD_ATTRIBUTES> Attributes);
typedef HidD_GetAttributes_Dart = int Function(
    int HidDeviceObject, Pointer<HIDD_ATTRIBUTES> Attributes);

final _hidDll = DynamicLibrary.open('hid.dll');
final _hidD_GetAttributes =
    _hidDll.lookupFunction<HidD_GetAttributes_Native, HidD_GetAttributes_Dart>(
        'HidD_GetAttributes');

bool HidD_GetAttributes(int deviceHandle, Pointer<HIDD_ATTRIBUTES> attributes) {
  return _hidD_GetAttributes(deviceHandle, attributes) != 0;
}

typedef HidD_GetHidGuid_Native = Void Function(Pointer<GUID> HidGuid);
typedef HidD_GetHidGuid_Dart = void Function(Pointer<GUID> HidGuid);
final _hidD_GetHidGuid =
    _hidDll.lookupFunction<HidD_GetHidGuid_Native, HidD_GetHidGuid_Dart>(
        'HidD_GetHidGuid');

void HidD_GetHidGuid(Pointer<GUID> guid) => _hidD_GetHidGuid(guid);
