# vscode 使用 glog 和 googletest

先把 googletest 和 glog clone 下来, 然后编译一下 glog, 把他的头文件 logging.h 搞出来.

然后按照 cmakelist 写就好了

## GLOG 的 flag
(http://www.yeolar.com/note/2014/12/20/glog/)[http://www.yeolar.com/note/2014/12/20/glog/]
常用flag有
* logtostderr （ bool ，默认为 false ） 日志输出到stderr，不输出到日志文件。
* colorlogtostderr （ bool ，默认为 false ） 输出彩色日志到stderr。
* stderrthreshold （ int ，默认为2，即 ERROR ） 将大于等于该级别的日志同时输出到stderr。日志级别 INFO, WARNING, ERROR, FATAL 的值分别为0、1、2、3。
* minloglevel （ int ，默认为0，即 INFO ） 打印大于等于该级别的日志。日志级别的值同上。
* log_dir （ string ，默认为 "" ） 指定输出日志文件的目录。

有两种方式设置这些 flag

* 通过环境变量 `GLOG_logtostderr=1 ./youapp`, 原始flag名字加上 `GLOG_`前缀然后设置值即可
* 代码中设置 `FLAGS_log_dir = "logdir"`, 在 `cpp` 文件中, 原始 flag 名字加上 `FLAGS_` 前缀, 然后设置即可


## GTEST
