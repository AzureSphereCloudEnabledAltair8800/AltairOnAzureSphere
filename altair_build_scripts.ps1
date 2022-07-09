# Project discovery assumes there is a cmake/azsphere_config.cmake folder/file for the high-level and real-time projects to be built 

Write-Output "`n`nBuild all test tool for AzureSphereDevX examples`n`n"

if ($IsWindows) {
    $files = Get-ChildItem "C:\Program Files (x86)" -Recurse -Filter arm-none-eabi-gcc-*.exe -ErrorAction SilentlyContinue | Sort-Object
    if ($files.count -gt 0){
        $gnupath = $files[0].Directory.FullName
    }
    else {
        Write-Output("Error: GNU Arm Embedded Toolchain not found in the C:\Program Files (x86) folder")
        Write-Output("Browse: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads")
        Write-Output("Download: gcc-arm-none-eabi-<version>-win32.exe")
        Write-Output("Install: install the gcc-arm-none-eabi-<version>-win32.exe, accepting the defaults.")
        exit 1
    }
}
else {
    if ($IsLinux) {
        $gnufolder = Get-ChildItem /opt/gcc-arm-none* -Directory
        if ($gnufolder.count -eq 1) {
            $gnupath = Join-Path -Path $gnufolder -ChildPath "bin"
        }
        else {
            Write-Output("Error: GNU Arm Embedded Toolchain not found in the /opt folder")
            Write-Output("Browse: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads")
            Write-Output("Download: gcc-arm-none-eabi-<version>-x86_64-linux.tar.bz2")
            Write-Output("Install: sudo tar -xjvf gcc-arm-none-eabi-<version>-x86_64-linux.tar.bz2 -C /opt")
            Write-Output("Path: nano ~/.bashrc, at end of file add export PATH=`$PATH:/opt/gcc-arm-none-eabi-<version>/bin")
            exit 1            
        }
    }
}

$exit_code = 0

function build-high-level {
    param (
        $dir,
        $config,
        $device,
        $mikroe
    )

    Set-Location $dir
    
    Get-Content -Path $config

    Rename-Item -Path "app_manifest.json" -NewName "app_manifest.json.backup"
    Copy-Item -Path $config -Destination "app_manifest.json"

    Remove-Item -Path .\CMakeLists.txt.backup -ErrorAction SilentlyContinue
    Copy-Item -Path .\CMakeLists.txt -Destination .\CMakeLists.txt.backup

    $c = Get-Content -path .\CMakeLists.txt -Raw
    $c = $c.Replace('set(AVNET ', '# set(AVNET ')
    $c = $c.Replace('# set(' + $device + ' ', 'set(' + $device + ' ')

    if ($mikroe){
        $c = $c.Replace('set(ALTAIR_FRONT_PANEL_NONE ', '# set(ALTAIR_FRONT_PANEL_NONE ')
        $c = $c.Replace('# set(ALTAIR_FRONT_PANEL_RETRO_CLICK ', 'set(ALTAIR_FRONT_PANEL_RETRO_CLICK ')
        $c = $c.Replace('# set(MICRO_SD_CLICK TRUE ', 'set(MICRO_SD_CLICK TRUE ')
    }


    Set-Content -Path .\CMakeLists.txt -Value $c

    New-Item -Name "./build" -ItemType "directory" -ErrorAction SilentlyContinue
    Set-Location "./build"

    if ($IsWindows) {
        cmake `
            -G "Ninja" `
            -DCMAKE_TOOLCHAIN_FILE="C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles\AzureSphereToolchain.cmake" `
            -DAZURE_SPHERE_TARGET_API_SET="latest-lts" `
            -DCMAKE_BUILD_TYPE="Release" `
            ..

        cmake --build . -j 24 --config Release
    }
    else {
        if ($IsLinux) {
            cmake `
                -G "Ninja" `
                -DCMAKE_TOOLCHAIN_FILE="/opt/azurespheresdk/CMakeFiles/AzureSphereToolchain.cmake" `
                -DAZURE_SPHERE_TARGET_API_SET="latest-lts" `
                -DCMAKE_BUILD_TYPE="Release" `
                $dir

            ninja
        }
        else {
            Write-Output "`nERROR: Tool supported on Windows and Linux.`n"
        }
    }

    Set-Location ".."
    Remove-Item -Path "app_manifest.json" -Force
    Rename-Item -Path "app_manifest.json.backup" -NewName "app_manifest.json"

    Remove-Item -Path .\CMakeLists.txt
    Rename-Item -Path .\CMakeLists.txt.backup -NewName .\CMakeLists.txt

    Set-Location "./build"
}

