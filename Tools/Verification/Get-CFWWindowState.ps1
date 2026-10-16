# Read-only Win32 process-window census. Explicit one-step restore operations are optional.
param(
    [Parameter(Mandatory = $true)] [ValidateRange(1, 2147483647)] [int] $TargetPid,
    [ValidateSet('Snapshot', 'Restore', 'ShowNormal', 'Foreground', 'BringToTop', 'SysRestore')]
    [string] $Operation = 'Snapshot',
    [string] $Hwnd,
    [string] $OutputPath
)

$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class CFWWindowNative {
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct WINDOWPLACEMENT {
        public int Length, Flags, ShowCmd;
        public POINT MinPosition, MaxPosition;
        public RECT NormalPosition;
    }
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)] public struct MONITORINFOEX {
        public int Size;
        public RECT Monitor, Work;
        public int Flags;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string Device;
    }
    [StructLayout(LayoutKind.Sequential)] public struct GUITHREADINFO {
        public int Size, Flags;
        public IntPtr Active, Focus, Capture, MenuOwner, MoveSize, Caret;
        public RECT CaretRect;
    }
    private delegate bool EnumWindowProc(IntPtr hwnd, IntPtr lparam);
    private delegate bool EnumMonitorProc(IntPtr monitor, IntPtr hdc, IntPtr rect, IntPtr lparam);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumWindowProc callback, IntPtr lparam);
    [DllImport("user32.dll")] private static extern bool EnumChildWindows(IntPtr parent, EnumWindowProc callback, IntPtr lparam);
    [DllImport("user32.dll")] private static extern bool EnumDisplayMonitors(IntPtr hdc, IntPtr clip, EnumMonitorProc callback, IntPtr lparam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetClassName(IntPtr hwnd, StringBuilder text, int maxCount);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr hwnd, StringBuilder text, int maxCount);
    [DllImport("user32.dll")] public static extern int GetWindowTextLength(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int GetDlgCtrlID(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr hwnd, uint command);
    [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern IntPtr GetLastActivePopup(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool IsZoomed(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool GetWindowPlacement(IntPtr hwnd, ref WINDOWPLACEMENT placement);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] public static extern int GetWindowLong(IntPtr hwnd, int index);
    [DllImport("user32.dll")] public static extern IntPtr MonitorFromWindow(IntPtr hwnd, uint flags);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern bool GetMonitorInfo(IntPtr monitor, ref MONITORINFOEX info);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool GetGUIThreadInfo(uint threadId, ref GUITHREADINFO info);
    [DllImport("user32.dll")] public static extern int GetSystemMetrics(int index);
    [DllImport("user32.dll", SetLastError = true)] public static extern bool ShowWindow(IntPtr hwnd, int command);
    [DllImport("user32.dll", SetLastError = true)] public static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll", SetLastError = true)] public static extern bool BringWindowToTop(IntPtr hwnd);
    [DllImport("user32.dll", SetLastError = true)] public static extern bool PostMessage(IntPtr hwnd, uint message, IntPtr wparam, IntPtr lparam);
    public static IntPtr[] Windows() {
        var result = new List<IntPtr>();
        EnumWindowProc callback = (hwnd, ignored) => { result.Add(hwnd); return true; };
        if (!EnumWindows(callback, IntPtr.Zero)) throw new InvalidOperationException("EnumWindows failed");
        return result.ToArray();
    }
    public static IntPtr[] Children(IntPtr parent) {
        var result = new List<IntPtr>();
        EnumWindowProc callback = (hwnd, ignored) => { result.Add(hwnd); return true; };
        if (!EnumChildWindows(parent, callback, IntPtr.Zero)) throw new InvalidOperationException("EnumChildWindows failed");
        return result.ToArray();
    }
    public static IntPtr[] Monitors() {
        var result = new List<IntPtr>();
        EnumMonitorProc callback = (monitor, hdc, rect, ignored) => { result.Add(monitor); return true; };
        if (!EnumDisplayMonitors(IntPtr.Zero, IntPtr.Zero, callback, IntPtr.Zero)) throw new InvalidOperationException("EnumDisplayMonitors failed");
        return result.ToArray();
    }
}
'@

function Format-Hwnd([IntPtr] $Value) { '0x{0:X}' -f $Value.ToInt64() }
function Convert-Rect($Rect) {
    [ordered]@{ left = $Rect.Left; top = $Rect.Top; right = $Rect.Right; bottom = $Rect.Bottom;
        width = $Rect.Right - $Rect.Left; height = $Rect.Bottom - $Rect.Top }
}
function Read-Monitors {
    @([CFWWindowNative]::Monitors() | ForEach-Object {
        $info = New-Object CFWWindowNative+MONITORINFOEX
        $info.Size = [Runtime.InteropServices.Marshal]::SizeOf([type][CFWWindowNative+MONITORINFOEX])
        if (-not [CFWWindowNative]::GetMonitorInfo($_, [ref]$info)) { throw 'GetMonitorInfo failed' }
        [ordered]@{ handle = Format-Hwnd $_; device = $info.Device; primary = [bool]($info.Flags -band 1);
            rectangle = Convert-Rect $info.Monitor; workArea = Convert-Rect $info.Work }
    })
}
function Read-DialogControls([IntPtr] $Dialog) {
    @([CFWWindowNative]::Children($Dialog) | ForEach-Object {
        $child = $_
        $class = New-Object System.Text.StringBuilder 256
        $caption = New-Object System.Text.StringBuilder ([Math]::Max(256, [CFWWindowNative]::GetWindowTextLength($child) + 1))
        [void][CFWWindowNative]::GetClassName($child, $class, $class.Capacity)
        [void][CFWWindowNative]::GetWindowText($child, $caption, $caption.Capacity)
        $rect = New-Object CFWWindowNative+RECT
        $hasRect = [CFWWindowNative]::GetWindowRect($child, [ref]$rect)
        [ordered]@{ hwnd = Format-Hwnd $child; className = $class.ToString(); title = $caption.ToString()
            controlId = [CFWWindowNative]::GetDlgCtrlID($child)
            visible = [CFWWindowNative]::IsWindowVisible($child)
            enabled = [CFWWindowNative]::IsWindowEnabled($child)
            windowRect = if ($hasRect) { Convert-Rect $rect } else { $null } }
    })
}
function Take-Snapshot {
    $monitors = @(Read-Monitors)
    $foreground = [CFWWindowNative]::GetForegroundWindow()
    [uint32]$foregroundPid = 0
    if ($foreground -ne [IntPtr]::Zero) { [void][CFWWindowNative]::GetWindowThreadProcessId($foreground, [ref]$foregroundPid) }
    $windows = @([CFWWindowNative]::Windows() | ForEach-Object {
        $handle = $_
        [uint32]$processId = 0
        $threadId = [CFWWindowNative]::GetWindowThreadProcessId($handle, [ref]$processId)
        if ($processId -ne $TargetPid) { return }
        $className = New-Object System.Text.StringBuilder 256
        $title = New-Object System.Text.StringBuilder ([Math]::Max(256, [CFWWindowNative]::GetWindowTextLength($handle) + 1))
        [void][CFWWindowNative]::GetClassName($handle, $className, $className.Capacity)
        [void][CFWWindowNative]::GetWindowText($handle, $title, $title.Capacity)
        $placement = New-Object CFWWindowNative+WINDOWPLACEMENT
        $placement.Length = [Runtime.InteropServices.Marshal]::SizeOf([type][CFWWindowNative+WINDOWPLACEMENT])
        $hasPlacement = [CFWWindowNative]::GetWindowPlacement($handle, [ref]$placement)
        $windowRect = New-Object CFWWindowNative+RECT
        $clientRect = New-Object CFWWindowNative+RECT
        $hasWindowRect = [CFWWindowNative]::GetWindowRect($handle, [ref]$windowRect)
        $hasClientRect = [CFWWindowNative]::GetClientRect($handle, [ref]$clientRect)
        $nearest = [CFWWindowNative]::MonitorFromWindow($handle, 2)
        $nearestInfo = New-Object CFWWindowNative+MONITORINFOEX
        $nearestInfo.Size = [Runtime.InteropServices.Marshal]::SizeOf([type][CFWWindowNative+MONITORINFOEX])
        $nearestDevice = $null
        if ($nearest -ne [IntPtr]::Zero -and [CFWWindowNative]::GetMonitorInfo($nearest, [ref]$nearestInfo)) { $nearestDevice = $nearestInfo.Device }
        $intersects = $false
        if ($hasWindowRect) {
            foreach ($monitor in $monitors) {
                $r = $monitor.rectangle
                if ($windowRect.Right -gt $r.left -and $windowRect.Left -lt $r.right -and
                    $windowRect.Bottom -gt $r.top -and $windowRect.Top -lt $r.bottom) { $intersects = $true; break }
            }
        }
        $gui = New-Object CFWWindowNative+GUITHREADINFO
        $gui.Size = [Runtime.InteropServices.Marshal]::SizeOf([type][CFWWindowNative+GUITHREADINFO])
        $hasGui = [CFWWindowNative]::GetGUIThreadInfo($threadId, [ref]$gui)
        [ordered]@{
            hwnd = Format-Hwnd $handle; pid = $processId; threadId = $threadId
            className = $className.ToString(); title = $title.ToString()
            dialogControls = if ($className.ToString() -eq '#32770') { @(Read-DialogControls $handle) } else { @() }
            ownerHwnd = Format-Hwnd ([CFWWindowNative]::GetWindow($handle, 4))
            parentHwnd = Format-Hwnd ([CFWWindowNative]::GetParent($handle))
            lastActivePopupHwnd = Format-Hwnd ([CFWWindowNative]::GetLastActivePopup($handle))
            visible = [CFWWindowNative]::IsWindowVisible($handle)
            enabled = [CFWWindowNative]::IsWindowEnabled($handle)
            iconic = [CFWWindowNative]::IsIconic($handle)
            zoomed = [CFWWindowNative]::IsZoomed($handle)
            placement = if ($hasPlacement) { [ordered]@{ showCmd = $placement.ShowCmd; flags = $placement.Flags;
                normalRect = Convert-Rect $placement.NormalPosition;
                minPosition = [ordered]@{ x = $placement.MinPosition.X; y = $placement.MinPosition.Y };
                maxPosition = [ordered]@{ x = $placement.MaxPosition.X; y = $placement.MaxPosition.Y } } } else { $null }
            windowRect = if ($hasWindowRect) { Convert-Rect $windowRect } else { $null }
            clientRect = if ($hasClientRect) { Convert-Rect $clientRect } else { $null }
            style = '0x{0:X8}' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes([CFWWindowNative]::GetWindowLong($handle, -16)), 0)
            extendedStyle = '0x{0:X8}' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes([CFWWindowNative]::GetWindowLong($handle, -20)), 0)
            nearestMonitor = $nearestDevice; intersectsMonitor = $intersects
            foreground = $handle -eq $foreground
            activeHwndOnThread = if ($hasGui) { Format-Hwnd $gui.Active } else { $null }
            focusHwndOnThread = if ($hasGui) { Format-Hwnd $gui.Focus } else { $null }
            guiThreadInfoAvailable = $hasGui
        }
    } | Sort-Object { [Convert]::ToInt64($_.hwnd.Substring(2), 16) })
    $owned = @($windows | ForEach-Object { $_.ownerHwnd })
    foreach ($entry in $windows) { $entry['hasOwnedPopup'] = $owned -contains $entry.hwnd }
    [ordered]@{
        capturedUtc = [DateTime]::UtcNow.ToString('o'); targetPid = $TargetPid
        foregroundHwnd = Format-Hwnd $foreground; foregroundPid = $foregroundPid
        desktopBounds = [ordered]@{ x = [CFWWindowNative]::GetSystemMetrics(76); y = [CFWWindowNative]::GetSystemMetrics(77);
            width = [CFWWindowNative]::GetSystemMetrics(78); height = [CFWWindowNative]::GetSystemMetrics(79) }
        monitors = $monitors; windows = $windows
    }
}

