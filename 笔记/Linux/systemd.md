---
title: systemd介绍
categories: Linux
tags:
 - Linux
typora-root-url: ..\..
---

# systemd

## 介绍
### Wiki
`systemd`是一个提供了一系列系统组件的软件集合。其主要是统一`Linux发行版`的服务配置和行为。`systemd`的主要组件是“系统和服务管理”。其同样也提供了许多替换守护进程和工具，其中包括设备管理、登录管理、网络连接管理、事件日志。

作者描述`systemd`开发是"永远不可能完成，但是会紧跟技术进步"。其描述`systemd`通过提供三个主要的功能来"统一各个发行版之间无意义的区别"：

* 一个系统和服务的管理器(不仅通过各种配置来管理系统，还能够管理其服务)
* 一个软件平台（作为开发其他软件的基础）
* 应用程序与内核之间的胶水（提供各种接口来使用内核提供的功能）

`systemd`包含以下特性：守护进程的按需启动、快照支持、进程跟踪、`inhibitor locks`。`systemd`其不止是一个用于初始化的守护进程，还意指与其相关的全部软件，其中包括`journald`，`logind`，`networkd`和其他低等级的组件。`systemd`不是一个程序，而是一整个软件集合。作为一整个软件集合，`systemd`替换通过传统`init daemon`控制的**启动顺序**和**运行级别**，以及在其控制下执行的shell脚本。`systemd`还通过处理用户登录名，系统控制台，设备热插拔，计划执行（替换cron），日志记录，主机名和语言环境，集成了Linux系统上常见的许多其他服务。

像init守护程序一样，`systemd`是管理其他守护程序的守护程序。`systemd`是在引导过程中启动的第一个守护程序，而在关闭过程中终止的最后一个守护程序。 `systemd`守护程序充当用户空间进程树的根。 第一个进程（PID 1）在Unix系统上具有特殊的作用，因为它在原始父进程终止时会替换进程的父进程。 因此，第一个过程特别适合于监视守护程序。

`systemd`并行执行其启动序列的软件，理论上比传统的启动序列方法要快。对于进程间通信（IPC），`systemd`使Unix域套接字和D-Bus可用于正在运行的守护程序。 `systemd`本身的状态也可以保留在快照中，以备将来调用。

> Inhibitor Lock 用于解决如下问题
>
> 由于systemd接管了电源控制，所以其可以实现如下功能
>
> * CD刻录应用程序希望确保在刻录过程中不会关闭或挂起系统。
> * 程序包管理器希望确保在程序包升级过程中不会关闭系统。

### 特点总结

* `systemd`是第一个启动的进程，其在系统中有特殊的地位，主要用于系统配置和启动守护进程
* `systemd`并行启动软件，能够有效解决启动中的相互依赖关系。
* `systemd`还提供了许多其他的功能，并在未来还会不断扩展`systemd`功能，来提供更多的服务。

<img src="/images/blogimage/f074d745-bc99-4d28-acfe-50d29fbeb92d.png" style="zoom:40%;" />



## 使用

对systemd的控制主要是通过systemctl命令来完成初始化系统的目的。初始化系统的基本目的是初始化那些在系统内核启动之后需要被初始化的组件，初始化系统被用来在任意时间节点管理服务和服务的守护进程。systemd中大多数`target`行为被称为`units`，其是`systemd`所了解管理的单元。`units`通过其所代表的资源和定义文件进行分类。每个`unit`可以通过其文件的后缀辨别类型。对于`service`管理任务，其units文件会带有明显的.service后缀。

### 启动和终止service

```shell
# 启动service
systemctl start application.service     # 启动application服务
systemctl start application				# 可以不使用.service后缀

# 终止service
systemctl stop application.servide		# 运行正在运行的
```

### 开机启动

启动和终止service并不能更改系统的启动行为，要想让某个程序在其启动阶段自动启动需要使用`enable`命令

```shell
systemctl enable application.service 	# 设置开机自启
```

这会在`systemd`用于查找自启动文件的目录下创建一个符号链接（通常为/etc/systemd/system/some_target.target.wants），这些符合连接的文件通常为service 文件的副本（通常位于/lib/systemd/system 或者  /etc/systemd/system ）

```shell
systemctl disable application.service   # 取消开机自启
```

此命令会移除符号连接

### 查看service状态

```shell
systemctl status application.service	# 用于查看service状态
```

其结果为

```shell
[heng@localhost system]$ systemctl status sshd.service
● sshd.service - OpenSSH server daemon
   Loaded: loaded (/usr/lib/systemd/system/sshd.service; enabled; vendor preset: enabled)
   Active: active (running) since Wed 2020-12-30 01:45:10 EST; 15min ago
     Docs: man:sshd(8)
           man:sshd_config(5)
 Main PID: 2777 (sshd)
   CGroup: /system.slice/sshd.service
           └─2777 /usr/sbin/sshd -D
```