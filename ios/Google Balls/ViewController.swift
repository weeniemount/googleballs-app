//
//  ViewController.swift
//  Google Balls
//
//  Created by Bonki on 9/3/25.
//

import SwiftUI
import UIKit
import WebKit

class ViewController: UIViewController {
    var webView: WKWebView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        webView = WKWebView(frame: self.view.bounds)
        webView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        view.addSubview(webView)
        
        loadLocalHtml()
    }
    
    func loadLocalHtml() {
        if let htmlPath = Bundle.main.path(forResource: "balls", ofType: "html") {
            let url = URL(fileURLWithPath: htmlPath)
            webView.loadFileURL(url, allowingReadAccessTo: url.deletingLastPathComponent())
            print("yoo twin i found the html")
        } else {
            print("balls.html not found :(")
        }
    }
}
