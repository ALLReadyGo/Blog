---
title: RPM和YUM的工作方式
categories: Linux
tags:
 - Linux
typora-root-url: ..\..
---

# RPM和YUM的工作方式

## 软件安装

有两种传统的软件安装方式，一种为源码安装，其需要用户自行编译配置、编译软件。这种安装方式对于非计算机专业人员来说很困难，并不是所有人都可以看懂和解决编译时的错误信息。但是这种方式可以使用户自由设置软件行为，裁剪软件功能，设置软件的安装路径。非常符合极客对自由的追求。

令一种则是将编译之后的软件直接分发给用户，用户将其解压到预先设置的对应目录即可正常工作，这种安装方式操作简单，非常适合非专业人员进行软件安装。针对二进制安装我们需要注意的是二进制程序受很多环境的影响，例如CPU架构中字节大小端问题、操作系统接口调用方式、共享库的链接方式等因素的影响，不同平台下（CPU架构不同、不同Linux发行版）编译的二进制文件有时不能混用。

## RPM

RPM(Redhat Package Manager)，是RedHat公司研制的一个开源工具。其能够实现将rpm打包的软件安装到指定的位置，安装前检查软件包的依赖关系。并且在安装完毕之后将此次安装的软件相关信息记录在数据库中，以便日后的升级、查询和卸载。



总结下来RPM就是一个高级tar + 依赖检查 + 数据库

* 高级tar

  RPM是一个将目录、各种文件打包到一个文件rpm文件当中，这样的功能就是一个tar功能。不同于tar的是，tar默认将解包之后的数据存储到当前的目录，而RPM则是将他们安装到预先定义的位置当中，这个安装位置是在生成RPM包时就指定的，用户无法修改。

  ```sh
  [heng@localhost ~]$ rpm -qpl vim-minimal-7.4.629-8.el7_9.x86_64.rpm  # 查看vim-minimal会进行哪些文件的安装
  /etc/virc
  /usr/bin/ex
  /usr/bin/rvi
  /usr/bin/rview
  /usr/bin/vi
  /usr/bin/view
  /usr/share/man/man1/ex.1.gz
  /usr/share/man/man1/rvi.1.gz
  /usr/share/man/man1/rview.1.gz
  /usr/share/man/man1/vi.1.gz
  /usr/share/man/man1/view.1.gz
  /usr/share/man/man1/vim.1.gz
  /usr/share/man/man5/virc.5.gz
  ```

