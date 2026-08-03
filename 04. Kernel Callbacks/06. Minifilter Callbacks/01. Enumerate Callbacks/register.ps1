param(
    [Parameter(Mandatory=$true)]
    [string]$DriverPath
)

# Create the main driver key under Services
$servicePath = "HKLM:\SYSTEM\CurrentControlSet\Services\driver"
New-Item -Path $servicePath -Force

# Set the ImagePath, Type, Start, and ErrorControl values
Set-ItemProperty -Path $servicePath -Name "ImagePath" -Value "\??\$DriverPath"
Set-ItemProperty -Path $servicePath -Name "Type" -Value 0x00000001
Set-ItemProperty -Path $servicePath -Name "Start" -Value 0x00000003
Set-ItemProperty -Path $servicePath -Name "ErrorControl" -Value 0x00000001

# Create the Instances key
$instancesPath = "$servicePath\Instances"
New-Item -Path $instancesPath -Force
Set-ItemProperty -Path $instancesPath -Name "DefaultInstance" -Value "driver Instance"

# Create the driver Instance key
$instancePath = "$instancesPath\driver Instance"
New-Item -Path $instancePath -Force
Set-ItemProperty -Path $instancePath -Name "Altitude" -Value "370000"
Set-ItemProperty -Path $instancePath -Name "Flags" -Value 0x00000000

Write-Host "[+] Registry keys created successfully."
Write-Host "[+] Driver path: \??\$DriverPath"
Write-Host "[+] Run 'fltmc load driver' to load the minifilter."