# ------------------------------------------------------ #
#  Makefile	( NTHU CS MapleBBS Ver 2.36 )		 #
# ------------------------------------------------------ #
#  author : opus.bbs@bbs.cs.nthu.edu.tw		 	 #
#  target : Makefile for MapleBBS main programs		 #
#  create : 95/03/29				 	 #
#  update : 95/12/15					 #
# ------------------------------------------------------ #


# ------------------------------------------------------ #
# 下列的 make rules 不需修改				 #
# ------------------------------------------------------ #


SO =	railway.so bwboard.so dragon.so \
	gray.so km.so guessnum.so liteon.so mine.so \
	pushbox.so recall.so tetris.so \
        bar.so bingo.so bj.so chessmj.so dice.so fantan.so \
        gp.so marie.so nine.so seven.so race.so

# 後兩排需要錢幣系統


.SUFFIXES:
.SUFFIXES: .c .o .so

.c.o:	;  $(CC) $(CFLAGS) -c $*.c
.o.so:	;  gcc -shared $*.o -o $*.so -L../lib -ldao


all: 
	@echo "Please enter 'make sys-type', "
	@echo " make sun     : for Sun-OS 4.x and maybe some BSD systems, cc or gcc" 
	@echo " make linux   : for Linux"
	@echo " make solaris : for Sun-OS 5.x gcc" 
	@echo " make sol-x86 : for Solaris 7 x86"
	@echo " make freebsd : for BSD 4.4 systems"
	@echo " make bsd     : for BSD systems, cc or gcc, if not in the above lists"
	@echo " make cygwin  : for Microsoft Windows and Cygwin gcc"

sun:
	@$(MAKE) CC=gcc CFLAGS="-O2 -pipe -fomit-frame-pointer -fno-stack-protector -Wunused-result -Wunused -I../include -fPIC" $(SO)

linux:
	@$(MAKE) CC=gcc CFLAGS="-DLINUX -O2 -pipe -fomit-frame-pointer -fno-stack-protector -Wunused-result -Wunused -I../include -fPIC" $(SO)

solaris:
	@$(MAKE) CC=gcc CFLAGS="-DSOLARIS -DSYSV -O2 -pipe -fomit-frame-pointer -fno-stack-protector -Wunused-result -Wunused -I../include -fPIC" $(SO)

sol-x86:
	@$(MAKE) CC=gcc CFLAGS="-DSOLARIS -DSYSV -O2 -fomit-frame-pointer -fno-stack-protector -Wunused-result -Wunused -I../include -fPIC" $(SO)

freebsd:
	@$(MAKE) CC=gcc CFLAGS="-DBSD44 -O2 -pipe -fomit-frame-pointer -fno-stack-protector -Wunused-result -Wunused -I../include -fPIC" $(SO)

bsd:
	@$(MAKE) CC=gcc CFLAGS="-DBSD44 -O2 -pipe -fomit-frame-pointer -fno-stack-protector -Wunused-result -Wunused -I../include -fPIC" $(SO)

cygwin:
	@cd ../maple; make cygwin


install: $(SO)
	install -m 0700 $? $(HOME)/bin
	
clean:
	rm -f *.so *.o *~
