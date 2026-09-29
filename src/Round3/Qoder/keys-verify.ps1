Add-Type -AssemblyName System.Drawing

$cs = @"
using System;
public static class KProbe {
    // x-range of non-black pixels in a given row
    public static int[] RowRange(byte[] shot, int stride, int w, int h, int y, int x0, int x1) {
        int lo = -1, hi = -1;
        for (int x = x0; x <= x1; x++) {
            int o = y * stride + x * 4;
            if (shot[o] > 20 || shot[o+1] > 20 || shot[o+2] > 20) { if (lo < 0) lo = x; hi = x; }
        }
        return new int[] { lo, hi };
    }
    // lowest non-black pixel y in column x within [y0,y1]
    public static int HeroBottom(byte[] shot, int stride, int w, int h, int x, int y0, int y1) {
        for (int y = y1; y >= y0; y--) {
            int o = y * stride + x * 4;
            if (shot[o] > 20 || shot[o+1] > 20 || shot[o+2] > 20) return y;
        }
        return -1;
    }
    // count pixels within tol of (r,g,b) in rect
    public static int CountColor(byte[] shot, int stride, int w, int h, int x0, int y0, int x1, int y1, int r, int g, int b, int tol) {
        int c = 0;
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                int o = y * stride + x * 4;
                if (Math.Abs(shot[o+2] - r) <= tol && Math.Abs(shot[o+1] - g) <= tol && Math.Abs(shot[o] - b) <= tol) c++;
            }
        return c;
    }
}
"@
Add-Type -TypeDefinition $cs

function Get-Bytes($path) {
    $b = [System.Drawing.Bitmap]::FromFile((Join-Path $PSScriptRoot $path))
    $rect = New-Object System.Drawing.Rectangle(0, 0, $b.Width, $b.Height)
    $d = $b.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bytes = New-Object byte[] ($d.Stride * $d.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($d.Scan0, $bytes, 0, $bytes.Length)
    $b.UnlockBits($d)
    $b.Dispose()
    return ,@($bytes, $d.Stride)
}

foreach ($name in @('shot-K0.png','shot-K1.png','shot-K2.png','shot-K3.png','shot-K4.png')) {
    $path = Join-Path $PSScriptRoot $name
    if (-not (Test-Path $path)) { Write-Host ("=== " + $name + " MISSING ==="); continue }
    $s = Get-Bytes $name
    $bottom = [KProbe]::HeroBottom($s[0], $s[1], 950, 650, 375, 550, 649)
    $r620 = [KProbe]::RowRange($s[0], $s[1], 950, 650, 620, 330, 430)
    $r640 = [KProbe]::RowRange($s[0], $s[1], 950, 650, 640, 330, 430)
    $bulletCount = [KProbe]::CountColor($s[0], $s[1], 950, 650, 340, 540, 410, 648, 196, 196, 196, 12)
    Write-Host ("=== " + $name + ": heroBottom(x=375)=" + $bottom + ", xRange(y620)=[" + $r620[0] + "," + $r620[1] + "], xRange(y640)=[" + $r640[0] + "," + $r640[1] + "], bullet px=" + $bulletCount)
}
