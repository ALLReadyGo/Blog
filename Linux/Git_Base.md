---
title: Git
categories: 工具
tags:
	- 工具
---



# Git

## Git配置文件

### Git提供配置文件用于使用户控制Git的行为，使得其可以针对相应的配置作出不同的行为。

### 配置类型

- 系统配置

	- /etc/gitconfig
	- git config --system
	- 系统配置为配置的最高等级，如果修改系统配置，那么所有Git仓库的配置也会发生相应的变化（如果用户没有针对此配置进行过特殊配置）。

- 全局配置

	- ~/.gitconfig
	- git config --global
	- 只是针对用户（一个系统中有多个用户）

- 本地配置

	- ./git/config
	- git config
	- 仅针对本地仓库

### Note

- 配置之间属于相互覆盖的关系，层级越低越不容易被覆盖，但是相应的影响范围也越低。

### 常用配置

- 设置用户名称

	- git config --global user.name "Heng"

- 设置用户邮箱

	- git config --global user.email name@163.com

- 设置文本编辑器

	- git config --global core.editor vim

- 查询配置

	- git config --list

- 查询某个配置

	- git config <key>

		- git config usr.name

## Git 命令

### clone

- 克隆一个远程仓库

	- 默认使用克隆仓库的原名称

		- git clone http://github.com/libgit2/libgit2

	- 更换默认名称，使用mygitlib

		- git clone http://github.com/libgit/libgit2 mylibgit

- Git的克隆相当于完全把仓库下载下来，不仅包括当前的工作目录下的内容，还包括历次提交的版本也一同下载下来。也就是你一旦克隆，那么你将获取到完成的仓库内容。
- Git包含很多传输协议，他可以选择https:// ，git://， ssh://这些协议

### git status

- 用于查看当前工作目录下的所有文件状态
- git status
- git status -s
git status --short

	- 用于简略信息查看各个文件状态
	- 状态符号意义

		- ??

			- untrack

		- M[ ] 

			- 被修改了并被存入暂存区

		- [ ]M

			- 被修改了但是还没有存入暂存区

		- A

			- 新添加的文件

### git diff

- 用文件补丁的形式列举出具体哪些地方发生了改变
- git diff

	- 用于比较work space 和 staged之间的差别，这个操作得出的不同是通过比较状态来进行的，而不是记录修改变化。

- git diff --cached
git diff --staged

	- 比较的是staged和last commit之间的区别

### git commit

- 将staged的文件保存到commit集合中
- git commit

	- 将staged文件输出到commit集合中，并不会将工作区的更改一同输入进去
	- 命令执行之后会启动文件编辑器，要求用户输入此次提交的版本信息。文本编译器下方会包含此次修改的简略信息

- git commit -v

	- 文本编辑器中包含详细的修改信息

- git commit -m cotent

	- 直接在命令行中将提示信息输入进取

- git commit -a 

	- 跳过暂存步骤，所有track的文件均会被暂存并且提交，相当于经历了add 和 commit 两个阶段

- git commit --amend

	- 用于修改last commit，而不是将这次提交作为一个单独的commit进行提交。
	- 这条命令执行之后，会将staged的内容修改至last commited。一同修改的还用每次提交都会保留的提交信息。
	- 可以通过这条命令来完善提交信息，或者通过此命令来完善last commit

### git rm

- 将某些文件从staged中删除
- git rm --cached fileName

	- 将文件从staged中删除，但是会保留WorkSpace的文件

- git rm -f fileName

	- 连同WorkSpace的文件一同删除

### git mv file1 file2

- 用户对文件进行移动，或者重命名
- git mv file1 file2相当于

	- mv file1 file2
	- git rm -f file1
	- git add file2

## Git 打标签

### 标签

- Git会对某一提交打上标签，以示其重要性，例如这次提交的版本为一个完整的发布版本。

### 相关命令

- 标签类型

	- 附注标签

		- 附注标签是存储在Git数据库的完整对象。他包含打标签者的名字、电子邮件地址、日期时间和一个标签信息。
		- 创建方法

			- git tag -a v1.4 -m "my version 1.4"

	- 轻量标签

		- 轻量标签实质上是将提交校验和存储在一个文件中，没有存储其他信息。
		- 创建方法

			- git tag v1.4-lw

- 列出标签

	- git tag
	- git tag -l 'v1.8.5*'

		- 使用表达式来查找特定名称的标签

- 后期打标签

	- 一、使用git log 命令查看所有的提交历史
	- 二、使用git log 中每次提交的校验和作为对那次提交的ID，用于指明打标签操作所对应的版本

## 远程仓库的使用

### 显示远程仓库信息

- git remote

	- 查看已经配置的远程仓库服务器

- git remote -v

	- 会显示读写远程仓库使用的Git保留的简写与对应的URL

### 添加远程仓库

- git remote add <shortname> <url>

	- 添加一个新的远程Git仓库，同时指定一个可以轻松引用的简写

### 远程仓库的拉取和推送

- git fetch [remote-name]

	- 这个命令会访问远程仓库，从中拉取所有你还没有的数据。执行完成后，你会拥有那个远程仓库中所有分支的引用，可以随意合并或查看。
	- 如果你克隆了一个仓库，系统会自动将其添加为远程仓库并默认以“origin”命名。
	- git fetch 命令会将数据拉取到你的本地仓库，它并不会自动合并或修改你当前的工作。如果你有一个分支设置为跟踪一个分支，可以使用git pull命令来自动的抓取然后合并远程分支到当前分支。默认情况下，git clone命令会自动设置本地的master分支跟踪远程仓库的master分支。

