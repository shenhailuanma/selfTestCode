CREATE TABLE  IF NOT EXISTS `douyin_video` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'the video id',
  `platform` varchar(16) DEFAULT '' COMMENT 'platform: douyin, kuaishou, ...',
  `title` varchar(32)  DEFAULT '',
  `url` varchar(1000)  DEFAULT '' COMMENT '',
  `md5`  varchar(32)  DEFAULT '' COMMENT 'md5',
  `urlmd5` varchar(32)  DEFAULT 'url md5',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;
