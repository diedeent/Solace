@echo off
set BUILD_DIR=out\build\windows-vs-debug
ctest --test-dir %BUILD_DIR% --output-on-failure --output-junit %BUILD_DIR%\test_results.xml > %BUILD_DIR%\test_log.txt 2>&1
echo Tests finished. Results: %BUILD_DIR%\test_results.xml
pause