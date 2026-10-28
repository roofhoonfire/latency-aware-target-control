@echo off
setlocal
if not defined VCToolsInstallDir call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "LATC_ROOT=%~dp0.."
set "LATC_BUILD=%LATC_ROOT%\build\host-checks"
if not exist "%LATC_BUILD%\common" mkdir "%LATC_BUILD%\common"
pushd "%LATC_BUILD%\common"
cl /nologo /utf-8 /W4 /WX /std:c11 /TC /c /I"%LATC_ROOT%\common\include" "%LATC_ROOT%\common\src\crc16.c" "%LATC_ROOT%\common\src\target_command_codec.c" "%LATC_ROOT%\common\src\uart_frame_codec.c" "%LATC_ROOT%\common\src\uart_frame_validator.c" "%LATC_ROOT%\common\src\steering_controller.c" "%LATC_ROOT%\common\src\sg90_mapper.c"
if errorlevel 1 exit /b 1
lib /nologo /out:latc_common.lib crc16.obj target_command_codec.obj uart_frame_codec.obj uart_frame_validator.obj steering_controller.obj sg90_mapper.obj
if errorlevel 1 exit /b 1
for %%T in (crc16 target_command_codec uart_frame_codec uart_frame_validator steering_controller sg90_mapper runtime_support uart_rx_stream) do (
  cl /nologo /utf-8 /W4 /WX /EHsc /std:c++17 /I"%LATC_ROOT%\common\include" "%LATC_ROOT%\tests\common\%%T_test.cpp" latc_common.lib /Fo:"%LATC_BUILD%\test.obj" /Fe:"%LATC_BUILD%\%%T_test.exe"
  if errorlevel 1 exit /b 1
  "%LATC_BUILD%\%%T_test.exe"
  if errorlevel 1 exit /b 1
)
popd
pushd "%LATC_BUILD%"
cl /nologo /utf-8 /W4 /WX /std:c11 /TC /c /I"%LATC_ROOT%\tests\platform\fakes" /I"%LATC_ROOT%\common\include" "%LATC_ROOT%\firmware\stm32\target_control_stm32\Core\Src\stm32_uart_rx.c"
if errorlevel 1 exit /b 1
cl /nologo /utf-8 /W4 /WX /EHsc /std:c++17 /DTEST_STM32 /I"%LATC_ROOT%\tests\platform\fakes" /I"%LATC_ROOT%\common\include" "%LATC_ROOT%\tests\platform\uart_rx_irq_test.cpp" stm32_uart_rx.obj /Fe:stm32_uart_rx_test.exe
if errorlevel 1 exit /b 1
cl /nologo /utf-8 /W4 /WX /std:c11 /TC /c /I"%LATC_ROOT%\tests\platform\fakes" /I"%LATC_ROOT%\common\include" "%LATC_ROOT%\firmware\zynq\vitis_workspace\zynq_freertos_bringup\src\zynq_uart_rx.c"
if errorlevel 1 exit /b 1
cl /nologo /utf-8 /W4 /WX /EHsc /std:c++17 /I"%LATC_ROOT%\tests\platform\fakes" /I"%LATC_ROOT%\common\include" "%LATC_ROOT%\tests\platform\uart_rx_irq_test.cpp" zynq_uart_rx.obj /Fe:zynq_uart_rx_test.exe
if errorlevel 1 exit /b 1
for %%B in (stm32 zynq) do (
  %%B_uart_rx_test.exe
  if errorlevel 1 exit /b 1
  %%B_uart_rx_test.exe allocation-failure
  if errorlevel 1 exit /b 1
)
popd
pushd "%LATC_ROOT%"
python -m unittest discover -s tests/tools -v
if errorlevel 1 exit /b 1
popd
echo Host checks PASS. ARM builds and hardware IRQ/timing checks are separate.
