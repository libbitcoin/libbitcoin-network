@ECHO OFF
ECHO Downloading libbitcoin vs2022 dependencies from NuGet
CALL nuget.exe install ..\vs2022\libbitcoin-network\packages.config
CALL nuget.exe install ..\vs2022\libbitcoin-network-test\packages.config
ECHO.
ECHO Downloading libbitcoin vs2019 dependencies from NuGet
CALL nuget.exe install ..\vs2019\libbitcoin-network\packages.config
CALL nuget.exe install ..\vs2019\libbitcoin-network-test\packages.config
ECHO.
ECHO Downloading libbitcoin vs2017 dependencies from NuGet
CALL nuget.exe install ..\vs2017\libbitcoin-network\packages.config
CALL nuget.exe install ..\vs2017\libbitcoin-network-test\packages.config

