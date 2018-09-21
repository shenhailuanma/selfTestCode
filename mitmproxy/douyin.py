import json

def request(flow):
    # print("zx get url:" + flow.request.url)
    url='http://v'
    #筛选出以上面url为开头的url
    if flow.request.url.startswith(url):
        print("zx get video url:" + flow.request.url)

