ATTACH DATABASE 'mapelementsdbnew.db' AS dbnew;
DROP TABLE IF EXISTS dbnew.mapelements;
CREATE TABLE dbnew.mapelements (ID INTEGER PRIMARY KEY NOT NULL,
                                TYPECODE INTEGER,
                                NAME TEXT,
                                LAT INTEGER,
                                LON INTEGER,
                                SWLAT INTEGER,
                                SWLON INTEGER,
                                NELAT INTEGER,
                                NELON INTEGER,
                                POLY BLOB);
CREATE INDEX dbnew.mapelements_name ON mapelements(NAME);
CREATE INDEX dbnew.mapelements_latlon ON mapelements(LAT,LON);
CREATE INDEX dbnew.mapelements_bbox ON mapelements(SWLAT,NELAT,SWLON,NELON);
CREATE TEMPORARY TABLE mapping (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                                OLDID INTEGER);
INSERT INTO mapping (OLDID) SELECT ID FROM mapelements ORDER BY SWLAT,NELAT,SWLON,NELON;
INSERT INTO dbnew.mapelements (ID,TYPECODE,NAME,LAT,LON,SWLAT,SWLON,NELAT,NELON,POLY)
  SELECT mapping.ID,TYPECODE,NAME,LAT,LON,SWLAT,SWLON,NELAT,NELON,POLY
  FROM mapelements,mapping ON mapelements.ID=mapping.OLDID
  ORDER BY mapping.ID;
ANALYZE dbnew;
