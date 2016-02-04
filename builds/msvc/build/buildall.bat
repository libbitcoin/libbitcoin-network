@ECHO OFF
ECHO.
ECHO Downloading libbitcoin-network dependencies from NuGet
CALL nuget.exe install ..\vs2013\libbitcoin-network\packages.config
CALL nuget.exe install ..\vs2013\libbitcoin-network-test\packages.config
ECHO.
CALL buildbase.bat ..\vs2013\libbitcoin-network.sln 12
ECHO.
PAUSE