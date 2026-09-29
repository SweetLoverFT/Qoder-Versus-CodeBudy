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

function Key($hwnd, [int]$vk, [bool]$down) {
    [Win32Drive]::PostMessage($hwnd, $(if ($down) { 0x100 } else { 0x101 }), [IntPtr]$vk, [IntPtr]0) | Out-Null
}
function Shot($hwnd, [string]$name) {
    Key $hwnd 0x7B $true
    Key $hwnd 0x7B $false
    Start-Sleep -Milliseconds 350
    Copy-Item "$dir\screenshot.png" "$dir\$name" -Force
    Write-Output ("  saved " + $name)
}

foreach ($f in @('shot-K0.png','shot-K1.png','shot-K2.png','shot-K3.png','shot-K4.png','screenshot.png')) {
    Remove-Item "$dir\$f" -ErrorAction SilentlyContinue
}
$p = Start-Process -FilePath "$dir\TankWar.exe" -WorkingDirectory $dir -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 10 -and $hwnd -eq [IntPtr]::Zero -and -not $p.HasExited; $i++) {
  Start-Sleep -Milliseconds 500
  $hwnd = [Win32Drive]::FindWindowByPid($p.Id)
}
if ($p.HasExited) { Write-Output ("EXITED EARLY code=" + $p.ExitCode); exit 2 }
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "WINDOW NOT FOUND"; exit 1 }
Write-Output ("hwnd=" + $hwnd + " pid=" + $p.Id)

Start-Sleep -Milliseconds 1000
Shot $hwnd "shot-K0.png"

Key $hwnd 0x20 $true                 # SPACE down+up
Key $hwnd 0x20 $false
Start-Sleep -Milliseconds 80
Shot $hwnd "shot-K1.png"             # bullet in flight if fired

Key $hwnd 0x27 $true                 # RIGHT down, hold 150ms
Start-Sleep -Milliseconds 150
Shot $hwnd "shot-K2.png"             # hero should be moving right
Key $hwnd 0x27 $false
Start-Sleep -Milliseconds 150
Shot $hwnd "shot-K3.png"             # hero stopped

Key $hwnd 0x26 $true                 # UP down, hold 150ms
Start-Sleep -Milliseconds 150
Shot $hwnd "shot-K4.png"             # hero should have moved up
Key $hwnd 0x26 $false

Start-Sleep -Milliseconds 300
if ($p.HasExited) { Write-Output ("EXITED code=" + $p.ExitCode) } else { Write-Output "STILL RUNNING"; Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
