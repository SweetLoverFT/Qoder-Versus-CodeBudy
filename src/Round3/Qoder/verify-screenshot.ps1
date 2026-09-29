$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr lp);
    public delegate bool EnumWindowsProc(IntPtr h, IntPtr lp);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")]
    public static extern int GetWindowTextW(IntPtr h, System.Text.StringBuilder sb, int max);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    public struct RECT { public int L, T, R, B; }
}
"@

$proc = Get-Process TankWar -ErrorAction Stop
$targetPid = [uint32]$proc.Id
$found = [IntPtr]::Zero
$titleSb = $null

$cb = {
    param($h, $lp)
    $pid2 = [uint32]0
    [Win32]::GetWindowThreadProcessId($h, [ref]$pid2) | Out-Null
    if ($pid2 -eq $targetPid -and [Win32]::IsWindowVisible($h)) {
        if ($script:found -eq [IntPtr]::Zero) {
            $sb = New-Object System.Text.StringBuilder 256
            [Win32]::GetWindowTextW($h, $sb, 256) | Out-Null
            $script:titleSb = $sb
            $script:found = $h
        }
    }
    return $true
}
[Win32]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null

if ($found -eq [IntPtr]::Zero) { Write-Error "TankWar window not found"; exit 1 }

# Title as unicode code points (avoid console codepage mojibake)
$cps = ($titleSb.ToString().ToCharArray() | ForEach-Object { "U+{0:X4}" -f [int]$_ }) -join ' '
Write-Output "title codepoints: $cps"

$rc = New-Object Win32+RECT
[Win32]::GetClientRect($found, [ref]$rc) | Out-Null
$w = $rc.R - $rc.L
$ht = $rc.B - $rc.T
Write-Output "client size: ${w}x${ht}"

# PrintWindow with PW_RENDERFULLCONTENT captures the window even if obscured
$bmp = New-Object System.Drawing.Bitmap($w, $ht)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
$ok = [Win32]::PrintWindow($found, $hdc, 2)
$g.ReleaseHdc($hdc)
$g.Dispose()
Write-Output "PrintWindow result: $ok"

$out = Join-Path $PSScriptRoot "verify-shot.png"
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
Write-Output "saved: $out"
