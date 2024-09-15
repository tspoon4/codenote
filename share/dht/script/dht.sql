-- Clean up previous tables and index
DROP INDEX dht_peers_ip;
DROP INDEX dht_peers_key;
DROP INDEX dht_torrents_name;
DROP TABLE dht_peers;
DROP TABLE dht_torrents;
DROP TABLE dht_crawl;
DROP TABLE dht_meta;
DROP TABLE dht_geoip;
DROP EXTENSION pg_trgm;


-- DHT peers table

CREATE TABLE dht_peers
(	
	time TIMESTAMP WITH TIME ZONE NOT NULL,
	peer BYTEA NOT NULL,
	hash BYTEA NOT NULL	
);

CREATE INDEX dht_peers_ip ON dht_peers(peer);
CREATE UNIQUE INDEX dht_peers_key ON dht_peers(hash, peer);

-- TORRENT info table

CREATE TABLE dht_torrents
(
	time TIMESTAMP WITH TIME ZONE NOT NULL,
	hash BYTEA PRIMARY KEY,	
	name VARCHAR(256) NOT NULL,
	files INT NOT NULL,
	size BIGINT NOT NULL
);

CREATE EXTENSION pg_trgm;
CREATE INDEX dht_torrents_name ON dht_torrents USING GIN(name gin_trgm_ops);

-- CRAWL statistics

CREATE TABLE dht_crawl
(
	start TIMESTAMP WITH TIME ZONE NOT NULL,
	stop TIMESTAMP WITH TIME ZONE NOT NULL,
	target BYTEA NOT NULL,
	total INT NOT NULL,
	timeout INT NOT NULL,
	errors INT NOT NULL,
	sample_ext INT NOT NULL,
	peers_found INT NOT NULL
);

CREATE TABLE dht_meta
(
	start TIMESTAMP WITH TIME ZONE NOT NULL,
	stop TIMESTAMP WITH TIME ZONE NOT NULL,
	total INT NOT NULL,
	timeout INT NOT NULL,
	errors INT NOT NULL,
	ut_metadata INT NOT NULL,
	torrents_found INT NOT NULL
);

-- DHT geolocalization table

CREATE TABLE dht_geoip
(
	time TIMESTAMP WITH TIME ZONE NOT NULL,
	subnet BYTEA PRIMARY KEY,
	country CHAR(2) NOT NULL,
	name VARCHAR(64) NOT NULL
);

