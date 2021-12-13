set enable_seqscan=off;

CREATE TABLE test_text (
	i text
) DISTRIBUTED BY(i);

INSERT INTO test_text VALUES ('a'),('ab'),('abc'),('abb'),('axy'),('xyz');

CREATE INDEX idx_text ON test_text USING gin (i);

SELECT * FROM test_text WHERE i<'abc' ORDER BY i;
SELECT * FROM test_text WHERE i<='abc' ORDER BY i;
SELECT * FROM test_text WHERE i='abc' ORDER BY i;
SELECT * FROM test_text WHERE i>='abc' ORDER BY i;
SELECT * FROM test_text WHERE i>'abc' ORDER BY i;
