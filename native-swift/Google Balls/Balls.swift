//
//  Balls.swift
//  Google Balls
//
//  Created by ween on 7/24/26.
//

import Foundation
import CoreGraphics
import SwiftUI

#if os(macOS)
import AppKit
import CoreVideo
public typealias PlatformView = NSView
public typealias PlatformColor = NSColor
#else
import UIKit
public typealias PlatformView = UIView
public typealias PlatformColor = UIColor
#endif

struct Vector3 {
    var x: Double = 0
    var y: Double = 0
    var z: Double = 0
}

struct RGBAColor {
    var r: CGFloat
    var g: CGFloat
    var b: CGFloat
    var a: CGFloat = 1.0

    static func fromHex(_ hex: String) -> RGBAColor {
        var sanitized = hex
        if sanitized.hasPrefix("#") { sanitized.removeFirst() }
        var value: UInt64 = 0
        Scanner(string: sanitized).scanHexInt64(&value)
        let r = CGFloat((value & 0xFF0000) >> 16) / 255.0
        let g = CGFloat((value & 0x00FF00) >> 8) / 255.0
        let b = CGFloat(value & 0x0000FF) / 255.0
        return RGBAColor(r: r, g: g, b: b, a: 1.0)
    }
}

final class BallPoint {
    var curPos: Vector3
    var originalPos: Vector3
    var targetPos: Vector3
    var velocity = Vector3()
    var color: RGBAColor
    var radius: Double
    var size: Double
    let friction = 0.8
    let springStrength = 0.1

    init(x: Double, y: Double, size: Double, colorHex: String) {
        curPos = Vector3(x: x, y: y, z: 0)
        originalPos = Vector3(x: x, y: y, z: 0)
        targetPos = Vector3(x: x, y: y, z: 0)
        self.size = size
        self.radius = size
        self.color = RGBAColor.fromHex(colorHex)
    }

    func update(dt: Double) {
        let targetFrameTime = 1.0 / 30.0
        let timeScale = dt / targetFrameTime

        let dx = targetPos.x - curPos.x
        let ax = dx * springStrength * timeScale
        velocity.x += ax
        velocity.x *= pow(friction, timeScale)
        curPos.x += velocity.x * timeScale

        let dy = targetPos.y - curPos.y
        let ay = dy * springStrength * timeScale
        velocity.y += ay
        velocity.y *= pow(friction, timeScale)
        curPos.y += velocity.y * timeScale

        let dox = originalPos.x - curPos.x
        let doy = originalPos.y - curPos.y
        let d = sqrt(dox * dox + doy * doy)

        targetPos.z = d / 100.0 + 1.0
        let dz = targetPos.z - curPos.z
        let az = dz * springStrength * timeScale
        velocity.z += az
        velocity.z *= pow(friction, timeScale)
        curPos.z += velocity.z * timeScale

        radius = size * curPos.z
        if radius < 1 { radius = 1 }
    }
}

final class BallPointCollection {
    var mousePos = Vector3()
    var points: [BallPoint] = []

    func addPoint(x: Double, y: Double, size: Double, color: String) {
        points.append(BallPoint(x: x, y: y, size: size, colorHex: color))
    }

    func update(dt: Double) {
        for point in points {
            let dx = mousePos.x - point.curPos.x
            let dy = mousePos.y - point.curPos.y
            let d = sqrt(dx * dx + dy * dy)

            if d < 150 {
                point.targetPos.x = point.curPos.x - dx
                point.targetPos.y = point.curPos.y - dy
            } else {
                point.targetPos.x = point.originalPos.x
                point.targetPos.y = point.originalPos.y
            }

            point.update(dt: dt)
        }
    }

    func draw(in context: CGContext) {
        for point in points {
            context.setFillColor(red: point.color.r, green: point.color.g, blue: point.color.b, alpha: point.color.a)
            let rect = CGRect(
                x: point.curPos.x - point.radius,
                y: point.curPos.y - point.radius,
                width: point.radius * 2,
                height: point.radius * 2
            )
            context.fillEllipse(in: rect)
        }
    }
}

struct BallPointData {
    let x: Double
    let y: Double
    let size: Double
    let color: String
}

