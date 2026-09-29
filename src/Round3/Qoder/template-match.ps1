Add-Type -AssemblyName System.Drawing

$cs = @"
using System;
using System.Collections.Generic;

public static class TMatcher {
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

$shot = [System.Drawing.Bitmap]::FromFile((Join-Path $PSScriptRoot 'screenshot.png'))
$s = Get-Bytes $shot
$shot.Dispose()

Write-Host '--- hero1U best match (region 300..415 x 550..610) ---'
$t = [System.Drawing.Bitmap]::FromFile((Join-Path $PSScriptRoot 'resources/images/hero/hero1U.gif'))
$tb = Get-Bytes $t; $t.Dispose()
$r = [TMatcher]::ScanBest($s[0], $s[1], 300, 550, 415, 610, $tb[0], $tb[1], 40, 40, 6)
Write-Host ("  best: match {0}/{1} at ({2},{3}); expect ~(355,610) with ~100%" -f $r[0], $r[3], $r[1], $r[2])

# full-screen enemy scan, cluster detections within 40px as one tank
$detections = @()
foreach ($d in @('U','D','L','R')) {
    $t = [System.Drawing.Bitmap]::FromFile((Join-Path $PSScriptRoot ("resources/images/enemy/enemy2{0}.gif" -f $d)))
    $tb = Get-Bytes $t; $t.Dispose()
    $opaqueTotal = 0
    for ($i = 3; $i -lt $tb[0].Length; $i += 4) { if ($tb[0][$i] -gt 0) { $opaqueTotal++ } }
    $minM = [int](0.5 * $opaqueTotal)
    $res = [TMatcher]::ScanAll($s[0], $s[1], 0, 0, 910, 610, $tb[0], $tb[1], 40, 40, 6, $minM)
    foreach ($m in $res) { $detections += ,@{ D = $d; Match = $m[0]; Total = $opaqueTotal; X = $m[1]; Y = $m[2] } }
}
Write-Host ("raw detections (>=50%): " + $detections.Count)
$clusters = @()
foreach ($det in ($detections | Sort-Object Match -Descending)) {
    $merged = $false
    foreach ($c in $clusters) {
        if ([math]::Abs($det.X - $c.X) -le 45 -and [math]::Abs($det.Y - $c.Y) -le 45) { $merged = $true; break }
    }
    if (-not $merged) {
        $clusters += ,@{ X = $det.X; Y = $det.Y; D = $det.D; Match = $det.Match; Total = $det.Total }
    }
}
Write-Host ("enemy clusters: " + $clusters.Count + " (expect 5, some may sit under walls/grass)")
$clusters | Sort-Object Y, X | ForEach-Object {
    Write-Host ("  dir=" + $_.D + " match " + $_.Match + "/" + $_.Total + " at (" + $_.X + "," + $_.Y + ")")
}
