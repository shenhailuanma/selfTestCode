CREATE TABLE  IF NOT EXISTS `short_video` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'the video id',
  `platform` int DEFAULT 0 COMMENT 'platform: 0:undefine 1:douyin, 2:kuaishou, ...',
  `status` int DEFAULT 0 COMMENT 'status: 0:init -1:unlike 1:used, ...',
  `title` varchar(128)  DEFAULT '' COMMENT 'title',
  `url` varchar(1000)  DEFAULT '' COMMENT 'video url',
  `md5`  varchar(32)  DEFAULT '' COMMENT 'md5',
  `urlmd5` varchar(32)  DEFAULT '' COMMENT 'urlmd5',
  `storepath` varchar(128)  DEFAULT '' COMMENT 'video storepath',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;
