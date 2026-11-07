param(
    [string]$SourcePath = (Join-Path $PSScriptRoot '..\resources\current-seed-panel-source.png')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$source = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $SourcePath))
try {
    if ($source.Width -gt [uint16]::MaxValue -or $source.Height -le 0) {
        throw "The panel image dimensions are not valid for a SpA1 sprite: $($source.Width)x$($source.Height)."
    }

    $resourceDir = Split-Path -Parent (Resolve-Path -LiteralPath $SourcePath)

    function Write-PanelSprite([System.Drawing.Bitmap]$InputBitmap,
            [int]$Width, [int]$Height, [string]$OutputPath) {
        $frame = [System.Drawing.Bitmap]::new(
            $Width, $Height,
            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($frame)
            try {
                $graphics.Clear([System.Drawing.Color]::FromArgb(0, 0, 0, 0))
                $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.DrawImage($InputBitmap,
                    [System.Drawing.Rectangle]::new(0, 0, $Width, $Height),
                    [System.Drawing.Rectangle]::new(0, 0, $InputBitmap.Width, $InputBitmap.Height),
                    [System.Drawing.GraphicsUnit]::Pixel)
            }
            finally {
                $graphics.Dispose()
            }

            $bytes = [byte[]]::new(40 + ($Width * $Height * 4))
            [System.Text.Encoding]::ASCII.GetBytes('SpA1').CopyTo($bytes, 0)
            [BitConverter]::GetBytes([uint16]31).CopyTo($bytes, 4)
            [BitConverter]::GetBytes([uint16]$Width).CopyTo($bytes, 6)
            [BitConverter]::GetBytes([uint32]$Width).CopyTo($bytes, 8)
            [BitConverter]::GetBytes([uint32]$Height).CopyTo($bytes, 12)
            [BitConverter]::GetBytes([uint32]1).CopyTo($bytes, 20)

            $offset = 40
            for ($y = 0; $y -lt $Height; $y++) {
                for ($x = 0; $x -lt $Width; $x++) {
                    $pixel = $frame.GetPixel($x, $y)
                    $bytes[$offset++] = $pixel.R
                    $bytes[$offset++] = $pixel.G
                    $bytes[$offset++] = $pixel.B
                    $bytes[$offset++] = $pixel.A
                }
            }

            [System.IO.File]::WriteAllBytes($OutputPath, $bytes)
        }
        finally {
            $frame.Dispose()
        }
    }

    Write-PanelSprite $source $source.Width $source.Height `
        (Join-Path $resourceDir 'current-seed-panel.sprite')
    $lowWidth = [Math]::Max(1, [int][Math]::Round($source.Width / 2.0))
    $lowHeight = [Math]::Max(1, [int][Math]::Round($source.Height / 2.0))
    Write-PanelSprite $source $lowWidth $lowHeight `
        (Join-Path $resourceDir 'current-seed-panel.lowend.sprite')
}
finally {
    $source.Dispose()
}