let ballPointData: [BallPointData] = [
    .init(x: 202, y: 78, size: 9, color: "#ed9d33"), .init(x: 348, y: 83, size: 9, color: "#d44d61"),
    .init(x: 256, y: 69, size: 9, color: "#4f7af2"), .init(x: 214, y: 59, size: 9, color: "#ef9a1e"),
    .init(x: 265, y: 36, size: 9, color: "#4976f3"), .init(x: 300, y: 78, size: 9, color: "#269230"),
    .init(x: 294, y: 59, size: 9, color: "#1f9e2c"), .init(x: 45,  y: 88, size: 9, color: "#1c48dd"),
    .init(x: 268, y: 52, size: 9, color: "#2a56ea"), .init(x: 73,  y: 83, size: 9, color: "#3355d8"),
    .init(x: 294, y: 6,  size: 9, color: "#36b641"), .init(x: 235, y: 62, size: 9, color: "#2e5def"),
    .init(x: 353, y: 42, size: 8, color: "#d53747"), .init(x: 336, y: 52, size: 8, color: "#eb676f"),
    .init(x: 208, y: 41, size: 8, color: "#f9b125"), .init(x: 321, y: 70, size: 8, color: "#de3646"),
    .init(x: 8,   y: 60, size: 8, color: "#2a59f0"), .init(x: 180, y: 81, size: 8, color: "#eb9c31"),
    .init(x: 146, y: 65, size: 8, color: "#c41731"), .init(x: 145, y: 49, size: 8, color: "#d82038"),
    .init(x: 246, y: 34, size: 8, color: "#5f8af8"), .init(x: 169, y: 69, size: 8, color: "#efa11e"),
    .init(x: 273, y: 99, size: 8, color: "#2e55e2"), .init(x: 248, y: 120, size: 8, color: "#4167e4"),
    .init(x: 294, y: 41, size: 8, color: "#0b991a"), .init(x: 267, y: 114, size: 8, color: "#4869e3"),
    .init(x: 78,  y: 67, size: 8, color: "#3059e3"), .init(x: 294, y: 23, size: 8, color: "#10a11d"),
    .init(x: 117, y: 83, size: 8, color: "#cf4055"), .init(x: 137, y: 80, size: 8, color: "#cd4359"),
    .init(x: 14,  y: 71, size: 8, color: "#2855ea"), .init(x: 331, y: 80, size: 8, color: "#ca273c"),
    .init(x: 25,  y: 82, size: 8, color: "#2650e1"), .init(x: 233, y: 46, size: 8, color: "#4a7bf9"),
    .init(x: 73,  y: 13, size: 8, color: "#3d65e7"), .init(x: 327, y: 35, size: 6, color: "#f47875"),
    .init(x: 319, y: 46, size: 6, color: "#f36764"), .init(x: 256, y: 81, size: 6, color: "#1d4eeb"),
    .init(x: 244, y: 88, size: 6, color: "#698bf1"), .init(x: 194, y: 32, size: 6, color: "#fac652"),
    .init(x: 97,  y: 56, size: 6, color: "#ee5257"), .init(x: 105, y: 75, size: 6, color: "#cf2a3f"),
    .init(x: 42,  y: 4,  size: 6, color: "#5681f5"), .init(x: 10,  y: 27, size: 6, color: "#4577f6"),
    .init(x: 166, y: 55, size: 6, color: "#f7b326"), .init(x: 266, y: 88, size: 6, color: "#2b58e8"),
    .init(x: 178, y: 34, size: 6, color: "#facb5e"), .init(x: 100, y: 65, size: 6, color: "#e02e3d"),
    .init(x: 343, y: 32, size: 6, color: "#f16d6f"), .init(x: 59,  y: 5,  size: 6, color: "#507bf2"),
    .init(x: 27,  y: 9,  size: 6, color: "#5683f7"), .init(x: 233, y: 116, size: 6, color: "#3158e2"),
    .init(x: 123, y: 32, size: 6, color: "#f0696c"), .init(x: 6,   y: 38, size: 6, color: "#3769f6"),
    .init(x: 63,  y: 62, size: 6, color: "#6084ef"), .init(x: 6,   y: 49, size: 6, color: "#2a5cf4"),
    .init(x: 108, y: 36, size: 6, color: "#f4716e"), .init(x: 169, y: 43, size: 6, color: "#f8c247"),
    .init(x: 137, y: 37, size: 6, color: "#e74653"), .init(x: 318, y: 58, size: 6, color: "#ec4147"),
    .init(x: 226, y: 100, size: 5, color: "#4876f1"), .init(x: 101, y: 46, size: 5, color: "#ef5c5c"),
    .init(x: 226, y: 108, size: 5, color: "#2552ea"), .init(x: 17,  y: 17, size: 5, color: "#4779f7"),
    .init(x: 232, y: 93, size: 5, color: "#4b78f1")
]

