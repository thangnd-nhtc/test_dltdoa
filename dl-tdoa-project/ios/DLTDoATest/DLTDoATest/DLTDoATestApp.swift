import SwiftUI

@main
struct DLTDoATestApp: App {
    @StateObject private var controller = DLTDoAController()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(controller)
        }
    }
}
