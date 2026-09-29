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

Remove-Item "$dir\screenshot.png","$dir\texture-dump.png","$dir\matrix-dump.txt" -ErrorAction SilentlyContinue
$p = Start-Process -FilePath "$dir\TankWar.exe" -WorkingDirectory $dir -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 10 -and $hwnd -eq [IntPtr]::Zero -and -not $p.HasExited; $i++) {
  Start-Sleep -Milliseconds 500
  $hwnd = [Win32Drive]::FindWindowByPid($p.Id)
}
if ($p.HasExited) {
  Write-Output ("EXITED EARLY code=" + $p.ExitCode)
  exit 2
}
if ($hwnd -eq [IntPtr]::Zero) {
  Write-Output "WINDOW NOT FOUND"
  if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
  exit 1
}
Write-Output ("hwnd=" + $hwnd + " pid=" + $p.Id)
[Win32Drive]::PostMessage($hwnd, 0x100, [IntPtr]0x7B, [IntPtr]0) | Out-Null
[Win32Drive]::PostMessage($hwnd, 0x101, [IntPtr]0x7B, [IntPtr]0) | Out-Null
Start-Sleep -Seconds 2
if ($p.HasExited) {
  Write-Output ("EXITED code=" + $p.ExitCode)
} else {
  Write-Output "STILL RUNNING"
  Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
}
Start-Sleep -Milliseconds 500
Write-Output ("screenshot.png exists: " + (Test-Path "$dir\screenshot.png"))
Write-Output ("texture-dump.png exists: " + (Test-Path "$dir\texture-dump.png"))
