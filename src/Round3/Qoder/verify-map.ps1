Add-Type -AssemblyName System.Drawing

$shot = [System.Drawing.Bitmap]::FromFile((Join-Path $PSScriptRoot 'screenshot.png'))

# ---- parse MAP_ONE from settings.h ----
$map = @()
$inMap = $false
foreach ($line in Get-Content (Join-Path $PSScriptRoot 'src\settings.h')) {
    if ($line -match 'MAP_ONE') { $inMap = $true; continue }
    if ($inMap) {
        if ($line -match '^\};') { break }
        if ($line -match '\d') {
            $nums = [regex]::Matches($line, '\d+') | ForEach-Object { [int]$_.Value }
            $map += ,$nums
        }
    }
}
Write-Host ("map parsed: " + $map.Count + " rows x " + $map[0].Count + " cols")

# ---- 1) every non-empty map cell: screenshot center pixel must equal wall texture center pixel ----
$fails = 0
$checked = 0
for ($r = 0; $r -lt $map.Count; $r++) {
    for ($c = 0; $c -lt $map[$r].Count; $c++) {
        $v = $map[$r][$c]
        if ($v -eq 0) { continue }
        $checked++
        $x = $c * 50 + 25; $y = $r * 50 + 25
        $px = $shot.GetPixel($x, $y)
        $tex = [System.Drawing.Bitmap]::FromFile((Join-Path $PSScriptRoot ("resources/images/walls/{0}.png" -f $v)))
        $tp = $tex.GetPixel(25, 25)
        $tex.Dispose()
        if ($px.R -ne $tp.R -or $px.G -ne $tp.G -or $px.B -ne $tp.B) {
            $fails++
            Write-Host ("MISMATCH cell($r,$c)=$v shot=($($px.R),$($px.G),$($px.B)) tex=($($tp.R),$($tp.G),$($tp.B))")
        }
    }
}
Write-Host ("wall cells checked: $checked, mismatches: $fails")

# ---- 2) full-screen scan: cluster pixels near enemy(66,99,3) / hero(249,224,93) ----
# LINEAR sampling blends edge texels, so tolerance matching is used
function Find-Blobs($shot, [int]$targetR, [int]$targetG, [int]$targetB, [int]$tol, [int]$minSize) {
    $w = $shot.Width; $h = $shot.Height
    $mask = New-Object 'bool[,]' $w, $h
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $p = $shot.GetPixel($x, $y)
            $mask[$x, $y] = ([math]::Abs($p.R - $targetR) -le $tol -and [math]::Abs($p.G - $targetG) -le $tol -and [math]::Abs($p.B - $targetB) -le $tol)
        }
    }
    $blobs = @()
    $visited = New-Object 'bool[,]' $w, $h
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            if ($mask[$x, $y] -and -not $visited[$x, $y]) {
                $stack = New-Object System.Collections.Generic.Stack[System.Drawing.Point]
                $stack.Push((New-Object System.Drawing.Point($x, $y)))
                $visited[$x, $y] = $true
                $minX = $x; $maxX = $x; $minY = $y; $maxY = $y; $count = 0
                while ($stack.Count -gt 0) {
                    $pt = $stack.Pop()
                    $count++
                    if ($pt.X -lt $minX) { $minX = $pt.X }; if ($pt.X -gt $maxX) { $maxX = $pt.X }
                    if ($pt.Y -lt $minY) { $minY = $pt.Y }; if ($pt.Y -gt $maxY) { $maxY = $pt.Y }
                    foreach ($d in @(@(1,0),@(-1,0),@(0,1),@(0,-1))) {
                        $nx = $pt.X + $d[0]; $ny = $pt.Y + $d[1]
                        if ($nx -ge 0 -and $nx -lt $w -and $ny -ge 0 -and $ny -lt $h -and $mask[$nx, $ny] -and -not $visited[$nx, $ny]) {
                            $visited[$nx, $ny] = $true
                            $stack.Push((New-Object System.Drawing.Point($nx, $ny)))
                        }
                    }
                }
                if ($count -ge $minSize) {
                    $blobs += ,@{ Count = $count; X1 = $minX; Y1 = $minY; X2 = $maxX; Y2 = $maxY }
                }
            }
        }
    }
    return $blobs
}

Write-Host '--- enemy-colored blobs (66,99,3) tol=10 ---'
$enemyBlobs = Find-Blobs $shot 66 99 3 10 50
$enemyBlobs | ForEach-Object { Write-Host ("  blob px=" + $_.Count + " bbox=(" + $_.X1 + "," + $_.Y1 + ")-(" + $_.X2 + "," + $_.Y2 + ")") }
Write-Host ("enemy blob count: " + $enemyBlobs.Count + " (expect <=5; overlaps merge, grass may cover some)")

Write-Host '--- hero-colored blobs (249,224,93) tol=10 ---'
$heroBlobs = Find-Blobs $shot 249 224 93 10 50
$heroBlobs | ForEach-Object { Write-Host ("  blob px=" + $_.Count + " bbox=(" + $_.X1 + "," + $_.Y1 + ")-(" + $_.X2 + "," + $_.Y2 + ")") }
Write-Host ("hero blob count: " + $heroBlobs.Count + " (expect 1, bbox ~ (355,610)-(395,650))")

$shot.Dispose()
