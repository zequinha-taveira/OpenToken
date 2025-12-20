---
description: How to build and run the OpenToken App for Mobile and Desktop
---

# Build and Run OpenToken App

Follow these steps to run the application on your desired platform.

### Prerequisites
- [Flutter SDK](https://docs.flutter.dev/get-started/install) installed and added to your PATH.
- Android Studio / Xcode (for Mobile).
- Visual Studio with C++ workload (for Windows Desktop).

### Setup
1. Open a terminal in the project root.
2. Navigate to the app directory:
   ```powershell
   cd opentoken_app
   ```
3. Fetch dependencies:
   ```powershell
   flutter pub get
   ```

### Running the App

#### Windows Desktop
To run natively on Windows:
// turbo
```powershell
flutter run -d windows
```

#### Android Mobile
Connect your device or start an emulator and run:
```powershell
flutter run -d android
```

#### iOS Mobile
```powershell
flutter run -d ios
```

### Building for Production
To create a release build:
- **Windows**: `flutter build windows`
- **Android**: `flutter build apk`
- **iOS**: `flutter build ios --release`
