CREATE TABLE IF NOT EXISTS dbset (
  session_id STRING NOT NULL,
  vserver_id STRING NOT NULL,
  username STRING NOT NULL,
  ip STRING NOT NULL,
  mac string STRING NOT NULL,
  session_start REAL NOT NULL,
  session_timeout REAL,
  bandwidth_slot_id INT,
  bandwidth_max_down REAL,
  bandwidth_max_up REAL,
  service_class STRING,
  service_class_slot_id INT,
  download_bytes BIGINT DEFAULT 0,
  upload_bytes BIGINT DEFAULT 0,
  download_speed INT DEFAULT 0,
  upload_speed INT DEFAULT 0,
  secure_token STRING,
  PRIMARY KEY (vserver_id,ip,mac)
);

CREATE TABLE IF NOT EXISTS nas (
  vserver_id STRING NOT NULL,
  nas_identifier STRING NOT NULL,
  PRIMARY KEY (vserver_id)
);

CREATE TABLE IF NOT EXISTS acct (
  ip_src CHAR(15) NOT NULL DEFAULT '0.0.0.0',
  ip_dst CHAR(15) NOT NULL DEFAULT '0.0.0.0',
  packets INT NOT NULL,
  bytes BIGINT NOT NULL,
  stamp_inserted DATETIME NOT NULL DEFAULT '0000-00-00 00:00:00',
  stamp_updated DATETIME,
  PRIMARY KEY (ip_src, ip_dst, stamp_inserted)
);

CREATE TRIGGER IF NOT EXISTS update_new_download AFTER INSERT ON acct
WHEN new.ip_src='0.0.0.0'
BEGIN
  UPDATE dbset SET download_bytes=new.bytes WHERE ip=new.ip_dst;
END;

CREATE TRIGGER IF NOT EXISTS update_download BEFORE UPDATE ON acct
WHEN old.ip_src='0.0.0.0'
BEGIN
  UPDATE dbset SET download_bytes=new.bytes,download_speed=IFNULL(((new.bytes-old.bytes)/(strftime('%s', datetime('now', 'localtime'))-strftime('%s', old.stamp_updated))), 0) WHERE ip=new.ip_dst;
END;

CREATE TRIGGER IF NOT EXISTS update_new_upload AFTER INSERT ON acct
WHEN new.ip_dst='0.0.0.0'
BEGIN
  UPDATE dbset SET upload_bytes=new.bytes WHERE ip=new.ip_src;
END;

CREATE TRIGGER IF NOT EXISTS update_upload BEFORE UPDATE ON acct
WHEN old.ip_dst='0.0.0.0'
BEGIN
  UPDATE dbset SET upload_bytes=new.bytes,upload_speed=IFNULL(((new.bytes-old.bytes)/(strftime('%s', datetime('now', 'localtime'))-strftime('%s', old.stamp_updated))), 0) WHERE ip=new.ip_src;
END;
