param(
    [string]$SourcePath = (Join-Path $PSScriptRoot '..\resources\current-seed-panel-button-source.png')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$source = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $SourcePath))
try {
    $frameWidth = 133
    $frameHeight = 49
    $frameCount = 4
    if ($source.Width -ne ($frameWidth * $frameCount) -or $source.Height -ne $frameHeight) {
        throw "Expected a $($frameWidth * $frameCount)x${frameHeight} atlas with four ${frameWidth}x${frameHeight} frames."
    }

    $resourceDir = Split-Path -Parent (Resolve-Path -LiteralPath $SourcePath)

    function Write-ButtonSprite([int]$Width, [int]$Height, [string]$OutputPath) {
        $frames = [System.Collections.Generic.List[System.Drawing.Bitmap]]::new()
        try {
            for ($frameIndex = 0; $frameIndex -lt $frameCount; $frameIndex++) {
                $frame = [System.Drawing.Bitmap]::new(
                    $Width, $Height,
                    [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
                $graphics = [System.Drawing.Graphics]::FromImage($frame)
                try {
                    $graphics.Clear([System.Drawing.Color]::FromArgb(0, 0, 0, 0))
                    $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
                    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                    $graphics.DrawImage($source,
                        [System.Drawing.Rectangle]::new(0, 0, $Width, $Height),
                        [System.Drawing.Rectangle]::new($frameIndex * $frameWidth, 0, $frameWidth, $frameHeight),
                        [System.Drawing.GraphicsUnit]::Pixel)
                }
                finally {
                    $graphics.Dispose()
                }
                $frames.Add($frame)
            }

            $sheetWidth = $Width * $frameCount
            $bytes = [byte[]]::new(40 + ($sheetWidth * $Height * 4))
            [System.Text.Encoding]::ASCII.GetBytes('SpA1').CopyTo($bytes, 0)
            [BitConverter]::GetBytes([uint16]31).CopyTo($bytes, 4)
            [BitConverter]::GetBytes([uint16]$Width).CopyTo($bytes, 6)
            [BitConverter]::GetBytes([uint32]$sheetWidth).CopyTo($bytes, 8)
            [BitConverter]::GetBytes([uint32]$Height).CopyTo($bytes, 12)
            [BitConverter]::GetBytes([uint32]$frameCount).CopyTo($bytes, 20)

            $offset = 40
            for ($y = 0; $y -lt $Height; $y++) {
                foreach ($frame in $frames) {
                    for ($x = 0; $x -lt $Width; $x++) {
                        $pixel = $frame.GetPixel($x, $y)
                        $bytes[$offset++] = $pixel.R
                        $bytes[$offset++] = $pixel.G
                        $bytes[$offset++] = $pixel.B
                        $bytes[$offset++] = $pixel.A
                    }
                }
            }
            [System.IO.File]::WriteAllBytes($OutputPath, $bytes)
        }
        finally {
            foreach ($frame in $frames) { $frame.Dispose() }
        }
    }

    Write-ButtonSprite $frameWidth $frameHeight `
        (Join-Path $resourceDir 'current-seed-panel-button.sprite')
    $lowWidth = [Math]::Max(1, [int][Math]::Ceiling($frameWidth / 2.0))
    $lowHeight = [Math]::Max(1, [int][Math]::Ceiling($frameHeight / 2.0))
    Write-ButtonSprite $lowWidth $lowHeight `
        (Join-Path $resourceDir 'current-seed-panel-button.lowend.sprite')
}
finally {
    $source.Dispose()
}
