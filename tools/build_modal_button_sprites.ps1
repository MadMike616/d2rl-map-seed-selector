param(
    [string]$SourcePath = (Join-Path $PSScriptRoot '..\resources\modal-button-source.png'),
    [string]$LowEndSourcePath = (Join-Path $PSScriptRoot '..\resources\modal-button-lowend-source.png')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

function Write-ModalButtonSprite([string]$InputPath, [int]$FrameWidth,
        [int]$FrameHeight, [string]$OutputPath) {
    $source = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $InputPath))
    try {
        # The supplied atlas has four button states with one separator pixel
        # between frames. Store the states tightly packed in the SpA1 sheet.
        $expectedWidth = (4 * $FrameWidth) + 3
        if ($source.Width -ne $expectedWidth -or $source.Height -ne $FrameHeight) {
            throw "Expected a ${expectedWidth}x${FrameHeight} atlas for frames ${FrameWidth}x${FrameHeight}: $InputPath"
        }

        $sheetWidth = 4 * $FrameWidth
        $bytes = [byte[]]::new(40 + ($sheetWidth * $FrameHeight * 4))
        [System.Text.Encoding]::ASCII.GetBytes('SpA1').CopyTo($bytes, 0)
        [BitConverter]::GetBytes([uint16]31).CopyTo($bytes, 4)
        [BitConverter]::GetBytes([uint16]$FrameWidth).CopyTo($bytes, 6)
        [BitConverter]::GetBytes([uint32]$sheetWidth).CopyTo($bytes, 8)
        [BitConverter]::GetBytes([uint32]$FrameHeight).CopyTo($bytes, 12)
        [BitConverter]::GetBytes([uint32]4).CopyTo($bytes, 20)

        $offset = 40
        for ($y = 0; $y -lt $FrameHeight; $y++) {
            for ($frameIndex = 0; $frameIndex -lt 4; $frameIndex++) {
                $frameStartX = $frameIndex * ($FrameWidth + 1)
                for ($x = 0; $x -lt $FrameWidth; $x++) {
                    $pixel = $source.GetPixel($frameStartX + $x, $y)
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
        $source.Dispose()
    }
}

$resourceDir = Split-Path -Parent (Resolve-Path -LiteralPath $SourcePath)
Write-ModalButtonSprite $SourcePath 265 70 (Join-Path $resourceDir 'modal-button.sprite')
Write-ModalButtonSprite $LowEndSourcePath 132 35 (Join-Path $resourceDir 'modal-button.lowend.sprite')
