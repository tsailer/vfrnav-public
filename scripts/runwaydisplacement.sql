ATTACH DATABASE 'airports.db' AS db0;
ATTACH DATABASE 'airports1.db' AS db1;

DROP TABLE IF EXISTS runwaydisplacements;
CREATE TABLE runwaydisplacements (
  ICAO TEXT NOT NULL,
  RWYIDENT TEXT,
  LAT INTEGER,
  LON INTEGER);

DROP INDEX IF EXISTS runwaydisplacements_index;
CREATE UNIQUE INDEX runwaydisplacements_index ON runwaydisplacements(ICAO,RWYIDENT);

INSERT INTO runwaydisplacements (ICAO,RWYIDENT,LAT,LON)
  SELECT db0.airports.ICAO,db0.airportrunways.IDENTHE,
         (db1.airportrunways.HELAT-db0.airportrunways.HELAT),
         (db1.airportrunways.HELON-db0.airportrunways.HELON)
    FROM db0.airports,db1.airports,db0.airportrunways,db1.airportrunways
    WHERE db0.airports.ICAO = db1.airports.ICAO
      AND db0.airports.ID = db0.airportrunways.ARPTID
      AND db1.airports.ID = db1.airportrunways.ARPTID
      AND db0.airportrunways.IDENTHE = db1.airportrunways.IDENTHE
      AND (db0.airportrunways.HELON != db1.airportrunways.HELON
           OR db0.airportrunways.HELAT != db1.airportrunways.HELAT);

INSERT INTO runwaydisplacements (ICAO,RWYIDENT,LAT,LON)
  SELECT db0.airports.ICAO,db0.airportrunways.IDENTLE,
         (db1.airportrunways.LELAT-db0.airportrunways.LELAT),
         (db1.airportrunways.LELON-db0.airportrunways.LELON)
    FROM db0.airports,db1.airports,db0.airportrunways,db1.airportrunways
    WHERE db0.airports.ICAO = db1.airports.ICAO
      AND db0.airports.ID = db0.airportrunways.ARPTID
      AND db1.airports.ID = db1.airportrunways.ARPTID
      AND db0.airportrunways.IDENTLE = db1.airportrunways.IDENTLE
      AND (db0.airportrunways.LELON != db1.airportrunways.LELON
           OR db0.airportrunways.LELAT != db1.airportrunways.LELAT);

INSERT INTO runwaydisplacements (ICAO,RWYIDENT,LAT,LON)
  SELECT db0.airports.ICAO,db0.airportrunways.IDENTHE,
         (db1.airportrunways.LELAT-db0.airportrunways.HELAT),
         (db1.airportrunways.LELON-db0.airportrunways.HELON)
    FROM db0.airports,db1.airports,db0.airportrunways,db1.airportrunways
    WHERE db0.airports.ICAO = db1.airports.ICAO
      AND db0.airports.ID = db0.airportrunways.ARPTID
      AND db1.airports.ID = db1.airportrunways.ARPTID
      AND db0.airportrunways.IDENTHE = db1.airportrunways.IDENTLE
      AND (db0.airportrunways.HELON != db1.airportrunways.LELON
           OR db0.airportrunways.HELAT != db1.airportrunways.LELAT);

INSERT INTO runwaydisplacements (ICAO,RWYIDENT,LAT,LON)
  SELECT db0.airports.ICAO,db0.airportrunways.IDENTLE,
         (db1.airportrunways.HELAT-db0.airportrunways.LELAT),
         (db1.airportrunways.HELON-db0.airportrunways.LELON)
    FROM db0.airports,db1.airports,db0.airportrunways,db1.airportrunways
    WHERE db0.airports.ICAO = db1.airports.ICAO
      AND db0.airports.ID = db0.airportrunways.ARPTID
      AND db1.airports.ID = db1.airportrunways.ARPTID
      AND db0.airportrunways.IDENTLE = db1.airportrunways.IDENTHE
      AND (db0.airportrunways.LELON != db1.airportrunways.HELON
           OR db0.airportrunways.LELAT != db1.airportrunways.HELAT);

DROP TABLE IF EXISTS airportdisplacements;
CREATE TABLE airportdisplacements (
  ICAO TEXT UNIQUE NOT NULL,
  COUNT INTEGER,
  LAT INTEGER,
  LON INTEGER);

CREATE TRIGGER aprtdispinsert_trigger INSTEAD OF INSERT ON airportdisplacements
BEGIN
  INSERT OR IGNORE INTO airportdisplacements (ICAO,COUNT,LAT,LON) VALUES (NEW.ICAO,0,0,0);
  UPDATE airportdisplacements SET COUNT=COUNT+1, LAT=LAT+NEW.LAT, LON=LON+NEW.LON WHERE ICAO=NEW.ICAO;
END;

INSERT INTO airportdisplacements (ICAO,COUNT,LAT,LON) SELECT ICAO,1,LAT,LON FROM runwaydisplacements;
