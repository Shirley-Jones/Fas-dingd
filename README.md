# FAS流控|FAS3.0流控|OpenVPN管理面板|筑梦dingd|支持CentOS7、Ubuntu20.04+

## 推荐的服务器配置
* 系统: CentOS7、Ubuntu20+
* CPU/内存：1核512M (512M内存 推荐使用CentOS7系统)
* 带宽：推荐5Mbps以上
* 网络：必须具有固定公网IP (IPV4)


## 更新日志
* 简化了CentOS7脚本的安装代码
* 新增启动器

## 安装脚本
通过安装启动器进行安装
Github
```shell script
wget --no-check-certificate -O fast.sh https://raw.githubusercontent.com/Shirley-Jones/Fas-dingd/master/fast.sh && chmod +x ./fast.sh && ./fast.sh
```
Gitee
```shell script
wget --no-check-certificate -O fast.sh https://gitee.com/JokerPan00/Fas-dingd/raw/master/fast.sh && chmod +x ./fast.sh && ./fast.sh
```

## 编译说明
> CentOS7 
* 先安装支持库: yum install curl libcurl-devel openssl openssl-devel gcc gcc++ gdb -y
* 编译 gcc -o 编译后的文件 源码文件 -lcurl
* 举个栗子 gcc -o fast.bin newfast.c -lcurl
----

> Ubuntu
* 更新列表: apt update
* 先安装支持库: apt install libcurl4-openssl-dev gcc gdb g++ openssl -y
* 编译 gcc -o 编译后的文件 源码文件 -lcurl
* 举个栗子 gcc -o fast.bin newfast.c -lcurl
----

## 常用命令
> 重启流控 vpn restart


## 免责声明
* 脚本写的很辣鸡，还请大佬多多包涵。
* 此脚本由Shirley开源，与筑梦网络科技(筑梦工作室)无关！
* 此版本为最终版本，后续不会对其进行修复或更新。
* 流控版权为筑梦网络科技(筑梦工作室)所有！！
* FAS流控官网: https://www.dingd.cn 已下线

## 关于源码
* 项目基于FAS而来，我个人没有加入任何后门，脚本已全部开源(CentOS7中部分C++文件筑梦网络科技没有开源我也没有)，欢迎检查，不放心的不要用，谢谢！
* 最新Ubuntu脚本中使用的C文件全部都是重新写的C++文件实现FAS所有的功能

## 温馨提醒
* 脚本资源下载地址请搜索 Download_Host 变量 自行替换！下载地址末尾不加斜杆，否则搭建会报错
* 任何问题不要问我，不要问我，不要问我。
* 任何问题不要问我，不要问我，不要问我。
* 任何问题不要问我，不要问我，不要问我。
