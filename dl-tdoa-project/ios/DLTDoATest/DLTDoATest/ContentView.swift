import SwiftUI

struct ContentView: View {
    @EnvironmentObject private var controller: DLTDoAController

    var body: some View {
        NavigationStack {
            VStack(spacing: 16) {
                GroupBox("Thiết bị") {
                    LabeledContent("Capability", value: controller.capabilityText)
                    LabeledContent("Session", value: controller.sessionState)
                }

                GroupBox("Cấu hình") {
                    TextField("Network Identifier", text: $controller.networkIdentifierText)
                        .keyboardType(.numberPad)
                        .textFieldStyle(.roundedBorder)
                        .accessibilityIdentifier("networkIdentifierField")
                    LabeledContent("Discovery", value: "Bluetooth Low Energy (default)")
                }

                HStack {
                    Button("Kiểm tra") { controller.checkCapability() }
                        .buttonStyle(.bordered)
                        .accessibilityIdentifier("checkCapabilityButton")
                    Button("Xin quyền") { controller.requestLocationPermission() }
                        .buttonStyle(.bordered)
                        .accessibilityIdentifier("requestLocationButton")
                    Button(controller.isRunning ? "Dừng" : "Bắt đầu") {
                        controller.isRunning ? controller.stop() : controller.start()
                    }
                    .buttonStyle(.borderedProminent)
                    .accessibilityIdentifier("toggleSessionButton")
                }

                GroupBox("Log") {
                    ScrollView {
                        Text(controller.logText)
                            .font(.system(.caption, design: .monospaced))
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .textSelection(.enabled)
                    }
                    .accessibilityIdentifier("measurementLogView")
                }
                .frame(maxHeight: .infinity)
            }
            .padding()
            .navigationTitle("DL‑TDoA Test")
            .toolbar {
                Button("Xóa log") { controller.clearLog() }
                    .accessibilityIdentifier("clearLogButton")
            }
        }
    }
}

#Preview {
    ContentView()
        .environmentObject(DLTDoAController())
}
