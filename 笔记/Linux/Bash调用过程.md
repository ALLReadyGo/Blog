---
title: Bash启动过程
categories: Linux
tags:
 - Linux
---

## Bash调用过程
下面翻译`man bash`中的部分内容
> 当bash以交互登录shell或伴随--login选项的非交互式的方式进行调用时，其会读取并执行/etc/profile脚本。之后，其会顺序查找~/.bash_profile, ~/.bash_login, ~/.profile，读取并执行匹配到的第一个可读可执行的文件。--noprofile选项可以禁止这种行为
> 当一个交互shell退出或一个非交互式shell执行exit命令时，bash从~/.bash_logout和 /etc/bash.bash_logout读取脚本并执行
> 当一个交互式shell作为非登录shell启动时，bash读取并运行~/.bashrc中的脚本。此行为可以通过--norc去禁止。--rcfile file选项可以让bash从file中读取并执行脚本而不是 ~/.bashrc
> 如果bash以非交互方式启动，去运行一个shell脚本，其会去查找环境变量BASH_ENV，存在时扩展其值并使用扩展值作为文件名去读取并执行。其行为类似于如下脚本`if [ -n "$BASH_ENV" ]; then . "$BASH_ENV; fi"`。其中PATH的值并不会被用于去寻找文件名。
> 如果bash以sh的名称被调用，其会尽可能地模拟历史版本的sh进行启动。如果以登录shell或者非交互shell但是有--login选项，其会顺序执行/etc/profile和~/.profile。--noprofile可以禁止此行为。当作为交互shell时，bash会查找变量ENV，扩展其值并使用扩展值作为文件名称去读取并执行。由于以sh运行的shell并不读取其他的启动文件，所以--norc选项并没有作用。一个以sh运行的非交互式shell并不会读取并执行其他任何启动文件。

理解这些概念和为什么需要执行这些文件可以从系统状态出发考虑
* 交互登录shell
当我们登录一个系统时，操作系统会启动一个shell程序来使我们能够与系统进行交互。之后我们运行的所有程序都是从这个程序fork而来，也就是这个程序的子进程。子进程可以从父进程继承环境变量，但是作为登录shell时，他没有能够继承的shell环境变量，因此此时我们必须在他启动时进行显示指定。
* 交互式非登录shell
非登录意指这个shell程序是由其他登录shell程序启动的，那么这个非登录shell便能够通过进程的环境变量继承操作完成环境变量的初始化，所以他便不必通过profile这种文件进行环境变量初始化。但是shell之间继承的只有环境变量和export function。alias是没办法继承的，但是这个东西又是交互时所必需的。所以我们需要在每次启动交互式shell时都需要调用某个文件去初始化(./bashrc)。
* 非交互式shell
非交互式shell是不需要初始化任何环境的，因为他的调用必定是通过登录shell调用，所以环境变量不必担心，而对于alias的不进行初始化是出于性能考虑。毕竟执行脚本就意味着批处理，而脚本中调用子shell执行程序是再寻常不过的，为了一点点特性而每次都需要执行一个初始化脚本很不划算。但是总归这么说，其还是提供了BASH_ENV这个可选文件，让你能够执行一些你希望的操作。你可以通过这个环境变量来实现在执行脚本之前的环境初始化行为。