* 依赖检查

  RPM包内部包含软件包的依赖、安装当前软件可能依赖的软件或需要的动态库。我们可以看到其需要`libacl.so.1()(64bit)`这个动态库。此时我们可以通过某些第三方网站的包分析报告(http://www.rpmfind.net/linux/rpm2html/search.php)，来查询哪些包提供这些链接库。

  ```shell
  [heng@localhost ~]$ rpm -qpR vim-minimal-7.4.629-8.el7_9.x86_64.rpm
  config(vim-minimal) = 2:7.4.629-8.el7_9
  libacl.so.1()(64bit)
  libacl.so.1(ACL_1.0)(64bit)
  libc.so.6()(64bit)
  libc.so.6(GLIBC_2.11)(64bit)
  libc.so.6(GLIBC_2.14)(64bit)
  libc.so.6(GLIBC_2.15)(64bit)
  libc.so.6(GLIBC_2.2.5)(64bit)
  libc.so.6(GLIBC_2.3)(64bit)
  libc.so.6(GLIBC_2.3.4)(64bit)
  libc.so.6(GLIBC_2.4)(64bit)
  libselinux.so.1()(64bit)
  libtinfo.so.5()(64bit)
  rpmlib(CompressedFileNames) <= 3.0.4-1
  rpmlib(FileDigests) <= 4.6.0-1
  rpmlib(PayloadFilesHavePrefix) <= 4.0-1
  rtld(GNU_HASH)
  rpmlib(PayloadIsXz) <= 5.2-1
  ```

  查询到需要下载哪些包提供这些之后我们可以通过手动下载的方式下载这些包。此时我们为了解决`libacl.so.1`的缺失，我们下载`libacl`这个包，此时我们查看libacl包提供的元列表可以看到如下信息。

  ```shell
  [heng@localhost ~]$ rpm -qp --provides libacl-2.2.51-15.el7.x86_64.rpm
  libacl = 2.2.51-15.el7
  libacl(x86-64) = 2.2.51-15.el7
  libacl.so.1()(64bit)
  libacl.so.1(ACL_1.0)(64bit)
  libacl.so.1(ACL_1.1)(64bit)
  libacl.so.1(ACL_1.2)(64bit)
  ```

* 数据库

  数据库中包含当前系统中安装到所有包信息，这样在包安装之前就可以有效检查当前系统环境是否符合包的安装要求。如果不符合则拒绝安装。例如我们的包需要`libacl.so.1()(64bit)`，检查那个包提供了这个动态链接库，如果没有包提供则拒绝此次安装。

  ```shell
  [heng@localhost ~]$ rpm -qa | head
  rsyslog-8.24.0-57.el7_9.x86_64
  yum-plugin-fastestmirror-1.1.31-54.el7_8.noarch
  setup-2.8.71-11.el7.noarch
  tuned-2.11.0-10.el7.noarch
  libseccomp-2.3.1-4.el7.x86_64
  kbd-legacy-1.15.5-15.el7.noarch
  sos-3.9-4.el7.centos.noarch
  fuse-libs-2.9.2-11.el7.x86_64
  ncurses-base-5.9-14.20130511.el7_4.noarch
  kernel-devel-3.10.0-1160.11.1.el7.x86_64
  ```

### RPM包的意义

RPM包解决依赖的关键在于其在rpm包中明确地描述了`requires`和`provides`两个关键信息。RPM安装时可以根据所以已安装包的`provides`和要安装包的`requires`信息来判断其是能否安装。如果不满足要求，我们还可以根据`requires`信息来查找哪个包提供我们需要的东西，并进行安装。

## YUM

yum(Yellowdog Updater Modified)是一个网络安装软件包的工具。其能够从YUM源服务器下载你需要的rpm包，解析其依赖并下载相关的依赖包。避免了RPM管理中需要自己查找依赖相关包并自己找资源下载的尴尬局面。

YUM的基本原理如下图所示：

![](/images/blogimage/A7B876CB-24FF-4f4b-AE46-BC461AF7CC42.jpg)

![](/images/blogimage/D28CB8AC-11C6-4c3b-8328-4701A8391FA5.jpg)

![](/images/blogimage/1757E865-1FE2-4174-B244-C390084672BD.jpg)

软件相关信息的列表记录在yum服务器上，其实yum服务器说白了就是一个文件服务器，并不带有任何的动态交互功能，平常的yum服务器只用Nginx便可以搭建完成。YUM服务器上保存的主要是两部分关键文件。1、RPM包，这是我们需要下载的东西。2、YUM服务器上所有RPM包的相关信息，这部分文件存储在服务器的repodata目录下。我们平常所说的update操作实际上就是重新拉取这部分文件。

> 在不了解这部分内容之前，我一直以为YUM服务器具有交互功能，平常我们需要哪个包时会去主动查询服务器，然后服务器告诉我们这个包的相关信息。之后客户端解析这个包的相关信息，然后告诉服务器我们缺少哪些`requires`,然后服务器告诉我们需要下载哪些文件，并提供下载链接。但是，实质上就是服务器就是个文件服务器。我们自己拉取服务器上的包介绍信息，然后查看相关信息获取包的下载地址，如果缺少依赖，在查找这些依赖的相关信息，下载相应包。

下面这个xml是截取了repodate目录中primary.xml文件的一部分内容。我们可以看到其内部包含了`vim-minimal`包相关的所有信息。如果没有这种xml，文件服务器上只有RPM。这种工作方式极大简化了YUM服务器的设计，只需要一个额外的文件夹就行了，所有的计算全部交由客户端自己完成就行。

```xml
<package type="rpm">
  <name>vim-minimal</name>
  <arch>x86_64</arch>
  <version epoch="2" ver="8.0.1763" rel="15.el8"/>
  <checksum type="sha256" pkgid="YES">3efd6a2548813167fe37718546bc768a5aa8ba59aa80edcecd8ba408bec329b0</checksum>
  <summary>A minimal version of the VIM editor</summary>
  <description>VIM (VIsual editor iMproved) is an updated and improved version of the
vi editor.  Vi was the first real screen-based editor for UNIX, and is
still very popular.  VIM improves on vi by adding new features:
multiple windows, multi-level undo, block highlighting and more. The
vim-minimal package includes a minimal version of VIM, which is
installed into /bin/vi for use when only the root partition is
present. NOTE: The online help is only available when the vim-common
package is installed.</description>
  <packager>CentOS Buildsys &lt;bugs@centos.org&gt;</packager>
  <url>http://www.vim.org/</url>
  <time file="1592496876" build="1592495363"/>
  <size package="586240" installed="1187276" archive="1189520"/>
  <location href="Packages/vim-minimal-8.0.1763-15.el8.x86_64.rpm"/>
  <format>
    <rpm:license>Vim and MIT</rpm:license>
    <rpm:vendor>CentOS</rpm:vendor>
    <rpm:group>Unspecified</rpm:group>
    <rpm:buildhost>x86-02.mbox.centos.org</rpm:buildhost>
    <rpm:sourcerpm>vim-8.0.1763-15.el8.src.rpm</rpm:sourcerpm>
    <rpm:header-range start="5608" end="49832"/>
    <rpm:provides>
      <rpm:entry name="/usr/bin/vi"/>
      <rpm:entry name="config(vim-minimal)" flags="EQ" epoch="2" ver="8.0.1763" rel="15.el8"/>
      <rpm:entry name="vi" flags="EQ" epoch="0" ver="8.0.1763" rel="15.el8"/>
      <rpm:entry name="vim-minimal" flags="EQ" epoch="2" ver="8.0.1763" rel="15.el8"/>
      <rpm:entry name="vim-minimal(x86-64)" flags="EQ" epoch="2" ver="8.0.1763" rel="15.el8"/>
    </rpm:provides>
    <rpm:requires>
      <rpm:entry name="libacl.so.1()(64bit)"/>
      <rpm:entry name="libacl.so.1(ACL_1.0)(64bit)"/>
      <rpm:entry name="libselinux.so.1()(64bit)"/>
      <rpm:entry name="libtinfo.so.6()(64bit)"/>
      <rpm:entry name="rtld(GNU_HASH)"/>
      <rpm:entry name="libc.so.6(GLIBC_2.28)(64bit)"/>
    </rpm:requires>
    <rpm:conflicts>
      <rpm:entry name="vim-common" flags="LT" epoch="0" ver="8.0.1428" rel="4"/>
    </rpm:conflicts>
    <file>/etc/virc</file>
    <file>/usr/bin/ex</file>
    <file>/usr/bin/rvi</file>
    <file>/usr/bin/rview</file>
    <file>/usr/bin/vi</file>
    <file>/usr/bin/view</file>
  </format>
</package>
```

## 依赖冲突问题

### RPM解决方式

rpm这种包管理技术并不能解决依赖冲突的问题。如果存在一个包依赖于某个较低版本的包，而另一个包依赖一个较高版本的包。那么这将导致依赖冲突，因为系统不允许安装两个版本的相同包，冲突一但产生很难解决。

为了解决这个问题，其实有很多办法，YUM的解决办法就是源服务器提供的包的版本尽可能地不与其他包产生冲突，这样在你安装时就可以避免依赖冲突问题。例如包A 2.0版本和包B的2.1版本会产生依赖冲突，但是包A 1.7和 B的 1.8版本却没有依赖冲突。那么源服务器就会选择A 1.7 和 B 1.8作为其YUM源的提供包。所以这也是我们常常看到的为什么某个rpm包在服务器上只有一个版本的原因。较多的版本将导致更为复杂的依赖关系，所以干脆只提供一个版本，并且服务器上对于新版本软件的依赖也不大，其还是稳定性占主导地位。因此对什么包的升级什么的也不是将包升到发布包的最新发布版本，而是把包升级到YUM源的最新版本。除此之外RPM的打包也很重要，如果我们将过多文件打包在一个RPM当中，有可能我们就需要使用其中的某一个文件却需要下载安装整个超大的RPM包，这是非常不科学的，因此RPM的打包应该符合最小功能点的打包原则，按需下载安装才是王道。通过上面的分析我们可以看到一个良好的YUM源是多么重要，他不仅要求包版本之间有较少的依赖冲突还要求包的打包安排的尽可能合理。所以说RPM在单一源下更像是一个git。

### Snap方式

RPM包产生冲突的根本原因就是他们将软件安装到同一个目录下，共享库也安装到同一目录下，这必然会导致冲突。如果我们将软件安装到不同的目录下，所有的依赖库都安装到对应软件的目录下，完全不会造成冲突。不过这会导致另一个问题，那就是重复库安装导致的硬盘膨胀问题。Snap就是这样的一个东西，详细信息可以看下面引用的知乎回答。

> 首先「受到各主流发行版和软件基金会欢迎」这句可是 Ubuntu 的人说给媒体人的，别的发行版都还没表态，见我另一个回答 Flatpak 和 Snap package 技术上有何区别？ 各有何优劣？ 如何看待两者的发展前景？ - fc farseer 的回答。 然后，容器技术的重要性和 Linux 上第三方软件开发商打包困难的问题很多人都提到了，都说得不错，不再复述。 我就提一下为什么 Ubuntu 要做这个，为什么 Ubuntu 要在这个时候大张旗鼓推这个。 在操作系统领域几十年来经久未变的一点是，操作系统本身不重要，重要的是能跑在其上的应用程序，现在的话说是生态环境。 而应用程序不是针对操作系统本身撰写，应用程序是针对操作系统提供的API/SDK撰写，换句话说，掌握了API/SDK的控制权，就掌握了最宝贵的应用程序开发者，操作系统本身就得以长久发展。 这就是为什么 Windows 远比 Mac 卖得好的道理，Windows 掌握着桌面操作系统里最稳定的SDK，几十年来保持兼容性未曾变过，而 Mac 时常破坏 API 兼容性使得老程序不能再跑在新系统上。 这个道理 How Microsoft Lost the API War 这篇文章阐述得非常明白。 GNU/Linux 乃至整个 FOSS 社区，在这一点上，其实非常另类。 GNU 系统从来没有把「保持程序兼容性以吸引用户和开发者」放在首要目标，GNU 的首要目标是「给用户以自由」。 那么 GNU/Linux 的应用程序兼容性不好么？ 并不见得，几十年前的 ed/vi/xterm 程序现在还好好得跑在 各大发行版上，一些程序比 Windows 上的软件还要古老很多。 但是这并不是 GNU/Linux 和各大发行版致力于保护兼容性的结果，而是这些软件「自由」的结果。 因为他们自由且开源，发行版维护者们可以拿他们的源代码重新编译以利用新的软件库新的 ABI ;因为他们自由而且开源，上游维护者可以不断更新他们的代码让他们适应新的技术新的框架新的 API ;因为他们自由而且开源，当上游开发者放弃项目不再开发的时候，还会有有志之士挺身而出接替开发维护的职责。 换句话说，在 Linux 发行版上，软件的兼容性好是软件自由的直接结果。 这就是现在 GNU/Linux 发行版们打包软件发布软件的模式，大家努力的目标是给予用户自由。 这一模式在自由开源软件上非常有效，但是面对闭源软件就不那么有效了。 闭源软件的源代码在开发者手上，没有发行版打包者做衔接工作，所以闭源软件在 GNU/Linux 上发布起来非常困难。 软件的自由，除了干净放心保证隐私外对普通用户来说没有立竿见影的优势，只对软件开发者们有意义，所以GNU/Linux发行版一直是程序员的天堂，用户的地狱。 而 Ubuntu 作为一个发行版，并没有共享传统发行版的自由精神。 从一开始，Ubuntu努力的首要目标就不是给用户自由，而是扩大普通用户的基数。 Ubuntu看到，对普通用户而言，闭源软件尤其是商业软件同等重要甚至可能更重要，普通用户宁愿忍受不自由，宁愿放弃隐私放弃控制权，也不愿使用那些表面粗质功能匮乏的开源替代。 所以 Ubuntu 需要打破传统发行版的发布方式，让商业闭源软件也能在 GNU/Linux 上轻松发布。 而且这条路的可行性早就验证过了。 Google 通过给Linux内核上包装一层 Apache协议的「自由性中立」的 userland 层，禁锢住了 GPL 的病毒传播性，开发出 Android 系统，发展出 Android 之上的生态环境，吸引到了无数开发者为其平台写（大部分闭源）软件。 另一点 Valve 通过 Steam 作为兼容层，附带大量依赖库并保持 API 足够稳定，同时充当游戏开发者和 Steam 兼容层之间的桥梁，也顺利地招揽到不少游戏开发商为 Steam 移植 Linux 平台游戏。 这两个先例都启迪 Ubuntu ，这件事可以做并且可以做好。 并且现在做 Snap 对 Ubuntu 有一个重大的好处，在于垄断 SDK 控制权。 Snap架空了发行版提供的包管理器，甚至架空了发行版本身（提供的依赖库），从而对开发者而言，针对Snap提供软件包就不需要考虑发行版（这是好事）。 如果Snap受到足够多的开发者支持，发展出成熟的生态，那么Ubuntu也就不再发愁今后的推广之路了，因为Ubuntu上的Snap支持必然比别的发行版要好。 目前Snap上发布或者安装软件包需要Ubuntu One身份认证，属于中央化的App Store模式，这给予Ubuntu最直接的控制权（而不是Ubuntu宣称的把控制权从发行版交还给开发者），到时候Ubuntu携应用以令用户，用户并没有选择的权利和自由。 另一点，Ubuntu要做手机系统做IoT系统，面向的用户群就是 Android/iOS 的用户群，这样的用户群下，用自由开源的生态在短期内显然难以抗衡，所以必须引入商业生态，从而提供类似的软件商店也是 Ubuntu 的必由之路。 最开始的时候，Ubuntu 最大的敌人是微软，这是它的 launchpad 上第一个 bug Bug #1 （liberation） â€œMicrosoft has a majority market shareâ€ ： Bugs ： Ubuntu 。 现在 Ubuntu 和微软成为合作关系，然后矛头调转直指一众发行版，司马昭之心可以想象。