func computeBounds(_ data: [BallPointData]) -> (w: Double, h: Double) {
    var minX = Double.greatestFiniteMagnitude, maxX = -Double.greatestFiniteMagnitude
    var minY = Double.greatestFiniteMagnitude, maxY = -Double.greatestFiniteMagnitude
    for p in data {
        minX = min(minX, p.x); maxX = max(maxX, p.x)
        minY = min(minY, p.y); maxY = max(maxY, p.y)
    }
    return (maxX - minX, maxY - minY)
}

public final class GoogleBallsView: PlatformView {
    private let pointCollection = BallPointCollection()

    private var timer: Timer?

    #if os(macOS)
    private var displayLink: CVDisplayLink?
    #else
    private var displayLink: CADisplayLink?
    #endif
    private var lastTimestamp: CFTimeInterval?

    private var lastSize: CGSize = .zero

    public var isDarkBackground: Bool = false {
        didSet {
            guard isDarkBackground != oldValue else { return }
            updateBackgroundColor()
        }
    }

    public var isLocked30FPS: Bool = true {
        didSet {
            guard isLocked30FPS != oldValue else { return }
            restartTimer()
        }
    }

    #if os(macOS)
    private var trackingArea: NSTrackingArea?
    #endif

    public override init(frame frameRect: CGRect) {
        super.init(frame: frameRect)
        commonInit()
    }

    public required init?(coder: NSCoder) {
        super.init(coder: coder)
        commonInit()
    }

    private func commonInit() {
        #if os(macOS)
        wantsLayer = true
        #else
        isMultipleTouchEnabled = false
        #endif
        updateBackgroundColor()

        for data in ballPointData {
            pointCollection.addPoint(x: data.x, y: data.y, size: data.size, color: data.color)
        }

        restartTimer()
    }

    private func updateBackgroundColor() {
        let color: PlatformColor = isDarkBackground
            ? PlatformColor(red: 26/255.0, green: 26/255.0, blue: 26/255.0, alpha: 1.0)
            : .white
        #if os(macOS)
        layer?.backgroundColor = color.cgColor
        #else
        backgroundColor = color
        #endif
    }

    private func restartTimer() {
        timer?.invalidate()
        timer = nil
        stopDisplayLink()
        lastTimestamp = nil

        if isLocked30FPS {
            timer = Timer.scheduledTimer(withTimeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
                self?.tick(dt: 1.0 / 30.0)
            }
        } else {
            startDisplayLink()
        }
    }

    #if os(macOS)
    private func startDisplayLink() {
        var link: CVDisplayLink?
        CVDisplayLinkCreateWithActiveCGDisplays(&link)
        guard let link else { return }

        CVDisplayLinkSetOutputHandler(link) { [weak self] _, _, _, _, _ in
            DispatchQueue.main.async {
                self?.tickFromDisplayLink()
            }
            return kCVReturnSuccess
        }

        CVDisplayLinkStart(link)
        displayLink = link
    }

    private func stopDisplayLink() {
        if let link = displayLink {
            CVDisplayLinkStop(link)
        }
        displayLink = nil
    }
    #else
    private func startDisplayLink() {
        let link = CADisplayLink(target: self, selector: #selector(displayLinkFired))
        link.add(to: .main, forMode: .common)
        displayLink = link
    }

    private func stopDisplayLink() {
        displayLink?.invalidate()
        displayLink = nil
    }

    @objc private func displayLinkFired() {
        tickFromDisplayLink()
    }
    #endif

    private func tickFromDisplayLink() {
        let now = CACurrentMediaTime()
        let dt = lastTimestamp.map { now - $0 } ?? (1.0 / 60.0)
        lastTimestamp = now
        tick(dt: min(dt, 0.1))
    }

    private func tick(dt: Double) {
        pointCollection.update(dt: dt)
        #if os(macOS)
        needsDisplay = true
        #else
        setNeedsDisplay()
        #endif
    }

    private func recenterIfNeeded() {
        guard bounds.size != lastSize, bounds.size != .zero else { return }
        lastSize = bounds.size

        let b = computeBounds(ballPointData)
        let offsetX = (Double(bounds.width) / 2.0) - (b.w / 2.0)
        let offsetY = (Double(bounds.height) / 2.0) - (b.h / 2.0)

        for (i, point) in pointCollection.points.enumerated() {
            let data = ballPointData[i]
            point.originalPos.x = offsetX + data.x
            point.originalPos.y = offsetY + data.y
            point.curPos.x = point.originalPos.x
            point.curPos.y = point.originalPos.y
        }
    }

    private func drawBalls(in context: CGContext) {
        context.setShouldAntialias(true)
        pointCollection.draw(in: context)
    }

    #if os(macOS)
    public override var isFlipped: Bool { true }

    public override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)
        guard let context = NSGraphicsContext.current?.cgContext else { return }
        drawBalls(in: context)
    }

    public override func updateTrackingAreas() {
        super.updateTrackingAreas()
        if let ta = trackingArea { removeTrackingArea(ta) }
        let ta = NSTrackingArea(
            rect: bounds,
            options: [.activeAlways, .mouseMoved, .inVisibleRect],
            owner: self,
            userInfo: nil
        )
        addTrackingArea(ta)
        trackingArea = ta
    }

    public override func mouseMoved(with event: NSEvent) {
        let loc = convert(event.locationInWindow, from: nil)
        pointCollection.mousePos.x = Double(loc.x)
        pointCollection.mousePos.y = Double(loc.y)
    }

    public override func layout() {
        super.layout()
        recenterIfNeeded()
    }
    #else
    public override func draw(_ rect: CGRect) {
        super.draw(rect)
        guard let context = UIGraphicsGetCurrentContext() else { return }
        drawBalls(in: context)
    }

    public override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        let loc = touch.location(in: self)
        pointCollection.mousePos.x = Double(loc.x)
        pointCollection.mousePos.y = Double(loc.y)
    }

    public override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        pointCollection.mousePos.x = -10000
        pointCollection.mousePos.y = -10000
    }

    public override func layoutSubviews() {
        super.layoutSubviews()
        recenterIfNeeded()
    }
    #endif

    deinit {
        timer?.invalidate()
        #if os(macOS)
        if let link = displayLink {
            CVDisplayLinkStop(link)
        }
        #else
        displayLink?.invalidate()
        #endif
    }
}

