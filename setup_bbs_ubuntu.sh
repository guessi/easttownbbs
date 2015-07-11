#!/bin/bash

echo "
--------------------------------------------------------
warning:

* make sure you have changed the password in this script
* current, the script is designed for Ubuntu Server only
* backup and double check before anything

script will start after 10 second, press CTRL-C to abort

--------------------------------------------------------"
sleep 10

# configuration
DIR=`cd $(dirname $0); pwd`
GITSRC="git://github.com/guessi/easttownbbs.git"
BBSUID="9999"
BBSGID="999"
USER="bbs"
GROUP="bbs"
BBSHOME="/home/bbs"
USRSHELL="/bin/bash"
PASSWORD="changeme"
ENCRYPTED=`openssl passwd -1 ${PASSWORD}`

# package setup
echo "apt-get update... "
apt-get update -qq

echo "Preparing for the required packages... "
apt-get install -qqy git wget build-essential csh gcc libc6 libc6-dev make

echo -n "Generating group \"bbs\"... "
grep -q "^${GROUP}" /etc/group
if [ $? -eq 0 ]; then
  echo "Skip, already exist"
else
  groupadd -g ${BBSGID} ${GROUP}
  echo "Done"
fi

echo -n "Checking group \"bbs\" GID... "
TMP=`grep "${USER}" /etc/group | cut -d":" -f3`
if [ ${TMP} != ${BBSGID} ]; then
  echo ${TMP}
  echo -n "Changing group \"bbs\" GID... "
  groupmod -g ${BBSGID} ${GROUP}
  echo "Done"
else
  echo "Pass"
fi

echo -n "Checking user \"bbs\" existence... "
id ${USER} >/dev/null
if [ $? -eq 0 ]; then
  echo "Found"
else
  echo "Not found"
  echo -n "Creating user \"bbs\"... "
  useradd -m -u ${BBSUID} -g ${GROUP} -s ${USRSHELL} -p ${ENCRYPTED} ${USER}
  echo "Done"
fi

echo -n "Checking for source folder's existance... "
if [ -d ${DIR}/easttownbbs ]; then
  mv -f ${DIR}/easttownbbs ${DIR}/easttownbbs-old
fi
echo "Done"

mkdir -p ${BBSHOME}
git clone ${GITSRC}
cp -rf ${DIR}/easttownbbs/bbs/* ${BBSHOME}
sed -i '/^#define BBSGID/s/[ \t]\+99/\t\t999/' ${BBSHOME}/src/include/config.h
chown -R bbs:bbs ${BBSHOME}

echo -n "Setting up for rc.local... "
grep -q "bbsd" /etc/rc.local
if [ $? -eq 0 ]; then
  echo "Skip, already exist"
else
  HOMEBIN="\/home\/bbs\/bin"
  HOMEINN="\/home\/bbs\/innd"

  sed -i "s/^exit 0$/\
${HOMEBIN}\/bbsd\n\
${HOMEBIN}\/bmtad\n\
${HOMEBIN}\/gemd\n\
${HOMEBIN}\/bguard\n\
${HOMEBIN}\/bhttpd\n\
${HOMEBIN}\/bpop3d\n\
${HOMEBIN}\/bnntpd\n\
${HOMEBIN}\/xchatd\n\
${HOMEINN}\/innbbsd\n\n\
su bbs -c \'${HOMEBIN}\/camera\'\n\
su bbs -c \'${HOMEBIN}\/account\'\n\n\
exit 0/g" /etc/rc.local
  echo "Done"
fi

echo -n "Checking for xchat/bbsnntp services existance... "
egrep -q '(xchat|bbsnntp)' /etc/services
if [ $? -eq 0 ]; then
  echo "Skip, already exist"
else
  sed -i '/^telnet[ \t]\+23\/tcp$/a\
xchat\t\t3838\/tcp\
xchat\t\t3838\/udp\
bbsnntp\t\t7777\/tcp\tusenet\
bbsnntp\t\t7777\/udp\tusenet' /etc/services
  echo "Done"
fi

su bbs -c 'cd /home/bbs/src; make clean linux install update'
result=$?

if [ $result -eq 0 ]; then
  echo "now you may reboot your system and check if everything works fine"
else
  echo "oops... something error"
fi
