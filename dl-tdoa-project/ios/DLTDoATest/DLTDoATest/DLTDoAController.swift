import Foundation
import NearbyInteraction
import CoreLocation

@MainActor
final class DLTDoAController: NSObject, ObservableObject {
    @Published var networkIdentifierText = "1"
    @Published private(set) var capabilityText = "Chưa kiểm tra"
    @Published private(set) var sessionState = "Đã dừng"
    @Published private(set) var logText = ""
    @Published private(set) var isRunning = false

    private let locationManager = CLLocationManager()
    private var session: NISession?

    override init() {
        super.init()
        locationManager.delegate = self
        appendLog("DL-TDoA Test ready")
    }

    func checkCapability() {
        let supported = NISession.deviceCapabilities.supportsDLTDOAMeasurement
        capabilityText = supported ? "Hỗ trợ DL-TDoA" : "Không hỗ trợ DL-TDoA"
        appendLog("supportsDLTDOAMeasurement = \(supported)")
    }

    func requestLocationPermission() {
        locationManager.requestWhenInUseAuthorization()
    }

    func start() {
        checkCapability()
        guard NISession.deviceCapabilities.supportsDLTDOAMeasurement else {
            sessionState = "Thiết bị không hỗ trợ"
            return
        }
        guard locationManager.authorizationStatus == .authorizedWhenInUse ||
              locationManager.authorizationStatus == .authorizedAlways else {
            sessionState = "Cần quyền Location"
            requestLocationPermission()
            return
        }
        guard let networkIdentifier = Int(networkIdentifierText) else {
            sessionState = "Network Identifier không hợp lệ"
            return
        }

        let newSession = NISession()
        newSession.delegate = self
        newSession.delegateQueue = .main

        // init(networkIdentifier:) mặc định dùng Bluetooth Low Energy discovery.
        // Không gọi overload discoveryMethod để project vẫn build bằng iOS 26 SDK đầu tiên.
        let configuration = NIDLTDOAConfiguration(networkIdentifier: networkIdentifier)
        session = newSession
        newSession.run(configuration)
        isRunning = true
        sessionState = "Đang chạy — BLE discovery"
        appendLog("Started networkIdentifier = \(networkIdentifier), discovery = BLE (default)")
    }

    func stop() {
        session?.invalidate()
        session = nil
        isRunning = false
        sessionState = "Đã dừng"
        appendLog("Session invalidated")
    }

    func clearLog() {
        logText = ""
    }

    private func appendLog(_ message: String) {
        let formatter = ISO8601DateFormatter()
        let line = "[\(formatter.string(from: Date()))] \(message)"
        logText = logText.isEmpty ? line : "\(logText)\n\(line)"
    }

    private func dumpMeasurement(_ measurement: NIDLTDOAMeasurement) {
        appendLog("--------------------------------")
        appendLog("DL-TDOA MEASUREMENT")
        appendLog("--------------------------------")
        // Reflection intentionally logs every property exposed by the installed SDK
        // without referencing properties that may only exist in a newer SDK.
        appendLog(String(reflecting: measurement))
        let mirror = Mirror(reflecting: measurement)
        if mirror.children.isEmpty {
            appendLog("No Mirror-visible fields; inspect the object description above.")
        } else {
            for child in mirror.children {
                appendLog("\(child.label ?? "field"): \(String(reflecting: child.value))")
            }
        }
        appendLog("--------------------------------")
    }
}

extension DLTDoAController: CLLocationManagerDelegate {
    nonisolated func locationManagerDidChangeAuthorization(_ manager: CLLocationManager) {
        Task { @MainActor in
            appendLog("Location authorization = \(manager.authorizationStatus.rawValue)")
            if manager.authorizationStatus == .authorizedWhenInUse ||
               manager.authorizationStatus == .authorizedAlways {
                sessionState = "Đã cấp quyền Location"
            }
        }
    }
}

extension DLTDoAController: NISessionDelegate {
    nonisolated func session(_ session: NISession, didUpdateDLTDOA measurements: [NIDLTDOAMeasurement]) {
        Task { @MainActor in
            appendLog("Received \(measurements.count) measurement(s)")
            measurements.forEach(dumpMeasurement)
        }
    }

    nonisolated func session(_ session: NISession, didInvalidateWith error: Error) {
        Task { @MainActor in
            isRunning = false
            sessionState = "Session lỗi"
            appendLog("NISession invalidated: \(error.localizedDescription)")
        }
    }

    nonisolated func sessionWasSuspended(_ session: NISession) {
        Task { @MainActor in
            sessionState = "Tạm dừng"
            appendLog("NISession suspended")
        }
    }

    nonisolated func sessionSuspensionEnded(_ session: NISession) {
        Task { @MainActor in
            sessionState = "Tiếp tục"
            appendLog("NISession suspension ended")
        }
    }
}
