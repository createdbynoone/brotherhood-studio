import Cocoa
// usage: ui.swift <cmd> args...   cmds: bounds | click x y [cmd|shift] | dclick x y | drag x1 y1 x2 y2 [cmd|shift] | key <name> [cmd|shift] | shot <file>
func win() -> CGRect? {
    let list = CGWindowListCopyWindowInfo([.optionOnScreenOnly], kCGNullWindowID) as? [[String: Any]] ?? []
    for w in list where (w[kCGWindowOwnerName as String] as? String) == "Brotherhood Studio" {
        if let b = w[kCGWindowBounds as String] as? [String: CGFloat], (b["Height"] ?? 0) > 300 {
            return CGRect(x: b["X"]!, y: b["Y"]!, width: b["Width"]!, height: b["Height"]!)
        }
    }
    return nil
}
guard let b = win() else { print("no window"); exit(1) }
// Never send synthetic input to another app: the target must be frontmost.
if NSWorkspace.shared.frontmostApplication?.localizedName != "Brotherhood Studio" && CommandLine.arguments[1] != "bounds" {
    print("ABORT: front app is", NSWorkspace.shared.frontmostApplication?.localizedName ?? "?"); exit(2)
}
let a = CommandLine.arguments
func flags(_ s: String?) -> CGEventFlags { s == "cmd" ? .maskCommand : s == "shift" ? .maskShift : [] }
func mouse(_ t: CGEventType, _ x: CGFloat, _ y: CGFloat, _ f: CGEventFlags = [], clicks: Int64 = 1) {
    let e = CGEvent(mouseEventSource: nil, mouseType: t, mouseCursorPosition: CGPoint(x: b.minX + x, y: b.minY + y), mouseButton: .left)
    e?.flags = f; e?.setIntegerValueField(.mouseEventClickState, value: clicks); e?.post(tap: .cghidEventTap)
}
func num(_ i: Int) -> CGFloat { CGFloat(Double(a[i])!) }
switch a[1] {
case "bounds": print(b)
case "click":
    let f = flags(a.count > 4 ? a[4] : nil)
    mouse(.mouseMoved, num(2), num(3)); usleep(150_000); mouse(.leftMouseDown, num(2), num(3), f); usleep(60_000); mouse(.leftMouseUp, num(2), num(3), f)
case "dclick":
    mouse(.mouseMoved, num(2), num(3)); usleep(150_000)
    mouse(.leftMouseDown, num(2), num(3)); usleep(40_000); mouse(.leftMouseUp, num(2), num(3))
    usleep(80_000)
    mouse(.leftMouseDown, num(2), num(3), [], clicks: 2); usleep(40_000); mouse(.leftMouseUp, num(2), num(3), [], clicks: 2)
case "drag":
    let f = flags(a.count > 6 ? a[6] : nil)
    mouse(.mouseMoved, num(2), num(3)); usleep(150_000); mouse(.leftMouseDown, num(2), num(3), f); usleep(100_000)
    let n = 24
    for i in 1...n { let t = CGFloat(i) / CGFloat(n); mouse(.leftMouseDragged, num(2) + (num(4) - num(2)) * t, num(3) + (num(5) - num(3)) * t, f); usleep(20_000) }
    usleep(120_000); mouse(.leftMouseUp, num(4), num(5), f)
case "key":
    let codes: [String: CGKeyCode] = ["k": 40, "m": 46, "d": 2, "u": 32, "a": 0, "f": 3, "n": 45, "i": 34, "left": 123, "right": 124, "up": 126, "down": 125, "esc": 53, "return": 36, "space": 49, "bracketr": 30, "bracketl": 33]
    let code = codes[a[2]]!
    let src = CGEventSource(stateID: .hidSystemState)
    let d = CGEvent(keyboardEventSource: src, virtualKey: code, keyDown: true); d?.flags = flags(a.count > 3 ? a[3] : nil); d?.post(tap: .cghidEventTap)
    usleep(40_000)
    let u = CGEvent(keyboardEventSource: src, virtualKey: code, keyDown: false); u?.flags = flags(a.count > 3 ? a[3] : nil); u?.post(tap: .cghidEventTap)
default: print("?")
}