#if os(macOS)
public struct GoogleBallsRepresentable: NSViewRepresentable {
    var isDarkBackground: Bool
    var isLocked30FPS: Bool

    public func makeNSView(context: Context) -> GoogleBallsView {
        let view = GoogleBallsView(frame: .zero)
        view.isDarkBackground = isDarkBackground
        view.isLocked30FPS = isLocked30FPS
        return view
    }

    public func updateNSView(_ nsView: GoogleBallsView, context: Context) {
        nsView.isDarkBackground = isDarkBackground
        nsView.isLocked30FPS = isLocked30FPS
    }
}
#else
public struct GoogleBallsRepresentable: UIViewRepresentable {
    var isDarkBackground: Bool
    var isLocked30FPS: Bool

    public func makeUIView(context: Context) -> GoogleBallsView {
        let view = GoogleBallsView(frame: .zero)
        view.isDarkBackground = isDarkBackground
        view.isLocked30FPS = isLocked30FPS
        return view
    }

    public func updateUIView(_ uiView: GoogleBallsView, context: Context) {
        uiView.isDarkBackground = isDarkBackground
        uiView.isLocked30FPS = isLocked30FPS
    }
}
#endif

private struct BallsCheckboxToggle: View {
    let title: LocalizedStringKey
    @Binding var isOn: Bool
    let isDarkBackground: Bool

    var body: some View {
        Button {
            isOn.toggle()
        } label: {
            HStack(spacing: 6) {
                Image(systemName: isOn ? "checkmark.square.fill" : "square")
                    .foregroundColor(isOn ? .accentColor : (isDarkBackground ? .white : .black))

                Text(title)
                    .foregroundColor(isDarkBackground ? .white : .black)
            }
        }
        .buttonStyle(.plain)
    }
}


public struct GoogleBallsSwiftUIView: View {
    @Environment(\.colorScheme) private var colorScheme
    @State private var isDarkBackground: Bool = false
    @State private var isLocked30FPS: Bool = true
    @State private var hasSetInitialColorScheme = false

    public init() {}

    public var body: some View {
        ZStack(alignment: .top) {
            (isDarkBackground ? Color(red: 26/255, green: 26/255, blue: 26/255) : Color.white)
                .ignoresSafeArea()

            GoogleBallsRepresentable(
                isDarkBackground: isDarkBackground,
                isLocked30FPS: isLocked30FPS
            )
            .ignoresSafeArea()

            HStack(spacing: 24) {
                BallsCheckboxToggle(
                    title: "dark_background",
                    isOn: $isDarkBackground,
                    isDarkBackground: isDarkBackground
                )

                BallsCheckboxToggle(
                    title: "lock_30_fps",
                    isOn: $isLocked30FPS,
                    isDarkBackground: isDarkBackground
                )
            }
            .padding(.top, 12)
            .padding(.horizontal, 16)
        }
        .preferredColorScheme(isDarkBackground ? .dark : .light)
        .onAppear {
            if !hasSetInitialColorScheme {
                isDarkBackground = colorScheme == .dark
                hasSetInitialColorScheme = true
            }
        }
    }

}
