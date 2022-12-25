REM copy_release_static.bat

call Z:\CQ2\Code\Libs\Src\CQ_FullBuild\prebuild_tasks.bat

copy Release\*.dll Z:\CQ2\Code\Libs\ImplicitDLL
copy Release\*.lib Z:\CQ2\Code\Libs\Static

