$ErrorActionPreference = "Stop"
$dir = $PSScriptRoot
Add-Type -AssemblyName System.Drawing

$cs = @"
using System;
using System.Collections.Generic;

public static class BMatcher {
    public static int[] ScanBest(byte[] shot, int sstride, int x0, int y0, int x1, int y1,
                                 byte[] tex, int tstride, int tw, int th, int tol) {
        var offs = new List<int>();
        for (int ty = 0; ty < th; ty++)
            for (int tx = 0; tx < tw; tx++) {
                int o = ty * tstride + tx * 4;
                if (tex[o + 3] > 0) offs.Add(o);
            }
        int total = offs.Count;
        int bestM = -1, bestX = 0, bestY = 0;
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                int m = 0;
                foreach (int o in offs) {
                    int ty = o / tstride;
                    int tx = (o % tstride) / 4;
                    int so = (y + ty) * sstride + (x + tx) * 4;
                    if (Math.Abs(shot[so] - tex[o]) <= tol &&
                        Math.Abs(shot[so + 1] - tex[o + 1]) <= tol &&
                        Math.Abs(shot[so + 2] - tex[o + 2]) <= tol) m++;
                }
                if (m > bestM) { bestM = m; bestX = x; bestY = y; }
            }
        return new int[] { bestM, bestX, bestY, total };
    }

    public static List<int[]> ScanAll(byte[] shot, int sstride, int x0, int y0, int x1, int y1,
                                      byte[] tex, int tstride, int tw, int th, int tol, int minMatch) {
        var offs = new List<int>();
        for (int ty = 0; ty < th; ty++)
            for (int tx = 0; tx < tw; tx++) {
                int o = ty * tstride + tx * 4;
                if (tex[o + 3] > 0) offs.Add(o);
            }
        var res = new List<int[]>();
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                int m = 0;
                foreach (int o in offs) {
                    int ty = o / tstride;
                    int tx = (o % tstride) / 4;
                    int so = (y + ty) * sstride + (x + tx) * 4;
                    if (Math.Abs(shot[so] - tex[o]) <= tol &&
                        Math.Abs(shot[so + 1] - tex[o + 1]) <= tol &&
                        Math.Abs(shot[so + 2] - tex[o + 2]) <= tol) m++;
                }
                if (m >= minMatch) res.Add(new int[] { m, x, y });
            }
        return res;
    }
}
"@
Add-Type -TypeDefinition $cs

