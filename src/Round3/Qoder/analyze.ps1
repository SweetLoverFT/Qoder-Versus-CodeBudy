Add-Type -AssemblyName System.Drawing
$dir = $PSScriptRoot
$bmp = New-Object System.Drawing.Bitmap("$dir\screenshot.png")
"size=" + $bmp.Width + "x" + $bmp.Height

# ASCII map: every 10px -> W=white R=red .=black #=other
for ($y = 0; $y -lt $bmp.Height; $y += 10) {
  $line = ""
  for ($x = 0; $x -lt $bmp.Width; $x += 10) {
    $c = $bmp.GetPixel($x, $y)
    if ($c.R -gt 240 -and $c.G -gt 240 -and $c.B -gt 240) { $line += "W" }
    elseif ($c.R -gt 200 -and $c.G -lt 80 -and $c.B -lt 80) { $line += "R" }
    elseif ($c.R -eq 0 -and $c.G -eq 0 -and $c.B -eq 0) { $line += "." }
    else { $line += "#" }
  }
  $line
}
