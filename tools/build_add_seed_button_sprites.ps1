param(
    [string]$SourcePath = (Join-Path $PSScriptRoot '..\resources\add-seed-button-source.png')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$source = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $SourcePath))
try {
    if (($source.Width % 2) -ne 0) {
        throw 'The source image must contain two equal-width button frames.'
    }

    $resourceDir = Split-Path -Parent (Resolve-Path -LiteralPath $SourcePath)
    $sourceFrameWidth = [int]($source.Width / 2)
    $sourceFrameHeight = $source.Height

    function Write-ButtonSprite([int]$size, [string]$outputPath) {
        $frames = [System.Collections.Generic.List[System.Drawing.Bitmap]]::new()
        try {
            # The right frame is the darker idle button; the brighter left
            # frame is used for hover.
            foreach ($frameIndex in @(1, 0)) {
                $frame = [System.Drawing.Bitmap]::new(
                    $size, $size,
                    [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
                $graphics = [System.Drawing.Graphics]::FromImage($frame)
                try {
                    $graphics.Clear([System.Drawing.Color]::FromArgb(0, 0, 0, 0))
                    $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
                    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                    $destinationRect = [System.Drawing.Rectangle]::new(0, 0, $size, $size)
                    $sourceRect = [System.Drawing.Rectangle]::new(
                        $frameIndex * $sourceFrameWidth, 0,
                        $sourceFrameWidth, $sourceFrameHeight)
                    $graphics.DrawImage($source, $destinationRect, $sourceRect,
                        [System.Drawing.GraphicsUnit]::Pixel)
                }
                finally {
                    $graphics.Dispose()
                }
                $frames.Add($frame)
            }

            $bytes = [byte[]]::new(40 + ($size * $size * 4 * $frames.Count))
            [System.Text.Encoding]::ASCII.GetBytes('SpA1').CopyTo($bytes, 0)
            [BitConverter]::GetBytes([uint16]31).CopyTo($bytes, 4)
            [BitConverter]::GetBytes([uint16]$size).CopyTo($bytes, 6)
            [BitConverter]::GetBytes([uint32]($size * $frames.Count)).CopyTo($bytes, 8)
            [BitConverter]::GetBytes([uint32]$size).CopyTo($bytes, 12)
            [BitConverter]::GetBytes([uint32]$frames.Count).CopyTo($bytes, 20)

            $offset = 40
            for ($y = 0; $y -lt $size; $y++) {
                foreach ($frame in $frames) {
                    for ($x = 0; $x -lt $size; $x++) {
                        $pixel = $frame.GetPixel($x, $y)
                        $bytes[$offset++] = $pixel.R
                        $bytes[$offset++] = $pixel.G
                        $bytes[$offset++] = $pixel.B
                        $bytes[$offset++] = $pixel.A
                    }
                }
            }

            [System.IO.File]::WriteAllBytes($outputPath, $bytes)
        }
        finally {
            foreach ($frame in $frames) { $frame.Dispose() }
        }
    }

    Write-ButtonSprite 56 (Join-Path $resourceDir 'add-seed-button.sprite')
    Write-ButtonSprite 28 (Join-Path $resourceDir 'add-seed-button.lowend.sprite')
}
finally {
    $source.Dispose()
}