function Get-Bytes($bmp) {
    $rect = New-Object System.Drawing.Rectangle(0, 0, $bmp.Width, $bmp.Height)
    $data = $bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bytes = New-Object byte[] ($data.Stride * $data.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
    $bmp.UnlockBits($data)
    return ,@($bytes, $data.Stride)
}

function Get-Pixel($bytes, $stride, $x, $y) {
    $o = $y * $stride + $x * 4
    return "{0},{1},{2}" -f $bytes[$o + 2], $bytes[$o + 1], $bytes[$o]
}

# wall texture center colors (from resources, at (25,25))
$wallCenter = @{}
foreach ($t in @(1, 2, 3, 5)) {
    $w = [System.Drawing.Bitmap]::FromFile((Join-Path $dir ("resources/images/walls/{0}.png" -f $t)))
    $wb = Get-Bytes $w; $w.Dispose()
    $wallCenter[$t] = Get-Pixel $wb[0] $wb[1] 25 25
}
Write-Host "wall texture centers:"
$wallCenter.GetEnumerator() | Sort-Object Name | ForEach-Object { Write-Host ("  type {0} = {1}" -f $_.Key, $_.Value) }

foreach ($name in @('shot-A.png', 'shot-B.png', 'shot-C.png')) {
    $path = Join-Path $dir $name
    if (-not (Test-Path $path)) { continue }
    Write-Host ""
    Write-Host ("===== " + $name + " =====")
    $bmp = [System.Drawing.Bitmap]::FromFile($path)
    $s = Get-Bytes $bmp; $bmp.Dispose()
    $shotBytes = $s[0]; $sstride = $s[1]

    # 1. hero: match all 4 templates in bottom corridor
    foreach ($d in @('L', 'R', 'U', 'D')) {
        $t = [System.Drawing.Bitmap]::FromFile((Join-Path $dir ("resources/images/hero/hero1{0}.gif" -f $d)))
        $tb = Get-Bytes $t; $t.Dispose()
        $r = [BMatcher]::ScanBest($shotBytes, $sstride, 320, 585, 435, 610, $tb[0], $tb[1], 40, 40, 6)
        Write-Host ("  hero1{0}: match {1}/{2} at ({3},{4})" -f $d, $r[0], $r[3], $r[1], $r[2])
    }

    # 2. wall cells near hero (map row 11-12, col 7-10)
    $cells = @(
        @{ X = 375; Y = 525; Name = "wall(7,10)=0" },
        @{ X = 425; Y = 525; Name = "wall(8,10)=0" },
        @{ X = 375; Y = 575; Name = "wall(7,11)=0" },
        @{ X = 425; Y = 575; Name = "wall(8,11)=1" },
        @{ X = 475; Y = 575; Name = "wall(9,11)=1" },
        @{ X = 525; Y = 575; Name = "wall(10,11)=1" },
        @{ X = 425; Y = 625; Name = "wall(8,12)=1" },
        @{ X = 475; Y = 625; Name = "boss(9,12)=5" },
        @{ X = 525; Y = 625; Name = "wall(10,12)=1" }
    )
    foreach ($c in $cells) {
        $p = Get-Pixel $shotBytes $sstride $c.X $c.Y
        $expect = ""
        if ($c.Name -match '=1') { $expect = $wallCenter[1] }
        elseif ($c.Name -match '=5') { $expect = $wallCenter[5] }
        elseif ($c.Name -match '=3') { $expect = $wallCenter[3] }
        $ok = ""
        if ($expect -ne "") { $ok = if ($p -eq $expect) { " INTACT" } else { " CHANGED" } }
        Write-Host ("  {0}: px=({1}){2}" -f $c.Name, $p, $ok)
    }

    # 3. bullet scan in bottom corridor (any bullet.png opaque pattern)
    $t = [System.Drawing.Bitmap]::FromFile((Join-Path $dir 'resources/images/bullet/bullet.png'))
    $bw = $t.Width; $bh = $t.Height
    $tb = Get-Bytes $t; $t.Dispose()
    $total = 0
    for ($i = 3; $i -lt $tb[0].Length; $i += 4) { if ($tb[0][$i] -gt 0) { $total++ } }
    Write-Host ("  bullet tex: {0}x{1}, opaque={2}" -f $bw, $bh, $total)
    $res = [BMatcher]::ScanAll($shotBytes, $sstride, 320, 500, 540, 640, $tb[0], $tb[1], $bw, $bh, 12, $total)
    Write-Host ("  bullets (need " + $total + "/" + $total + "): " + $res.Count)
    $res | ForEach-Object { Write-Host ("    at ({0},{1})" -f $_[1], $_[2]) }

    # 4. blast check near wall (8,12)
    $blastBest = ""
    foreach ($i in 1..8) {
        $t = [System.Drawing.Bitmap]::FromFile((Join-Path $dir ("resources/images/boom/blast{0}.gif" -f $i)))
        $tb = Get-Bytes $t; $t.Dispose()
        $r = [BMatcher]::ScanBest($shotBytes, $sstride, 390, 590, 450, 610, $tb[0], $tb[1], 40, 40, 8)
        if ($r[0] -gt 0) { $blastBest += (" blast{0}={1}/{2}@({3},{4})" -f $i, $r[0], $r[3], $r[1], $r[2]) }
    }
    if ($blastBest -ne "") { Write-Host ("  blast near (8,12):" + $blastBest) }
    else { Write-Host "  blast near (8,12): none" }
}
