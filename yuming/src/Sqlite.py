#!/usr/bin/env python
# -*- coding: utf-8 -*-

from Log import Logger
from Config import Config

import sqlite3

logger = Logger(log_path=Config.logdir+'/sqlite.log', log_level=Config.loglevel, log_name='Sqlite')

class Sqlite:
    def __init__(self, sqlite_path = Config.sqlite_path):
        # get the logger
        self.logger = logger
        self.logger.debug("logger test ok")

        # init sqlite3
        self.sqlite_path = sqlite_path
        self.logger.debug("sqlite_path:%s" %(self.sqlite_path))

        self.sqlite = sqlite3.connect(self.sqlite_path,check_same_thread = False)

        # create must tables if not exist
        table_yuming = '''CREATE TABLE IF NOT EXISTS yuming ( 
            name    char(16)  PRIMARY KEY NOT NULL,
            state   char(16)  DEFAULT 'unknown',
            registration_date datetime DEFAULT NULL,
            expiration_date datetime DEFAULT NULL,
            delete_date datetime DEFAULT NULL,
            follow  char(16)  DEFAULT 'no',
            blacklist  char(16)  DEFAULT 'no'
            )'''
        self.logger.debug("table_yuming:%s" %(table_yuming))
        self.sqlite.execute(table_yuming)


    def __string_value_list(self,value_list):

        try:
            string_name_list = ''
            string_value_list = ''

            '''
                value_list format:
                [{"name":"port", "value":9090},{"name":"server","value":"10.33.0.123"}]
            '''
            value_cnt = 0
            for value_one in value_list:

                if value_cnt == 0:
                    string_name_list = "{0}".format(value_one['name'])
                    if isinstance(value_one['value'], basestring):
                        string_value_list = "'{0}'".format(value_one['value'])
                    else:
                        string_value_list = "{0}".format(value_one['value'])
                    value_cnt += 1
                else:
                    string_name_list = "{0},{1}".format(string_name_list, value_one['name'])
                    if isinstance(value_one['value'], basestring):
                        string_value_list = "{0},'{1}'".format(string_value_list, value_one['value'])
                    else:
                        string_value_list = "{0}, {1}".format(string_value_list, value_one['value'])

            return 'success',string_name_list,string_value_list
        except Exception,ex:
            self.logger.error("__string_value_list error:%s" %(ex))
            return 'error',str(ex),str(ex)

    def __string_set_list(self,set_list):

        try:
            string_set_list = ''

            '''
                set_list format:
                [{"name":"port", "value":9090},{"name":"server","value":"10.33.0.123"}]
            '''
            set_cnt = 0
            for set_one in set_list:
                if set_cnt == 0:
                    if isinstance(set_one['value'], basestring):
                        string_set_list = "{0}='{1}'".format(set_one['name'],set_one['value'])
                    else:
                        string_set_list = "{0}={1}".format(set_one['name'],set_one['value'])
                    set_cnt += 1
                else:
                    if isinstance(set_one['value'], basestring):
                        string_set_list = "{0},{1}='{2}'".format(string_set_list, set_one['name'], set_one['value'])
                    else:
                        string_set_list = "{0},{1}={2}".format(string_set_list, set_one['name'],set_one['value'])

            return 'success',string_set_list
        except Exception,ex:
            return 'error',str(ex)

    def __string_where_list(self,where_list):

        try:
            string_where_list = ''

            '''
                where_list format:
                [{"name":"port", "value":9090},{"name":"server","value":"10.33.0.123"}]
            '''
            where_cnt = 0
            for where_one in where_list:
                if where_cnt == 0:
                    if isinstance(where_one['value'], basestring):
                        string_where_list = "{0}='{1}'".format(where_one['name'],where_one['value'])
                    else:
                        string_where_list = "{0}={1}".format(where_one['name'],where_one['value'])
                    where_cnt += 1
                else:
                    if isinstance(where_one['value'], basestring):
                        string_where_list = "{0} and {1}='{2}'".format(string_where_list, where_one['name'],where_one['value'])
                    else:
                        string_where_list = "{0} and {1}={2}".format(string_where_list, where_one['name'],where_one['value'])

            return 'success',string_where_list
        except Exception,ex:
            return 'error',str(ex)


    def __add_one_name_and_value(self, inlist, name, value):
        one_in = {}
        one_in['name']  = name
        one_in['value'] = value
        inlist.append(one_in)


    def db_insert_common(self, table_name, value_list):
        result = 'error'

        try:

            result, name_list_string, value_list_string = self.__string_value_list(value_list)
            if result != 'success':
                self.logger.error("db_insert_common, __string_where_list error:%s" %name_list_string)
                return result,name_list_string  


            conn = self.sqlite
            
            sql = "INSERT INTO {0}({1})  VALUES({2});".format(table_name, name_list_string, value_list_string)

            self.logger.debug("[db_insert_common]%s" %(sql));
            conn.execute(sql);
            conn.commit();

            result = 'success'

            return result
        except Exception,ex:
            self.logger.error("db_insert_common, error:%s" %(ex))
            return result       

    def db_update_common(self, table_name, set_list, where_list):
        result = 'error'

        try:

            result,set_list_string = self.__string_set_list(set_list)
            if result != 'success':
                self.logger.error("mauna_update_common, __string_set_list error:%s" %set_list_string)
                return result,set_list_string    


            result,where_list_string = self.__string_where_list(where_list)
            if result != 'success':
                self.logger.error("mauna_update_common, __string_where_list error:%s" %where_list_string)
                return result,where_list_string  

            conn = self.sqlite
            
            sql = "UPDATE {0} SET {1} WHERE {2};".format(table_name, set_list_string, where_list_string)

            self.logger.debug("[mauna_update_common]%s" %(sql));
            conn.execute(sql);
            conn.commit();

            result = 'success'
            return result,'success'
        except Exception,ex:
            msg = "error:%s." %(str(ex))
            self.logger.warning("mauna_update_common, %s" %msg)
            return result,msg   

    def insert_one_yuming_info(self, info):
        try:
            '''
                info is a dict that contain one yuming info, the format should be:
                {
                    "name" : "baidu.com",
                    "state" : "used",
                    "registration_date" : "1999-10-11",
                    "expiration_date" : "2017-10-11",
                    "delete_date" : "2017-12-25"
                }
            '''
            value_list = []

            if info.has_key('name'):
                self.__add_one_name_and_value(value_list, 'name', info['name'])
            else:
                self.logger.error("info error: no must params: name")
                return False

            if info.has_key('state') and info['state'] != None:
                self.__add_one_name_and_value(value_list, 'state', info['state'])
            if info.has_key('registration_date') and info['registration_date'] != None:
                self.__add_one_name_and_value(value_list, 'registration_date', info['registration_date'])
            if info.has_key('expiration_date') and info['expiration_date'] != None:
                self.__add_one_name_and_value(value_list, 'expiration_date', info['expiration_date'])       
            if info.has_key('delete_date') and info['delete_date'] != None:
                self.__add_one_name_and_value(value_list, 'delete_date', info['delete_date'])   

            self.db_insert_common('yuming', value_list)

            return True
        except Exception,ex:
            self.logger.error("insert_one_yuming_info error:%s" %(ex))
            return False


    def get_one_yuming_info(self, name):
        try:
            info = None

            sql = "SELECT name,state,registration_date,expiration_date,delete_date,follow,blacklist FROM yuming WHERE name='{0}';".format(name)
            #self.logger.debug("[get_one_yuming_info]%s" %(sql));
            conn = self.sqlite
            dataset = conn.execute(sql)

            for row in dataset:
                info = {}
                info['name'] = row[0]
                info['state'] = row[1]
                info['registration_date'] = row[2]
                info['expiration_date'] = row[3]
                info['delete_date'] = row[4]
                info['follow'] = row[5]
                info['blacklist'] = row[6]
                #self.logger.debug("[get_one_yuming_info] info:%s" %(info));

            return info
        except Exception,ex:
            self.logger.error("get_one_yuming_info(%s) error:%s" %(name,ex))
            return None


    def get_yuming_infos_by_expiration_date(self, expiration_date):
        try:
            info = None

            sql = "SELECT name,state,registration_date,expiration_date,delete_date FROM yuming WHERE expiration_date<'{0}';".format(expiration_date)
            #self.logger.debug("[get_one_yuming_info]%s" %(sql));
            conn = self.sqlite
            dataset = conn.execute(sql)

            result = []
            for row in dataset:
                info = {}
                info['name'] = row[0]
                #info['state'] = row[1]
                #info['registration_date'] = row[2]
                info['expiration_date'] = row[3]
                info['delete_date'] = row[4]
                result.append(info)

                #self.logger.debug("[get_one_yuming_info] info:%s" %(info));

            return result
        except Exception,ex:
            self.logger.error("get_one_yuming_info(%s) error:%s" %(name,ex))
            return []

    def get_yuming_infos_by_where(self, where):
        try:
            info = None

            sql = "SELECT name,state,registration_date,expiration_date,delete_date FROM yuming WHERE {0};".format(where)
            #self.logger.debug("[get_yuming_infos_by_where]%s" %(sql));
            conn = self.sqlite
            dataset = conn.execute(sql)

            result = []
            for row in dataset:
                info = {}
                info['name'] = row[0]
                info['state'] = row[1]
                info['registration_date'] = row[2]
                info['expiration_date'] = row[3]
                info['delete_date'] = row[4]
                result.append(info)

                #self.logger.debug("[get_yuming_infos_by_where] info:%s" %(info));

            return result
        except Exception,ex:
            self.logger.error("get_one_yuming_info(%s) error:%s" %(name,ex))
            return []

    def get_yuming_infos(self, offset, number):
        try:
            info = None

            sql = "SELECT name,state,registration_date,expiration_date,delete_date FROM yuming LIMIT {0},{1};".format(offset, number)
            #self.logger.debug("[get_one_yuming_info]%s" %(sql));
            conn = self.sqlite
            dataset = conn.execute(sql)

            result = []
            for row in dataset:
                info = {}
                info['name'] = row[0]
                info['state'] = row[1]
                info['registration_date'] = row[2]
                info['expiration_date'] = row[3]
                info['delete_date'] = row[4]
                result.append(info)

                #self.logger.debug("[get_one_yuming_info] info:%s" %(info));

            return result
        except Exception,ex:
            self.logger.error("get_one_yuming_info(%s) error:%s" %(name,ex))
            return []

    def get_available_yuming_infos(self):
        try:
            info = None

            sql = "SELECT name FROM yuming WHERE state='available';"
            conn = self.sqlite
            dataset = conn.execute(sql)

            result = []
            for row in dataset:
                result.append(row[0])

            return result
        except Exception,ex:
            self.logger.error("get_available_yuming_infos error:%s" %(ex))
            return []

    def get_available_yuming_infos_all(self):
        try:
            info = None

            sql = "SELECT name,state,registration_date,expiration_date,delete_date,follow,blacklist FROM yuming WHERE state='available' AND blacklist=='no';"
            conn = self.sqlite
            dataset = conn.execute(sql)

            result = []
            for row in dataset:
                info = {}
                info['name'] = row[0]
                info['state'] = row[1]
                info['registration_date'] = row[2]
                info['expiration_date'] = row[3]
                info['delete_date'] = row[4]
                info['follow'] = row[5]
                info['blacklist'] = row[6]
                result.append(info)

            return result
        except Exception,ex:
            self.logger.error("get_available_yuming_infos_all error:%s" %(ex))
            return []

    def update_yuming_info(self, info):
        try:
            '''
                info is a dict that contain one yuming info, the format should be:
                {
                    "name" : "baidu.com",
                    "state" : "used",
                    "registration_date" : "1999-10-11",
                    "expiration_date" : "2017-10-11",
                    "delete_date" : "2017-12-25",
                    "follow" : "no",
                    "blacklist" : "no"
                }
            '''
            where_list = []
            set_list = []

            if info.has_key('name'):
                self.__add_one_name_and_value(where_list, 'name', info['name'])
            else:
                self.logger.error("info error: no must params: name")
                return False

            if info.has_key('state') and info['state'] != None:
                self.__add_one_name_and_value(set_list, 'state', info['state'])
            if info.has_key('registration_date') and info['registration_date'] != None:
                self.__add_one_name_and_value(set_list, 'registration_date', info['registration_date'])
            if info.has_key('expiration_date') and info['expiration_date'] != None:
                self.__add_one_name_and_value(set_list, 'expiration_date', info['expiration_date'])       
            if info.has_key('delete_date') and info['delete_date'] != None:
                self.__add_one_name_and_value(set_list, 'delete_date', info['delete_date'])   
            if info.has_key('follow') and info['follow'] != None:
                self.__add_one_name_and_value(set_list, 'follow', info['follow'])   
            if info.has_key('blacklist') and info['blacklist'] != None:
                self.__add_one_name_and_value(set_list, 'blacklist', info['blacklist'])   

            self.db_update_common('yuming', set_list, where_list)

            return True
        except Exception,ex:
            self.logger.error("insert_one_yuming_info error:%s" %(ex))
            return False