- git push origin master

	- 当你想要将master分支推送到origin服务器是，运行这个命令可以将其备份到服务器中。

### 查看远程仓库的信息

- git remote show [remote-name]

	- 显示你跟踪了哪些分支，服务器中出现了哪些新的分支，哪些分支已经不在你的本地了

### 远程仓库的移除与重命名

- git remote rename oldName newName

	- 用于对远程仓库重命名

- git remote rm  name

	- 用于删除某一远程仓库

## Git提交历史信息

### Git log

- 用于查看提交历史，输入命令之后会将当前项目的历次提交按照时间显示出来，最后一次提交显示在最前面。
显示的数据有提交时间，提交人的名字、邮箱、提交信息
- 用于控制log的内容和格式

	- Git log -p - 2

		- -p

			- 用于显示两次提交之间的差异

		- -2

			- 显示最近的两次提交

	- Git log --stat(statistics)

		- 用于显示每次提交的统计信息。如哪个文件被添加了，哪个文件被删除了，那个文件增加了几行或者删除了几行

	- git log --pretty= choice

		- choice 是pretty的子选项，这条命令用于控制log的输出日志内容和格式
		- oneline，short，full，fuller，format

	- git log --pretty=format:"%h 
 %s" --graph

		- 用于以图表的方式显示提交历史

- 针对显示commit的筛选

	- -<n>

		- git log -2
		- 用于显示最近两次的提交记录

	- 针对提交时间进行控制

		- --sinc，--after

			- git log --since=2.weeks
			- Subtopic 2

		- --until，--before

	- 仅显示提交者相关的提交

		- --committer

	- 仅显示作者相关的提交

		- --author

	- 仅显示指定关键字之间的提交

		- --grep

	- -S

		- 仅显示添加或移除了某个关键字的提交

## Git分支

### Git 分支对象存储形式

- blob对象

	- blob对象为文件快照，是文件备份存储的实体。Git中将每个文件都分别保存为一个blob对象。

- 树对象

	- 记录着目录结构和blob对象索引。这因为blob对象仅仅保存每个文件对象的快照，针对文件之间的关系并没有保存。树对象在提交时才进行生成，Staged阶段并不会生成树对象。

- 提交对象

	- 包含提交信息，如提交大小、作者、提交者并和树对象的索引

### Git分支

- Git分支仅仅是提交对象的可变指针
- GIt分支的创建仅仅是创建一个可以自由移动的指针指向提交对象
- Git分支只能通过现有分支进行创建
- GIt分支的移动只能通过合并和提交进行
- GIt分支有一个HEAD指针，他用于指向当前工作目录，当HEAD指针的指向发生变化时，用户所对应的工作目录也将发生相应变化，变化为此次分支的最后一次提交。

### Git分支命令

- 分支创建

	- git branch newBranch

		- 在当前分支（HEAD）上创建一个新的分支，新创建的分支与（HEAD）和当前分支共同指向一个提交对象

	- git checkout -b newBranch

		- Git 在当前分支上创建一个新的分支，并切换到新的分支上。

- 分支查看

	- git log --oneline --decorate

		- 

	- git log --oneline --decorate --graph

		- 

- 分支合并

	- git merge mergeBranch

		- 将当前分支与mergeBranch进行合并
		- 合并的过程中会进行三个文件之间的比较，分别是两个分支所对应的提交和两个分支的最近父节点。通过比较三个文件的差异得出最后的合并结果
		- 合并依据：
其实合并相当简单，假设合并的两个分支分别为A，B。这两个分支的最近父节点为P。那么GIt只用比较出D(P, A)，即A相较于P发生的所有新增改变，和D(P, B)。然后将D(P, A) 和 D(P, B)都应用到D上面就行了。
		- 合并之后当前分支会向前移动，mergeBranch分支并不移动，此时应该删除mergerBanch分支，因为这条分支一旦合并就再也没用了。

	- Git 分支冲突解决

		- 如果在合并的两个分支中出现了对文件的不同修改，那么就会引发冲突，当前的工作目录会成为一个冲突解决工作目录
		- git status

			- git status 可以查看到当前存在冲突的文件

		- 冲突的查看

			- 

		- 冲突解决标记

			- 一旦暂存这些原本有冲突的文件，GIt就会将他们标记为冲突已经解决

		- merge提交

			- git commit 会对合并版本进行提交，所以此处可以发现当发生冲突的时候，merge并不会创建一个完整的提交到分支中，而是在工作目录中产生一个冲突解决工作空间。只有当冲突完全解决时才能提交，并且将合并对象提交到分支当中。

- 分支删除

	- git brach -d  branchName

		- 如果这个分支还未进行合并，那么Git会提醒你这个分支还没有进行保存，无法进行删除，并且没有办法删除删除当前分支
		- 如果想要强制删除分支，可以使用git branch -D branchName

- 分支切换

	- git checkout  toBranch

- 分支管理

	- git branch

		- 获得一个当前分支的一个列表

	- git branch -v

		- 查看所有分支，并显示最后一次提交的备注

	- git branch --merged

		- 查看已经合并进入当前分支的分支

	- git branch --no-merged

		- 查看还未合并到当前分支的分支

*XMind - Trial Version*