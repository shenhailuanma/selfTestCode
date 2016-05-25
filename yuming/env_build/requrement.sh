## requirements ##
#  system: centos7
#  python: 2.6 or 2.7

# install requirements
yum -y install sqlite
yum -y install python-setuptools
easy_install pip
pip install flask


# build dir
mkdir -p /usr/local/yuming/data
mkdir -p /usr/local/yuming/config
mkdir -p /usr/local/yuming/src
mkdir -p /usr/local/yuming/etc/cron.d
mkdir -p /usr/local/yuming/etc/logrotate
mkdir -p /usr/local/yuming/var/logs

cp -rf ../src/* /usr/local/yuming/src