def update_one_db_by_another(one_db_path, another_db_path):
    try:
        # open the databases
        one_db = Sqlite(one_db_path)
        another_db = Sqlite(another_db_path)

        # sync 
        offset = 0
        number = 500
        update_cnt = 0
        while True:
            another_data = another_db.get_yuming_infos(offset, number)
            size = len(another_data)
            if size == 0:
                logger.info("update_one_db_by_another there is no data to update, break.")
                break

            offset += size
            # sync the one_db
            for one_data in another_data:
                info = one_db.get_one_yuming_info(one_data['name'])
                if info == None:
                    if one_db.insert_one_yuming_info(one_data):
                        update_cnt += 1


        logger.info("another_db data count:%s, sync one_db count:%s." %(offset, update_cnt))

    except Exception,ex:
        logger.error("update_one_db_by_another error:%s" %(ex))


if __name__ == "__main__":
    db = Sqlite()

    data =  {
                    "name" : "baidu.com",
                    "state" : "used",
                    "registration_date" : "1999-10-11",
                    "expiration_date" : "2017-10-11",
                    "delete_date" : "2017-12-25"
                }

    #info = db.get_one_yuming_info('baidu.com')
    #print "get info:%s" %(info)

    #db.insert_one_yuming_info(data)

    #ret = db.get_yuming_infos_by_expiration_date('2016-03-12')
    #print ret

    update_one_db_by_another('/usr/local/yuming/data/yuming.db','/tmp/yuming.db')