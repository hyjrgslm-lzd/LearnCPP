# 固定本地依赖

C10使用stdexec tag `nvhpc-26.05`，SHA `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`。源码可以位于本目录下的stdexec，或通过`FETCHCONTENT_SOURCE_DIR_STDEXEC`明确传入。构建验证HEAD及include/src的未提交变化；不调用上游CMake，不自动下载rapids-cmake或ICM。

准备依赖是单独动作。已有相同版本可直接复用；需要获取时按固定SHA准备独立checkout，勿使用main代替。第三方源码及构建产物留在本机，不作为课程正文提交。上游许可见checkout的LICENSE.txt；课程中改编的源码另保留来源和许可说明。

GPU分支使用nvexec，需要实际兼容的nvc++及运行时。NVCC不是该编译器的替代。Linux I/O复用C07的固定liburing2.15隔离前缀，不能静默链接另一版本。
