ABOUT THE PROJECT
-----------------
`MapleBBS-EastTown` is a project based on `MapleBBS-3.10-itoc`, which is written in C Code and some shell scripts

BBS, DEFINITION
---------------
* Please have a look at the following link: [Bulletin Board System(BBS)](http://en.wikipedia.org/wiki/Bulletin_board_system)

LICENSE
-------
* (not decided yet)

AUTHORS
-------
* `MapleBBS-3.10-itoc`: itoc (itoc.bbs@cpu.tfcis.org)
* `MapleBBS-EastTown`: guessi (guessi@gmail.com)
* `(more)`

README
------
* A basic tutorial can be found at `bbs/doc/doc/0readme.htm`

SUGGESTED ENVIRONMENTS
----------------------
* `Ubuntu Server 10.04.4 LTS`
* `Ubuntu Server 12.04.2 LTS`
* `CentOS 6.2 Final`
* `FreeBSD 8.2R`


* for Ubuntu Server users, just execute `setup_bbs_ubuntu.sh`, and it's done!


BEFORE INSTALL
--------------
* You should ALWAYS... BACKUP & RE-CHECK before making changes!!!

PREPARATION
-----------
**for Debian/Ubuntu/Mint:**
> `$ sudo apt-get install git wget build-essential csh gcc libc6 libc6-dev make`

**for CentOS/Fedora/RHEL:**
> `$ su -c 'yum install git wget csh gcc telnet make'`

**for FreeBSD:**
> `(Skip)`


ADD USER/GROUP
--------------

**for FreeBSD:**
> `$ su -`

> `# vipw`
>>     `! bbs:*:9999:99::0:0:BBS Administrator:/home/bbs:/bin/tcsh`

> `# vi /etc/group`
>>     ! bbs:*:99:bbs

> `# mkdir -p /home/bbs`

> `# exit`

**for Linux:**
> `$ sudo su -` or `su -`

> `# vipw`
>>     `! bbs:x:9999:999:BBS Admin:/home/bbs:/bin/bash`

> `# vi /etc/group`
>>     ! bbs:*:999:bbs

> `# mkdir /home/bbs`

> `# exit`

**for ALL:**
> `$ sudo su -` or `su -`

> `# vi /etc/rc.local`
>>       #!/bin/sh
>>
>>     + /home/bbs/bin/bbsd
>>     + /home/bbs/bin/bmtad
>>     + /home/bbs/bin/gemd
>>     + /home/bbs/bin/bguard
>>     + /home/bbs/bin/bhttpd
>>     + /home/bbs/bin/bpop3d
>>     + /home/bbs/bin/bnntpd
>>     + /home/bbs/bin/xchatd
>>     + /home/bbs/innd/innbbsd
>>
>>     + su bbs -c '/home/bbs/bin/camera'
>>     + su bbs -c '/home/bbs/bin/account'
>>       ...
>>       exit 0

> `# vi /etc/services`
>>       ...
>>       telnet 23/tcp
>>     + xchat 3838/tcp
>>     + xchat 3838/udp
>>     + bbsnntp 7777/tcp usenet # Network News Transfer Protocol
>>     + bbsnntp 7777/udp usenet # Network News Transfer Protocol
>>       ...

> `# exit`

GET THE CODE
------------
> `(switch to 'bbs')`

> `$ cd ~ && git clone git://github.com/guessi/easttownbbs.git`

> `$ mv ~/easttownbbs/bbs/* ~ && rm -rf ~/easttownbbs`

> `$ vi ~/src/include/config.h`
>>     ! #define BBSUID 9999
>>     ! #define BBSGID 999

> `(switch to 'root')`

> `# chown -R bbs:bbs ~`

> `(switch to 'bbs')`

> `$ cd ~/src && make clean *ostype* install`

TEST IT
-------
> `(restore your data)`

> `(reboot your server)`

> `$ telnet 127.0.0.1 23`

> `(check if everything works fine, enjoy)`
