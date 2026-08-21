# DLTDoATest — Phase 1

Minimal SwiftUI capability and measurement logger for Apple Nearby Interaction DL‑TDoA.

## Requirements

- macOS with Xcode containing the iOS 26 SDK or newer.
- A physical iPhone/iPad running iOS/iPadOS 26 or newer.
- An Apple Developer signing team.
- DL‑TDoA entitlement if required by the target OS/account. Apple states it is no longer required in iOS 27+.

## Create/open the Xcode project

This repository intentionally stores plain Swift sources because Xcode cannot run on Windows.

1. On the Mac, create an **iOS App** project named `DLTDoATest` using SwiftUI and Swift.
2. Set the deployment target to **iOS 26.0**.
3. Replace generated Swift files with the files in the `DLTDoATest/` directory.
4. Add the two usage-description keys from `Info.plist` to the target Info settings. If using the supplied plist directly, disable automatic Info.plist generation and set its path.
5. Select your signing team and a unique bundle identifier.
6. Build for a physical device, not Simulator.

## Test order

1. Tap **Kiểm tra** and record `supportsDLTDOAMeasurement`.
2. Tap **Xin quyền** and grant location authorization.
3. Enter the same Network Identifier as the anchor infrastructure.
4. Tap **Bắt đầu**.
5. Record session errors or `NIDLTDOAMeasurement` callbacks.

The logger uses reflection for measurement details so it doesn't call fields that may only exist in a newer SDK. After the exact Xcode SDK is known, replace reflection with typed property logging verified against that SDK header.
