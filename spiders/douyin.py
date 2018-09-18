import requests
import json
import re
import os
from pprint import pprint as pp
import queue


class DouYin:
    header = {
        'accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8',
        'accept-encoding': 'gzip, deflate, br',
        'accept-language': 'zh-CN,zh;q=0.9',
        'cache-control': 'max-age=0',
        'upgrade-insecure-requests': '1',
        'user-agent': 'Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1',
    }

    def __init__(self, url=None):
        self.url = self.get_RealAddress(url)
        # 获取用户视频的url
        self.user_video_url = 'https://www.douyin.com/aweme/v1/aweme/post/?{0}'
        self.user_id = re.search(r'user/(.*)\?', self.url).group(1)     # 用户id
        requests.packages.urllib3.disable_warnings()
        self.session = requests.Session()
        self.target_folder = ''     # 创建文件的路径
        self.queue = queue.Queue()      # 生成一个队列对象

    def user_info(self):
        self.mkdir_dir()
        p = os.popen('node fuck.js %s' % self.user_id)  # 获取加密的signature
        signature = p.readlines()[0]

        user_video_params = {
            'user_id': str(self.user_id),
            'count': '21',
            'max_cursor': '0',
            'aid': '1128',
            '_signature': signature
        }

        # 获取下载视频的列表
        def get_aweme_list(max_cursor=None):
            if max_cursor:
                user_video_params['max_cursor'] = str(max_cursor)
            user_video_url = self.user_video_url.format(
                '&'.join([key + '=' + user_video_params[key] for key in user_video_params]))    # 拼接参数
            response = requests.get(
                url=user_video_url, headers=self.header, verify=False)
            contentJson = json.loads(response.content.decode('utf-8'))  # 将返回的进行utf8编码
            aweme_list = contentJson.get('aweme_list', [])
            for aweme in aweme_list:
                video_name = aweme.get(
                    'share_info', None).get('share_desc', None)     # 视频的名字
                video_url = aweme.get('video', None).get('play_addr', None).get(
                    'url_list', None)[0].replace('playwm', 'play')      # 视频链接
                self.queue.put((video_name, video_url))  # 将数据进队列
            if contentJson.get('has_more') == 1:    # 判断后面是不是还有是1就是还有
                return get_aweme_list(contentJson.get('max_cursor'))    # 有的话获取参数max_cursor
        get_aweme_list()

    # 下载视频
    def get_download(self):
        while True:
            video_name, video_url = self.queue.get()
            file_name = video_name + '.mp4'
            file_path = os.path.join(self.target_folder, file_name)
            if not os.path.isfile(file_path):
                print('download %s form %s.\n' % (file_name, video_url))
                times = 0
                while times < 10:
                    try:
                        response = requests.get(
                            url=video_url, stream=True, timeout=10, verify=False)   # 开启流下载
                        with open(file_path, 'wb') as f:
                            for chunk in response.iter_content(1024):   # 返回迭代对象
                                f.write(chunk)
                            print('下载成功')
                        break
                    except:
                        print('下载失败')
                    times += 1

    # 创建对应的文件夹

    def mkdir_dir(self):
        current_folder = os.getcwd()
        self.target_folder = os.path.join(
            current_folder, 'download/%s' % self.user_id)
        if not os.path.isdir(self.target_folder):
            os.mkdir(self.target_folder)

    # 短链接转长地址
    def get_RealAddress(self, url):
        if url.find('v.douyin.com') < 0:
            return url
        response = requests.get(
            url=url, headers=self.header, allow_redirects=False)  # allow_redirects 允许跳转
        return response.headers['Location']

if __name__ == '__main__':
    print("start")
    douyin = DouYin(url='http://v.douyin.com/duAERB/')
    douyin.user_info()
    print("over")
    douyin.get_download()