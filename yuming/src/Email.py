#!/usr/bin/env python
# -*- coding: utf-8 -*-


import os
import sys
import logging
import smtplib

from email.mime.text import MIMEText
from email.header import Header

import socket
from datetime import datetime

class Email:
    def __init__(self, server, port,  user, passwd):

        try:
            # set the logger
            self.log_level = logging.DEBUG
            self.log_path = '/var/log/Email.log'

            self.logger = logging.getLogger('Email')
            self.logger.setLevel(self.log_level)

            # create a handler for write the log to file.
            fh = logging.FileHandler(self.log_path)
            fh.setLevel(self.log_level)

            # create a handler for print the log info on console.
            ch = logging.StreamHandler()
            ch.setLevel(self.log_level)

            # set the log format
            formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
            fh.setFormatter(formatter)
            ch.setFormatter(formatter)

            # add the handlers to logger
            self.logger.addHandler(fh)
            self.logger.addHandler(ch)

            # init email server
            self.user = user

            #self.email_server = smtplib.SMTP(server, port)
            self.email_server = smtplib.SMTP_SSL(server, port)
            self.email_server.set_debuglevel(1)

            self.email_server.login(user, passwd)


            self.logger.info('init over.')

        except Exception,ex:
            print "error:%s" %(ex)
            return
 

    def send_mail(self, to_addr, message, subject=u'test'):

        try:
            self.logger.debug('send_mail.')

            message = 'Message:' + message

            msg = MIMEText(message, 'plain', 'utf-8')
            msg['From'] = self.user
            msg['To'] = to_addr
            #msg['Subject'] = u'来自SMTP的问候……'.encode('utf-8')
            msg['Subject'] = subject
            self.email_server.sendmail(self.user, [to_addr], msg.as_string())
        except Exception,ex:
            print "send_mail error:%s" %(ex)

    def recv_mail(self):
        self.logger.debug('recv_mail.')


smtp_server = 'smtp.sina.com'
smtp_port = 465

user_name = 'watchingmans@sina.com'
user_passwd = 'WatchingMan'

email = Email(smtp_server, smtp_port, user_name, user_passwd)


if __name__ == "__main__":

    smtp_server = 'smtp.sina.com'
    smtp_port = 465

    user_name = 'watchingmans@sina.com'
    user_passwd = 'WatchingMan'

    email = Email(smtp_server, smtp_port, user_name, user_passwd)

    email.send_mail("zhangxu@tvie.com.cn", 'zxtest mail  test ', u'测试')
    


