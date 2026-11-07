param(
    [string]$SourcePath = (Join-Path $PSScriptRoot '..\resources\question-button-source.png')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$source = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $SourcePath))
try {
    $frameWidth = 120
    $frameHeight = 120
    $frameCount = 4
    $separatorWidth = 1
    $expectedWidth = ($frameWidth * $frameCount) + ($separatorWidth * ($frameCount - 1))
    if ($source.Width -ne $expectedWidth -or $source.Height -ne $frameHeight) {
        throw "Expected a ${expectedWidth}x${frameHeight} atlas with four ${frameWidth}x${frameHeight} frames."
    }

    $resourceDir = Split-Path -Parent (Resolve-Path -LiteralPath $SourcePath)

    function Write-QuestionButtonSprite([int]$size, [string]$outputPath) {
        $frames = [System.Collections.Generic.List[System.Drawing.Bitmap]]::new()
        try {
            for ($frameIndex = 0; $frameIndex -lt $frameCount; $frameIndex++) {
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
                        $frameIndex * ($frameWidth + $separatorWidth), 0,
                        $frameWidth, $frameHeight)
                    $graphics.DrawImage($source, $destinationRect, $sourceRect,
                        [System.Drawing.GraphicsUnit]::Pixel)
                }
                finally {
                    $graphics.Dispose()
                }
                $frames.Add($frame)
            }

            $sheetWidth = $size * $frameCount
            $bytes = [byte[]]::new(40 + ($sheetWidth * $size * 4))
            [System.Text.Encoding]::ASCII.GetBytes('SpA1').CopyTo($bytes, 0)
            [BitConverter]::GetBytes([uint16]31).CopyTo($bytes, 4)
            [BitConverter]::GetBytes([uint16]$size).CopyTo($bytes, 6)
            [BitConverter]::GetBytes([uint32]$sheetWidth).CopyTo($bytes, 8)
            [BitConverter]::GetBytes([uint32]$size).CopyTo($bytes, 12)
            [BitConverter]::GetBytes([uint32]$frameCount).CopyTo($bytes, 20)

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

    Write-QuestionButtonSprite 56 (Join-Path $resourceDir 'question-button.sprite')
    Write-QuestionButtonSprite 28 (Join-Path $resourceDir 'question-button.lowend.sprite')
}
finally {
    $source.Dispose()
}