function build-real-time {
    param (
        $dir
    )

    New-Item -Name "./build" -ItemType "directory" -ErrorAction SilentlyContinue
    Set-Location "./build"

    if ($IsWindows) {
        cmake `
            -G "Ninja" `
            -DCMAKE_TOOLCHAIN_FILE="C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles\AzureSphereRTCoreToolchain.cmake" `
            -DARM_GNU_PATH:STRING=$gnupath `
            -DCMAKE_BUILD_TYPE="Release" `
            $dir

        cmake --build .
    }
    else {
        if ($IsLinux) {
            cmake `
                -G "Ninja" `
                -DCMAKE_TOOLCHAIN_FILE="/opt/azurespheresdk/CMakeFiles/AzureSphereRTCoreToolchain.cmake" `
                -DARM_GNU_PATH:STRING=$gnupath  `
                -DCMAKE_BUILD_TYPE="Release" `
                $dir

            ninja
        }
        else {
            Write-Output "`nERROR: Tool supported on Windows and Linux.`n"
        }
    }
}

function build_application {

    param (
        $dir,
        $config,
        $device,
        $image_name,
        $mikroe
    )

    Get-ChildItem

    $manifest = Join-Path -Path $dir -ChildPath "app_manifest.json"
    $cmakefile = Join-Path -Path $dir -ChildPath "CMakeLists.txt"

    if (Test-Path -path $cmakefile) {

        # Only build executable apps not libraries
        if (Select-String -Path $cmakefile -Pattern 'add_executable' -Quiet ) {

            # Is this a high-level app?
            if (Select-String -Path $manifest -Pattern 'Default' -Quiet) {

                build-high-level $dir $config $device $mikroe
            }
            else {
                # Is this a real-time app?
                if (Select-String -Path $manifest -Pattern 'RealTimeCapable' -Quiet) {

                    build-real-time $dir
                }
                else {
                    # Not a high-level or real-time app so just continue
                    continue
                }
            }

            # was the imagepackage created - this is a proxy for sucessful build
            $imagefile = Get-ChildItem "*.imagepackage" -Recurse
            if ($imagefile.count -ne 0) {
                Write-Output "`nSuccessful build: $dir.`n"
                Copy-Item -Path $imagefile[0] -Destination ../../Altair_imagepackages/$image_name.imagepackage
            }
            else {
                Set-Location ".."
                Write-Output "`nBuild failed: $dir.`n"

                $script:exit_code = 1

                break
            }

            Set-Location ".."
            Remove-Item -Recurse -Force -ErrorAction SilentlyContinue -path ./build
            Set-Location ".."
        }
    }
}

$StartTime = $(get-date)

Set-Location "C:\GitHub\Altair8800Emulator"

build_application 'AltairHL_emulator' 'app_manifest.json.avnet.geo_owm_games' 'AVNET' 'emulator_avnet_r1_geo_owm_games'
build_application 'AltairHL_emulator' 'app_manifest.json.avnet.geo_owm_games' 'AVNET_REV_2' 'emulator_avnet_r2_geo_owm_games'
build_application 'AltairHL_emulator' 'app_manifest.json.avnet.no_cloud' 'AVNET' 'emulator_avnet_r1_no_cloud'
build_application 'AltairHL_emulator' 'app_manifest.json.avnet.no_cloud' 'AVNET_REV_2' 'emulator_avnet_r2_no_cloud'

build_application 'AltairHL_emulator' 'app_manifest.json.avnet.geo_owm_games' 'AVNET' 'emulator_avnet_r1_geo_owm_games_sd_retro' 'mikroe'
build_application 'AltairHL_emulator' 'app_manifest.json.avnet.geo_owm_games' 'AVNET_REV_2' 'emulator_avnet_r2_geo_owm_games_sd_retro' 'mikroe'
build_application 'AltairHL_emulator' 'app_manifest.json.avnet.no_cloud' 'AVNET' 'emulator_avnet_r1_no_cloud_sd_retro' 'mikroe'
build_application 'AltairHL_emulator' 'app_manifest.json.avnet.no_cloud' 'AVNET_REV_2' 'emulator_avnet_r2_no_cloud_sd_retro' 'mikroe'

build_application 'AltairHL_emulator' 'app_manifest.json.seeed.geo_owm_games' 'SEEED_STUDIO_RDB' 'emulator_seeed_rdb_geo_owm_games'
build_application 'AltairHL_emulator' 'app_manifest.json.seeed.no_cloud' 'SEEED_STUDIO_RDB' 'emulator_seeed_rdb_no_cloud'
build_application 'AltairHL_emulator' 'app_manifest.json.seeed.geo_owm_games' 'SEEED_STUDIO_MDB' 'emulator_seeed_mdb_geo_owm_games'
build_application 'AltairHL_emulator' 'app_manifest.json.seeed.no_cloud' 'SEEED_STUDIO_MDB' 'emulator_seeed_mdb_no_cloud'

$elapsedTime = $(get-date) - $StartTime
$totalTime = "{0:HH:mm:ss}" -f ([datetime]$elapsedTime.Ticks)

if ($exit_code -eq 0) {
    Write-Output "Build All completed successfully. Elapsed time: $totalTime"
}
else {
    Write-Output "Build All failed. Elapsed time: $totalTime"
}

exit $exit_code