通过上面的解释我们也能够明白为什么在`/etc/profile`中初始化环境变量，而alias和function要在`/etc/bashrc`中初始化。他们分开初始化和分别调用是符合逻辑的。
其中为什么function为什么与alias放在一起，function是可以export的。对于这个问题，我的理解是随便export function可能导致子进程环境污染，谁都不希望自己写的脚本因为export function而产生某种错误的行为。例如我们定义了一个叫做yum的函数，我们便无法再对yum应用程序正常调用了。
## CentOS 8 下各启动文件内容
/etc/profile
```shell
# /etc/profile

# System wide environment and startup programs, for login setup
# Functions and aliases go in /etc/bashrc

# It's NOT a good idea to change this file unless you know what you
# are doing. It's much better to create a custom.sh shell script in
# /etc/profile.d/ to make custom changes to your environment, as this
# will prevent the need for merging in future updates.

pathmunge () {
    case ":${PATH}:" in
        *:"$1":*)                           # 如果环境变量存在
            ;;
        *)
            if [ "$2" = "after" ] ; then    # 如果第二个参数为after，那么在末尾插入搜索路径，这样会将其搜索顺序靠后
                PATH=$PATH:$1
            else
                PATH=$1:$PATH               # 在前面插入
            fi
    esac
}


if [ -x /usr/bin/id ]; then             # /usr/bin/id 是一个ELF应用程序，能够获取当前用户id
    if [ -z "$EUID" ]; then             # 如果EUID变量为空值
        # ksh workaround                
        EUID=`/usr/bin/id -u`           # 此处使用` `,等价于$()。 设置EUDI为 `/usr/bin/id -u`的执行结果，也就是当前有效用户ID
        UID=`/usr/bin/id -ru`           # 设置为真实用户ID
    fi
    USER="`/usr/bin/id -un`"            # 设置USER的名称
    LOGNAME=$USER                       # 设置与USER同值
    MAIL="/var/spool/mail/$USER"        # 设置MAIL
fi

# Path manipulation
if [ "$EUID" = "0" ]; then              # 如果用户为ROOT
    pathmunge /usr/sbin                 # 调用函数，向PATH中插入新的搜索路径
    pathmunge /usr/local/sbin
else                                    # 非root用户，搜索路径插入末尾
    pathmunge /usr/local/sbin after     
    pathmunge /usr/sbin after
fi

HOSTNAME=`/usr/bin/hostname 2>/dev/null`            # 设置主机名， 调用程序为 /usr/bin/hostname
HISTSIZE=1000                                       
if [ "$HISTCONTROL" = "ignorespace" ] ; then
    export HISTCONTROL=ignoreboth
else
    export HISTCONTROL=ignoredups
fi

export PATH USER LOGNAME MAIL HOSTNAME HISTSIZE HISTCONTROL         # 将上述变量导出为环境变量

# By default, we want umask to get set. This sets it for login shell
# Current threshold for system reserved uid/gids is 200
# You could check uidgid reservation validity in
# /usr/share/doc/setup-*/uidgid file
if [ $UID -gt 199 ] && [ "`/usr/bin/id -gn`" = "`/usr/bin/id -un`" ]; then  # UID 大于等于 200 且 用户名与组名相同的登录账号 其 umask 为 002
    umask 002
else
    umask 022                                                               # 否则 022
fi

for i in /etc/profile.d/*.sh /etc/profile.d/sh.local ; do  # 遍历/ect/profile.d 目录下的所有以.sh结尾的文件，即脚本文件 和 /etc/profile.d/sh.local文件
    if [ -r "$i" ]; then
        if [ "${-#*i}" != "$-" ]; then                     # 执行脚本文件
            . "$i"
        else
            . "$i" >/dev/null                              # 也是执行脚本文件，只是会将执行的结果重定向到/dev/null 
        fi
    fi
done

unset i
unset -f pathmunge

if [ -n "${BASH_VERSION-}" ] ; then                                     # BASH_VERSION-环境变量设置，需要执行下列操作
        if [ -f /etc/bashrc ] ; then    
                # Bash login shells run only /etc/profile               
                # Bash non-login shells run only /etc/bashrc
                # Check for double sourcing is done in /etc/bashrc.
                . /etc/bashrc                                           # 执行/etc/bashrc 
       fi
fi
```
/etc/bashrc
```cpp
# /etc/bashrc

# System wide functions and aliases
# Environment stuff goes in /etc/profile

# It's NOT a good idea to change this file unless you know what you
# are doing. It's much better to create a custom.sh shell script in
# /etc/profile.d/ to make custom changes to your environment, as this
# will prevent the need for merging in future updates.

# Prevent doublesourcing
if [ -z "$BASHRCSOURCED" ]; then                  # 避免重复调用
  BASHRCSOURCED="Y"

  # are we an interactive shell?
  if [ "$PS1" ]; then                             # SP1 设置表示其为交互shell
    if [ -z "$PROMPT_COMMAND" ]; then             # PROMPT_COMMAND为空值，执行下列命令，这一段代码的作用是根据登录的不同终端，设置不同的提示命令
      case $TERM in                                                                                           # 判断当前登录shell的种类
      xterm*|vte*)                                                                                            # xterm*或者vte*
        if [ -e /etc/sysconfig/bash-prompt-xterm ]; then                                                      # 判断 /etc/sysconfig/bash-prompt-xterm文件是否存在
            PROMPT_COMMAND=/etc/sysconfig/bash-prompt-xterm                                                   # 存在该配置文件
        elif [ "${VTE_VERSION:-0}" -ge 3405 ]; then                                                           
            PROMPT_COMMAND="__vte_prompt_command"                                                             
        else
            PROMPT_COMMAND='printf "\033]0;%s@%s:%s\007" "${USER}" "${HOSTNAME%%.*}" "${PWD/#$HOME/\~}"'      # 写入一句命令， 其会打印 用户名  主机名 和 HOME
        fi
        ;;
      screen*)
        if [ -e /etc/sysconfig/bash-prompt-screen ]; then
            PROMPT_COMMAND=/etc/sysconfig/bash-prompt-screen
        else
            PROMPT_COMMAND='printf "\033k%s@%s:%s\033\\" "${USER}" "${HOSTNAME%%.*}" "${PWD/#$HOME/\~}"'
        fi
        ;;
      *)
        [ -e /etc/sysconfig/bash-prompt-default ] && PROMPT_COMMAND=/etc/sysconfig/bash-prompt-default        # 未知终端
        ;;
      esac
    fi
    # Turn on parallel history                                                      # 设置shell以扩展插入的方式将历史命令写入 HISTFILE当中，而不是overwrite
    shopt -s histappend                                                           
    history -a                                                                    
    # Turn on checkwinsize                                                          # shell在每次执行完命令之后都会检查windows的大小，如果需要其会更新LINES和COLUMNS的值
    shopt -s checkwinsize
    [ "$PS1" = "\\s-\\v\\\$ " ] && PS1="[\u@\h \W]\\$ "                             # 设置SP1
    # You might want to have e.g. tty in prompt (e.g. more virtual machines)
    # and console windows
    # If you want to do so, just add e.g.
    # if [ "$PS1" ]; then
    #   PS1="[\u@\h:\l \W]\\$ "
    # fi
    # to your custom modification shell script in /etc/profile.d/ directory
  fi

  if ! shopt -q login_shell ; then # We're not a login shell                        # 如果不是登录shell
    # Need to redefine pathmunge, it gets undefined at the end of /etc/profile
    pathmunge () {
        case ":${PATH}:" in
            *:"$1":*)
                ;;
            *)
                if [ "$2" = "after" ] ; then
                    PATH=$PATH:$1
                else
                    PATH=$1:$PATH
                fi
        esac
    }

    # By default, we want umask to get set. This sets it for non-login shell.
    # Current threshold for system reserved uid/gids is 200
    # You could check uidgid reservation validity in
    # /usr/share/doc/setup-*/uidgid file
    if [ $UID -gt 199 ] && [ "`/usr/bin/id -gn`" = "`/usr/bin/id -un`" ]; then      # 设置用户的掩码
       umask 002
    else
       umask 022
    fi

    SHELL=/bin/bash                                                                 # SHELL的环境变量
    # Only display echos from profile.d scripts if we are no login shell            
    # and interactive - otherwise just process them to set envvars                  
    for i in /etc/profile.d/*.sh; do                                                # 执行 profile.d下的所有.sh脚本
        if [ -r "$i" ]; then                                                        
            if [ "$PS1" ]; then                                                     # 执行脚本，并能够回显
                . "$i"
            else
                . "$i" >/dev/null                                                   # 执行但是不进行结果回显
            fi
        fi
    done

    unset i
    unset -f pathmunge
  fi

fi
# vim:ts=4:sw=4

```
~/.bash_profile
```shell
# .bash_profile

# Get the aliases and functions
if [ -f ~/.bashrc ]; then							# 运行~/.bashrc脚本
	. ~/.bashrc
fi

# User specific environment and startup programs
Bash_PROFILE=Y
export Bash_PROFILE
```


~/.bashrc
```shell
# .bashrc

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi

# User specific environment
PATH="$HOME/.local/bin:$HOME/bin:$PATH"							# PATH环境变量扩展
export PATH

# Uncomment the following line if you don't like systemctl's auto-paging feature:
# export SYSTEMD_PAGER=

# User specific aliases and functions
```