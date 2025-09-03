//
//  ContentView.swift
//  Google Balls
//
//  Created by Bonki on 9/3/25.
//

import SwiftUI
import UIKit
import WebKit

struct WebViewControllerWrapper: UIViewControllerRepresentable {
    func makeUIViewController(context: Context) -> some UIViewController {
        return ViewController()
    }
    
    func updateUIViewController(_ uiViewController: UIViewControllerType, context: Context) {
        // h
    }
}

struct ContentView: View {
    var body: some View {
        WebViewControllerWrapper()
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
