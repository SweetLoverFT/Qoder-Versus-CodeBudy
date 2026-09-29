Add-Type -AssemblyName System.Drawing

$cs = @"
using System;
using System.Collections.Generic;

public static class TMatcher2 {
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

function Load-Bytes($path) {
    $b = [System.Drawing.Bitmap]::FromFile((Join-Path $PSScriptRoot $path))
    $r = Get-Bytes $b
    $b.Dispose()
    return $r
}

$wallTex = Load-Bytes 'resources/images/walls/1.png'
$wallCenter = @($wallTex[0][((25 * $wallTex[1]) + 25 * 4) + 2], $wallTex[0][((25 * $wallTex[1]) + 25 * 4) + 1], $wallTex[0][((25 * $wallTex[1]) + 25 * 4) + 0])

foreach ($shotName in @('shot-A.png','shot-B.png','shot-C.png')) {
    Write-Host ("=== " + $shotName + " ===")
    $shot = [System.Drawing.Bitmap]::FromFile((Join-Path $PSScriptRoot $shotName))
    $s = Get-Bytes $shot
    $shot.Dispose()

    # hero: best among hero1U / hero1R in bottom area
    foreach ($d in @('U','R')) {
        $hb = Load-Bytes ("resources/images/hero/hero1{0}.gif" -f $d)
        $hw = $hb[1] / 4; $hh = $hb[0].Length / $hb[1]
        $r = [TMatcher2]::ScanBest($s[0], $s[1], 300, 550, 460, 610, $hb[0], $hb[1], $hw, $hh, 6)
        Write-Host ("  hero1" + $d + ": match " + $r[0] + "/" + $r[3] + " at (" + $r[1] + "," + $r[2] + ")")
    }

    # wall cell (8,12): center pixel (425,625)
    $so = (625 * $s[1]) + 425 * 4
    $px = @($s[0][$so + 2], $s[0][$so + 1], $s[0][$so + 0])
    $isWall = ($px[0] -eq $wallCenter[0] -and $px[1] -eq $wallCenter[1] -and $px[2] -eq $wallCenter[2])
    Write-Host ("  wall(8,12) center: shot=($($px[0]),$($px[1]),$($px[2])) wallTex=($($wallCenter[0]),$($wallCenter[1]),$($wallCenter[2])) intact=" + $isWall)

    # blast effect frames near wall position (effect rect = wall rect (400,600), blast is 40x40)
    $bestBlast = 0; $bestBlastFrame = -1; $bestBlastPos = ""
    foreach ($i in 1..8) {
        $bb = Load-Bytes ("resources/images/boom/blast{0}.gif" -f $i)
        $bw = $bb[1] / 4; $bh = $bb[0].Length / $bb[1]
        $r = [TMatcher2]::ScanBest($s[0], $s[1], 360, 560, 490, 610, $bb[0], $bb[1], $bw, $bh, 20)
        $pct = if ($r[3] -gt 0) { [int](100 * $r[0] / $r[3]) } else { 0 }
        if ($pct -gt $bestBlast) { $bestBlast = $pct; $bestBlastFrame = $i; $bestBlastPos = ("(" + $r[1] + "," + $r[2] + ")") }
    }
    Write-Host ("  blast best frame: blast" + $bestBlastFrame + " at " + $bestBlast + "% " + $bestBlastPos)
}
