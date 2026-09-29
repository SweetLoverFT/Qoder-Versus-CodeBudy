$ErrorActionPreference = "Stop"
$dir = $PSScriptRoot
Set-Location $dir

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32Drive {
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr lParam);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
  public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
  public static IntPtr FindWindowByPid(uint pid) {
    IntPtr found = IntPtr.Zero;
    EnumWindows((h, l) => {
      uint p; GetWindowThreadProcessId(h, out p);
      if (p == pid) { found = h; return false; }
      return true;
    }, IntPtr.Zero);
    return found;
  }
}
"@

function Send-Key($hwnd, [int]$vk) {
    [Win32Drive]::PostMessage($hwnd, 0x100, [IntPtr]$vk, [IntPtr]0) | Out-Null
    [Win32Drive]::PostMessage($hwnd, 0x101, [IntPtr]$vk, [IntPtr]0) | Out-Null
}

function Grab-Shot($hwnd, [string]$name) {
    [Win32Drive]::PostMessage($hwnd, 0x100, [IntPtr]0x7B, [IntPtr]0) | Out-Null
    [Win32Drive]::PostMessage($hwnd, 0x101, [IntPtr]0x7B, [IntPtr]0) | Out-Null
    Start-Sleep -Milliseconds 400
    Copy-Item "$dir\screenshot.png" "$dir\$name" -Force
    Write-Output ("  saved " + $name)
}

Remove-Item "$dir\shot-A.png","$dir\shot-B.png","$dir\shot-C.png","$dir\screenshot.png" -ErrorAction SilentlyContinue
$p = Start-Process -FilePath "$dir\TankWar.exe" -WorkingDirectory $dir -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 10 -and $hwnd -eq [IntPtr]::Zero -and -not $p.HasExited; $i++) {
  Start-Sleep -Milliseconds 500
  $hwnd = [Win32Drive]::FindWindowByPid($p.Id)
}
if ($p.HasExited) { Write-Output ("EXITED EARLY code=" + $p.ExitCode); exit 2 }
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "WINDOW NOT FOUND"; Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue; exit 1 }
Write-Output ("hwnd=" + $hwnd + " pid=" + $p.Id)

Start-Sleep -Milliseconds 1000
Grab-Shot $hwnd "shot-A.png"          # hero at start position, wall (8,12) intact

# hold RIGHT ~300ms: hero moves right into wall (8,12), ends at x=358 facing RIGHT
[Win32Drive]::PostMessage($hwnd, 0x100, [IntPtr]0x27, [IntPtr]0) | Out-Null
Start-Sleep -Milliseconds 300
[Win32Drive]::PostMessage($hwnd, 0x101, [IntPtr]0x27, [IntPtr]0) | Out-Null
Start-Sleep -Milliseconds 400
Grab-Shot $hwnd "shot-B.png"          # hero at ~(358,610) facing right, wall intact

Send-Key $hwnd 0x20                   # SPACE: shot 1 -> red wall (8,12) life 2->1
Start-Sleep -Milliseconds 300
Send-Key $hwnd 0x20                   # SPACE: shot 2 -> wall destroyed, blast effect starts
Start-Sleep -Milliseconds 300
Grab-Shot $hwnd "shot-C.png"          # wall gone, blast effect mid-animation, hero alive

Send-Key $hwnd 0x20                   # SPACE: shot 3 -> hits boss wall -> game over -> exit
$exited = $false
for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Milliseconds 250
    if ($p.HasExited) { $exited = $true; break }
}
if ($exited) {
    Write-Output ("EXITED code=" + $p.ExitCode + " (expect 0)")
} else {
    Write-Output "STILL RUNNING (FAIL: boss hit did not end game)"
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    exit 3
}