try {
    $before = Take-Snapshot
    if (-not $before.windows.Count) { throw "No top-level windows found for PID $TargetPid" }
    $action = $null
    $after = $null
    if ($Operation -ne 'Snapshot') {
        if (-not $Hwnd -or $Hwnd -notmatch '^0x[0-9a-fA-F]+$') { throw '-Hwnd must be an exact hexadecimal target for a diagnostic operation' }
        $chosen = @($before.windows | Where-Object { $_.hwnd -ieq $Hwnd })
        if ($chosen.Count -ne 1) { throw "HWND $Hwnd is not a unique top-level window of PID $TargetPid" }
        $number = [Convert]::ToInt64($Hwnd.Substring(2), 16)
        $pointer = [IntPtr]::new($number)
        $ok = switch ($Operation) {
            'Restore' { [CFWWindowNative]::ShowWindow($pointer, 9) }
            'ShowNormal' { [CFWWindowNative]::ShowWindow($pointer, 1) }
            'Foreground' { [CFWWindowNative]::SetForegroundWindow($pointer) }
            'BringToTop' { [CFWWindowNative]::BringWindowToTop($pointer) }
            'SysRestore' { [CFWWindowNative]::PostMessage($pointer, 0x112, [IntPtr]::new(0xF120), [IntPtr]::Zero) }
        }
        $action = [ordered]@{ operation = $Operation; hwnd = $chosen[0].hwnd; returnValue = [bool]$ok;
            returnMeaning = if ($Operation -in @('Restore', 'ShowNormal')) { 'previouslyVisible, not success' } else { 'API result' };
            win32LastError = [Runtime.InteropServices.Marshal]::GetLastWin32Error() }
        Start-Sleep -Milliseconds 150
        $after = Take-Snapshot
    }
    $result = [ordered]@{ schemaVersion = 2; tool = 'Get-CFWWindowState.ps1'; operation = $Operation;
        before = $before; action = $action; after = $after }
    $json = $result | ConvertTo-Json -Depth 12
    if ($OutputPath) {
        if (Test-Path -LiteralPath $OutputPath) { throw "Refusing to overwrite $OutputPath" }
        [IO.File]::WriteAllText($OutputPath, $json + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
    }
    $json
} catch {
    Write-Error $_
    exit 1
}
