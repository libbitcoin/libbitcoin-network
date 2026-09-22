@ECHO OFF
ECHO Downloading libbitcoin vs2026 dependencies from NuGet
CALL nuget.exe install ..\vs2026\libbitcoin-network\packages.config
CALL nuget.exe install ..\vs2026\libbitcoin-network-test\packages.config
