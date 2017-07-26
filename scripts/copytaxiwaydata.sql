ATTACH DATABASE 'airports1.db' AS db1;

DELETE FROM main.airporthelipads WHERE ARPTID IN (
  SELECT main.airports.ID
    FROM db1.airports,db1.airporthelipads,main.airports
    WHERE main.airports.ICAO = db1.airports.ICAO AND db1.airporthelipads.ARPTID = db1.airports.ID);
INSERT INTO airporthelipads (ARPTID,ID,IDENT,LENGTH,WIDTH,SURFACE,LAT,LON,HDG,ELEV,FLAGS)
  SELECT main.airports.ID,db1.airporthelipads.ID,db1.airporthelipads.IDENT,db1.airporthelipads.LENGTH,db1.airporthelipads.WIDTH,
         db1.airporthelipads.SURFACE,db1.airporthelipads.LAT,db1.airporthelipads.LON,db1.airporthelipads.HDG,db1.airporthelipads.ELEV,db1.airporthelipads.FLAGS
    FROM db1.airports,db1.airporthelipads,main.airports
    WHERE main.airports.ICAO = db1.airports.ICAO AND db1.airporthelipads.ARPTID = db1.airports.ID;

DELETE FROM main.airportlinefeatures WHERE ARPTID IN (
  SELECT main.airports.ID
    FROM db1.airports,db1.airportlinefeatures,main.airports
    WHERE main.airports.ICAO = db1.airports.ICAO AND db1.airportlinefeatures.ARPTID = db1.airports.ID);
INSERT INTO airportlinefeatures (ARPTID,ID,NAME,SURFACE,FLAGS,POLY)
  SELECT main.airports.ID,db1.airportlinefeatures.ID,db1.airportlinefeatures.NAME,db1.airportlinefeatures.SURFACE,db1.airportlinefeatures.FLAGS,db1.airportlinefeatures.POLY
    FROM db1.airports,db1.airportlinefeatures,main.airports
    WHERE main.airports.ICAO = db1.airports.ICAO AND db1.airportlinefeatures.ARPTID = db1.airports.ID;

DELETE FROM main.airportpointfeatures WHERE ARPTID IN (
  SELECT main.airports.ID
    FROM db1.airports,db1.airportpointfeatures,main.airports
    WHERE main.airports.ICAO = db1.airports.ICAO AND db1.airportpointfeatures.ARPTID = db1.airports.ID);
INSERT INTO airportpointfeatures (ARPTID,ID,FEATURE,LAT,LON,HDG,ELEV,SUBTYPE,ATTR1,ATTR2,NAME,RWYIDENT)
  SELECT main.airports.ID,db1.airportpointfeatures.ID,db1.airportpointfeatures.FEATURE,
         db1.airportpointfeatures.LAT,db1.airportpointfeatures.LON,db1.airportpointfeatures.HDG,db1.airportpointfeatures.ELEV,
         db1.airportpointfeatures.SUBTYPE,db1.airportpointfeatures.ATTR1,db1.airportpointfeatures.ATTR2,
         db1.airportpointfeatures.NAME,db1.airportpointfeatures.RWYIDENT
    FROM db1.airports,db1.airportpointfeatures,main.airports
    WHERE main.airports.ICAO = db1.airports.ICAO AND db1.airportpointfeatures.ARPTID = db1.airports.ID;

ANALYZE;
