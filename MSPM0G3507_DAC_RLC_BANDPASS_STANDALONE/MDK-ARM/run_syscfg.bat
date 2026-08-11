@echo off
set "PROJECT_ROOT=%~dp0.."
set "SYSCFG_CLI=D:\TI\SYSCONFIG\sysconfig_cli.bat"
set "SDK_ROOT=D:\TI\M0_SDK\mspm0_sdk_2_11_00_07"

if not exist "%SYSCFG_CLI%" (
    echo Cannot find SysConfig CLI: %SYSCFG_CLI%
    exit /b 1
)

"%SYSCFG_CLI%" -o "%PROJECT_ROOT%" -s "%SDK_ROOT%\.metadata\product.json" --compiler keil "%PROJECT_ROOT%\adc_spi_timer.syscfg"
