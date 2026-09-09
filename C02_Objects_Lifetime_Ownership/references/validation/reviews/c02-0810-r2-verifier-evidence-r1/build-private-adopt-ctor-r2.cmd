@echo off
set "PATH=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin;%PATH%"
clang-cl /nologo /std:c++20 /EHsc /I "F:\CPPTrain\LearnCPP\Core_Study\exercises\L10_control_block\src\reference" /I "F:\CPPTrain\LearnCPP\Core_Study\exercises\L10_control_block\checks\support" "F:\CPPTrain\LearnCPP\Core_Study\references\validation\reviews\c02-0810-r2-verifier-evidence-r1\private-adopt-ctor-r2.cpp" /Fe:"F:\CPPTrain\LearnCPP\Core_Study\references\validation\reviews\c02-0810-r2-verifier-evidence-r1\private-adopt-ctor-r2.exe"
